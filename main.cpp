/*
 * Copyright (C) 2026 Microchip Technology Inc.  All rights reserved.
 *   Wayne Jia <wayne.jia@microchip.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <egt/ui>
#include <atomic>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <alsa/asoundlib.h>

#include "kws_gui.h"

#include "vedya_kws_library.h"
#include "vedya_Model_class.h"

#define HOP_LENGTH_AUDIO 1600
#define SAMPLING_RATE 16000
#define CHANNELS 1

struct AudioContext
{
    snd_pcm_t* pcm{nullptr};
    snd_pcm_uframes_t period_frames{HOP_LENGTH_AUDIO};

    int16_t* buffers[2]{nullptr, nullptr};
    size_t buffer_bytes{0};
    int samples[2]{0, 0};

    int write_index{0};
    bool ready[2]{false, false};

    pthread_mutex_t lock;
    pthread_cond_t cond;

    std::atomic<bool> stop{false};
};

enum class KwsEvent
{
    None,
    WakewordDetected,
    CommandDetected,
    ListeningWakeword,
    ListeningCommand,
    Timeout
};

static std::atomic<KwsEvent> g_event{KwsEvent::None};
static std::atomic<int> g_event_class{-1};
static std::atomic<int> g_state{0};

static AudioContext g_audio;

static int init_alsa(AudioContext* ctx)
{
    snd_pcm_hw_params_t* hw;

    int ret = snd_pcm_open(&ctx->pcm, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (ret < 0)
    {
        std::cerr << "ALSA open failed: " << snd_strerror(ret) << std::endl;
        return -1;
    }

    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(ctx->pcm, hw);
    snd_pcm_hw_params_set_access(ctx->pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(ctx->pcm, hw, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(ctx->pcm, hw, CHANNELS);

    unsigned int rate = SAMPLING_RATE;
    snd_pcm_hw_params_set_rate_near(ctx->pcm, hw, &rate, nullptr);

    ctx->period_frames = HOP_LENGTH_AUDIO;
    snd_pcm_hw_params_set_period_size_near(ctx->pcm, hw, &ctx->period_frames, nullptr);

    ret = snd_pcm_hw_params(ctx->pcm, hw);
    if (ret < 0)
    {
        std::cerr << "ALSA hw params failed: " << snd_strerror(ret) << std::endl;
        return -1;
    }

    ctx->buffer_bytes = HOP_LENGTH_AUDIO * sizeof(int16_t);
    ctx->buffers[0] = new int16_t[HOP_LENGTH_AUDIO];
    ctx->buffers[1] = new int16_t[HOP_LENGTH_AUDIO];
    ctx->write_index = 0;
    ctx->ready[0] = false;
    ctx->ready[1] = false;

    pthread_mutex_init(&ctx->lock, nullptr);
    pthread_cond_init(&ctx->cond, nullptr);

    return 0;
}

static void* capture_thread_fn(void* arg)
{
    auto* ctx = static_cast<AudioContext*>(arg);
    int16_t temp[HOP_LENGTH_AUDIO];

    while (!ctx->stop.load())
    {
        int idx = ctx->write_index;

        snd_pcm_sframes_t n = snd_pcm_readi(ctx->pcm, temp, HOP_LENGTH_AUDIO);
        if (n <= 0)
        {
            snd_pcm_prepare(ctx->pcm);
            continue;
        }
        if (n != HOP_LENGTH_AUDIO)
            continue;

        std::memcpy(ctx->buffers[idx], temp, ctx->buffer_bytes);

        pthread_mutex_lock(&ctx->lock);
        ctx->samples[idx] = static_cast<int>(n);
        ctx->ready[idx] = true;
        ctx->write_index = 1 - idx;
        pthread_cond_signal(&ctx->cond);
        pthread_mutex_unlock(&ctx->lock);
    }
    return nullptr;
}

static void* processing_thread_fn(void* arg)
{
    auto* ctx = static_cast<AudioContext*>(arg);

    tKwsModelHandle model = nullptr;
    kws_status status{};

    if (vKwsModelCreate(&model) != 0 || model == nullptr)
    {
        std::cerr << "KWS model creation failed" << std::endl;
        ctx->stop.store(true);
        return nullptr;
    }

    int prev_listening_trigger = 1;
    int prev_listening_command = 0;
    int read_index = 0;

    while (!ctx->stop.load())
    {
        pthread_mutex_lock(&ctx->lock);
        while (!ctx->ready[read_index] && !ctx->stop.load())
            pthread_cond_wait(&ctx->cond, &ctx->lock);

        if (ctx->stop.load())
        {
            pthread_mutex_unlock(&ctx->lock);
            break;
        }

        int16_t* frame = ctx->buffers[read_index];
        int samples = ctx->samples[read_index];
        ctx->ready[read_index] = false;
        pthread_mutex_unlock(&ctx->lock);

        if (vKwsModelProcess(model, frame, samples, &status) != 0)
        {
            std::cerr << "KWS process failed" << std::endl;
            break;
        }

        if (status.listening_for_trigger && !prev_listening_trigger)
        {
            if (prev_listening_command)
                g_event.store(KwsEvent::Timeout);
            else
                g_event.store(KwsEvent::ListeningWakeword);
        }

        if (status.listening_for_command && !prev_listening_command)
        {
            g_event.store(KwsEvent::ListeningCommand);
        }

        if (status.detection_status == 1)
        {
            if (status.trigger_class != -1)
            {
                g_event_class.store(status.trigger_class);
                g_event.store(KwsEvent::WakewordDetected);
            }
            if (status.command_class != -1)
            {
                g_event_class.store(status.command_class);
                g_event.store(KwsEvent::CommandDetected);
            }
        }

        prev_listening_trigger = status.listening_for_trigger;
        prev_listening_command = status.listening_for_command;
        read_index = 1 - read_index;
    }

    vKwsModelDestroy(model);
    return nullptr;
}

int main(int argc, char** argv)
{
    egt::Application app(argc, argv);
    egt::TopWindow window;
    window.color(egt::Palette::ColorId::bg, egt::Color(0x0a, 0x0e, 0x27));

    KwsGui gui(window);

    bool audio_ok = (init_alsa(&g_audio) == 0);

    pthread_t cap_tid{}, proc_tid{};
    if (audio_ok)
    {
        pthread_create(&cap_tid, nullptr, capture_thread_fn, &g_audio);
        pthread_create(&proc_tid, nullptr, processing_thread_fn, &g_audio);
        std::cout << "KWS GUI started with live audio capture" << std::endl;
    }
    else
    {
        std::cerr << "Audio init failed - running in display-only mode" << std::endl;
    }

    egt::PeriodicTimer poll_timer(std::chrono::milliseconds(100));
    poll_timer.on_timeout([&gui]()
    {
        KwsEvent ev = g_event.exchange(KwsEvent::None);
        int cls = g_event_class.exchange(-1);

        switch (ev)
        {
        case KwsEvent::WakewordDetected:
            gui.on_wakeword_detected(cls);
            break;
        case KwsEvent::CommandDetected:
            gui.on_command_detected(cls);
            break;
        case KwsEvent::ListeningWakeword:
            gui.on_listening_wakeword();
            break;
        case KwsEvent::ListeningCommand:
            gui.on_listening_command();
            break;
        case KwsEvent::Timeout:
            gui.on_timeout();
            break;
        default:
            break;
        }
    });
    poll_timer.start();

    window.show();
    int ret = app.run();

    g_audio.stop.store(true);
    pthread_cond_signal(&g_audio.cond);

    if (audio_ok)
    {
        pthread_join(cap_tid, nullptr);
        pthread_join(proc_tid, nullptr);
        snd_pcm_close(g_audio.pcm);
    }

    delete[] g_audio.buffers[0];
    delete[] g_audio.buffers[1];

    return ret;
}

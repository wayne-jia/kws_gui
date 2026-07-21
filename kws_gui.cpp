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

#include "kws_gui.h"

#include "vedya_Model_class.h"

static const char* keyword_display_names[] = {
    "HELLO CHIP",
    "LIGHTS ON",
    "LIGHTS OFF",
    "START FAN",
    "STOP FAN",
    "PLAY MUSIC",
    "STOP MUSIC"
};

static const char* keyword_icons[] = {
    "\xE2\x9C\xA8",
    "\xF0\x9F\x92\xA1",
    "\xF0\x9F\x8C\x99",
    "\xF0\x9F\x8C\x80",
    "\xE2\x9B\x94",
    "\xF0\x9F\x8E\xB5",
    "\xE2\x8F\xB9",
};

KwsGui::KwsGui(egt::TopWindow& window)
    : m_window(window)
{
    create_background(window);
    create_title(window);
    create_keyword_panel(window);
    create_status_bar(window);
}

void KwsGui::create_background(egt::TopWindow& window)
{
    window.color(egt::Palette::ColorId::bg, m_bg_dark);
}

void KwsGui::create_title(egt::TopWindow& window)
{
    m_title_label = std::make_shared<egt::Label>(
        "EDGE AI  KEYWORD SPOTTING",
        egt::Rect(0, 20, 1280, 80),
        egt::AlignFlag::center);
    m_title_label->font(egt::Font("DejaVu Sans", 36, egt::Font::Weight::bold));
    m_title_label->color(egt::Palette::ColorId::label_text, m_cyan);
    m_title_label->color(egt::Palette::ColorId::label_bg, egt::Palette::transparent);
    window.add(m_title_label);

    auto subtitle = std::make_shared<egt::Label>(
        "Microchip SAMA7D65 | Voice Command Recognition",
        egt::Rect(0, 90, 1280, 40),
        egt::AlignFlag::center);
    subtitle->font(egt::Font("DejaVu Sans", 16));
    subtitle->color(egt::Palette::ColorId::label_text,
                    egt::Color(m_text_color, 150));
    subtitle->color(egt::Palette::ColorId::label_bg, egt::Palette::transparent);
    window.add(subtitle);
}

void KwsGui::create_keyword_panel(egt::TopWindow& window)
{
    m_pulse = std::make_shared<PulseWidget>(egt::Rect(40, 160, 400, 500));
    window.add(m_pulse);

    int panel_x = 500;
    int panel_y = 160;
    int label_h = 60;
    int label_gap = 10;
    int panel_w = 700;

    auto panel_title = std::make_shared<egt::Label>(
        "COMMAND LIST",
        egt::Rect(panel_x, panel_y, panel_w, 40),
        egt::AlignFlag::left);
    panel_title->font(egt::Font("DejaVu Sans", 14, egt::Font::Weight::bold));
    panel_title->color(egt::Palette::ColorId::label_text,
                       egt::Color(m_cyan, 180));
    panel_title->color(egt::Palette::ColorId::label_bg, egt::Palette::transparent);
    window.add(panel_title);

    panel_y += 50;

    for (int i = 0; i < NUM_KEYWORDS; i++)
    {
        std::string text = "  ";
        if (i == 0)
            text += std::string("[WAKE]  ") + keyword_display_names[i];
        else
            text += std::string("[CMD ")  + std::to_string(i) + "]  " + keyword_display_names[i];

        auto label = std::make_shared<egt::Label>(
            text,
            egt::Rect(panel_x, panel_y + i * (label_h + label_gap), panel_w, label_h),
            egt::AlignFlag::left | egt::AlignFlag::center_vertical);

        label->font(egt::Font("DejaVu Sans Mono", 22));
        label->color(egt::Palette::ColorId::label_text, m_text_color);
        label->color(egt::Palette::ColorId::label_bg, m_label_bg);
        label->border(1);
        label->color(egt::Palette::ColorId::border, m_label_border);

        window.add(label);
        m_keyword_labels.push_back(label);
    }
}

void KwsGui::create_status_bar(egt::TopWindow& window)
{
    m_status_label = std::make_shared<egt::Label>(
        "Say \"hello chip\" to wake up the system...",
        egt::Rect(0, 720, 1280, 60),
        egt::AlignFlag::center);
    m_status_label->font(egt::Font("DejaVu Sans", 20));
    m_status_label->color(egt::Palette::ColorId::label_text, m_cyan);
    m_status_label->color(egt::Palette::ColorId::label_bg,
                          egt::Color(0x0a, 0x0e, 0x27, 200));
    window.add(m_status_label);
}

void KwsGui::highlight_keyword(int idx)
{
    if (idx < 0 || idx >= NUM_KEYWORDS)
        return;

    reset_highlights();
    m_highlighted_idx = idx;
    m_keyword_labels[idx]->color(egt::Palette::ColorId::label_text, m_bg_dark);
    m_keyword_labels[idx]->color(egt::Palette::ColorId::label_bg, m_green);
    m_keyword_labels[idx]->color(egt::Palette::ColorId::border, m_green);
    m_pulse->trigger_burst();

    m_highlight_timer = std::make_unique<egt::PeriodicTimer>(std::chrono::seconds(2));
    m_highlight_timer->on_timeout([this]()
    {
        reset_highlights();
        m_highlight_timer->stop();
    });
    m_highlight_timer->start();
}

void KwsGui::reset_highlights()
{
    if (m_highlighted_idx >= 0 && m_highlighted_idx < NUM_KEYWORDS)
    {
        m_keyword_labels[m_highlighted_idx]->color(
            egt::Palette::ColorId::label_text, m_text_color);
        m_keyword_labels[m_highlighted_idx]->color(
            egt::Palette::ColorId::label_bg, m_label_bg);
        m_keyword_labels[m_highlighted_idx]->color(
            egt::Palette::ColorId::border, m_label_border);
    }
    m_highlighted_idx = -1;
}

void KwsGui::on_wakeword_detected(int class_idx)
{
    highlight_keyword(class_idx);
    m_status_label->text("Wake word detected! Listening for command...");
    m_status_label->color(egt::Palette::ColorId::label_text, m_purple);
    m_pulse->set_listening_command(true);
}

void KwsGui::on_command_detected(int class_idx)
{
    highlight_keyword(class_idx);
    std::string msg = std::string("Command: ") + keyword_display_names[class_idx] + "  Executed!";
    m_status_label->text(msg);
    m_status_label->color(egt::Palette::ColorId::label_text, m_green);
}

void KwsGui::on_listening_wakeword()
{
    m_status_label->text("Say \"hello chip\" to wake up the system...");
    m_status_label->color(egt::Palette::ColorId::label_text, m_cyan);
    m_pulse->set_listening_command(false);
}

void KwsGui::on_listening_command()
{
    m_status_label->text("Listening for command...");
    m_status_label->color(egt::Palette::ColorId::label_text, m_purple);
    m_pulse->set_listening_command(true);
}

void KwsGui::on_timeout()
{
    m_status_label->text("No command detected. Say \"hello chip\" to try again...");
    m_status_label->color(egt::Palette::ColorId::label_text,
                          egt::Color(0xff, 0x80, 0x00));
    m_pulse->set_listening_command(false);
}

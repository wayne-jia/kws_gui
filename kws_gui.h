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

#ifndef KWS_GUI_H
#define KWS_GUI_H

#include <egt/ui>
#include <vector>
#include <memory>
#include "pulse_widget.h"

class KwsGui
{
public:
    KwsGui(egt::TopWindow& window);

    void on_wakeword_detected(int class_idx);
    void on_command_detected(int class_idx);
    void on_listening_wakeword();
    void on_listening_command();
    void on_timeout();

private:
    void create_background(egt::TopWindow& window);
    void create_title(egt::TopWindow& window);
    void create_keyword_panel(egt::TopWindow& window);
    void create_status_bar(egt::TopWindow& window);
    void highlight_keyword(int idx);
    void reset_highlights();

    static constexpr int NUM_KEYWORDS = 7;

    egt::TopWindow& m_window;
    std::shared_ptr<PulseWidget> m_pulse;
    std::vector<std::shared_ptr<egt::Label>> m_keyword_labels;
    std::shared_ptr<egt::Label> m_status_label;
    std::shared_ptr<egt::Label> m_title_label;

    egt::Color m_bg_dark{egt::Color(0x0a, 0x0e, 0x27)};
    egt::Color m_bg_mid{egt::Color(0x1a, 0x1a, 0x3e)};
    egt::Color m_cyan{egt::Color(0x00, 0xd4, 0xff)};
    egt::Color m_purple{egt::Color(0x7b, 0x2f, 0xf7)};
    egt::Color m_green{egt::Color(0x00, 0xff, 0x88)};
    egt::Color m_text_color{egt::Color(0xe0, 0xf0, 0xff)};
    egt::Color m_label_bg{egt::Color(0x15, 0x1a, 0x40)};
    egt::Color m_label_border{egt::Color(0x30, 0x40, 0x70)};

    std::unique_ptr<egt::PeriodicTimer> m_highlight_timer;
    int m_highlighted_idx{-1};
};

#endif

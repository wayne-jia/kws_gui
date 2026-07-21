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

#ifndef PULSE_WIDGET_H
#define PULSE_WIDGET_H

#include <egt/ui>
#include <vector>

class PulseWidget : public egt::Widget
{
public:
    PulseWidget(const egt::Rect& rect = {});

    void draw(egt::Painter& painter, const egt::Rect& rect) override;

    void set_color(const egt::Color& color);
    void set_listening_command(bool cmd_mode);
    void trigger_burst();

private:
    struct Ring
    {
        float radius{0.0f};
        float alpha{1.0f};
    };

    egt::PeriodicTimer m_timer{std::chrono::milliseconds(33)};
    std::vector<Ring> m_rings;
    float m_max_radius{0.0f};
    float m_spawn_timer{0.0f};
    egt::Color m_base_color{egt::Color(0x00, 0xd4, 0xff)};
    egt::Color m_cmd_color{egt::Color(0x7b, 0x2f, 0xf7)};
    bool m_cmd_mode{false};
    float m_burst_alpha{0.0f};
};

#endif

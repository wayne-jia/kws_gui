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

#include "pulse_widget.h"
#include <algorithm>
#include <cmath>

PulseWidget::PulseWidget(const egt::Rect& rect)
    : Widget(rect)
{
    fill_flags().clear();
    m_max_radius = std::min(rect.width(), rect.height()) / 2.0f * 0.9f;

    m_timer.on_timeout([this]()
    {
        m_spawn_timer += 33.0f;
        if (m_spawn_timer >= 800.0f)
        {
            m_rings.push_back({0.0f, 1.0f});
            m_spawn_timer = 0.0f;
        }

        for (auto& ring : m_rings)
        {
            ring.radius += m_max_radius * 0.015f;
            ring.alpha = 1.0f - (ring.radius / m_max_radius);
        }

        m_rings.erase(
            std::remove_if(m_rings.begin(), m_rings.end(),
                [this](const Ring& r) { return r.radius >= m_max_radius; }),
            m_rings.end());

        if (m_burst_alpha > 0.0f)
            m_burst_alpha -= 0.03f;

        damage();
    });
    m_timer.start();

    m_rings.push_back({0.0f, 1.0f});
}

void PulseWidget::set_color(const egt::Color& color)
{
    m_base_color = color;
}

void PulseWidget::set_listening_command(bool cmd_mode)
{
    m_cmd_mode = cmd_mode;
}

void PulseWidget::trigger_burst()
{
    m_burst_alpha = 1.0f;
}

void PulseWidget::draw(egt::Painter& painter, const egt::Rect& /*rect*/)
{
    auto b = box();
    float cx = b.x() + b.width() / 2.0f;
    float cy = b.y() + b.height() / 2.0f;

    auto active_color = m_cmd_mode ? m_cmd_color : m_base_color;

    painter.save();

    for (const auto& ring : m_rings)
    {
        if (ring.alpha <= 0.0f)
            continue;

        auto alpha = static_cast<uint8_t>(ring.alpha * 200);
        egt::Color c(active_color.red(), active_color.green(),
                     active_color.blue(), alpha);

        float line_w = 2.0f + ring.alpha * 3.0f;

        painter.set(c);
        painter.line_width(line_w);
        painter.draw(egt::CircleF(egt::PointF(cx, cy), ring.radius));
        painter.stroke();
    }

    float core_pulse = 0.6f + 0.4f * std::sin(m_spawn_timer * 0.005f);
    float core_radius = 20.0f * core_pulse;
    auto core_alpha = static_cast<uint8_t>(180 * core_pulse);
    egt::Color core_color(active_color.red(), active_color.green(),
                          active_color.blue(), core_alpha);
    painter.set(core_color);
    painter.draw(egt::CircleF(egt::PointF(cx, cy), core_radius));
    painter.fill();

    if (m_burst_alpha > 0.0f)
    {
        auto ba = static_cast<uint8_t>(m_burst_alpha * 150);
        egt::Color burst(0x00, 0xff, 0x88, ba);
        float br = m_max_radius * (1.0f - m_burst_alpha) * 0.8f;
        painter.set(burst);
        painter.line_width(4.0f);
        painter.draw(egt::CircleF(egt::PointF(cx, cy), br));
        painter.stroke();
    }

    painter.restore();
}

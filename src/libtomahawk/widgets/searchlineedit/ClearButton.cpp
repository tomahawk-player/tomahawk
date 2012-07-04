/**
 * Copyright (c) 2009, Benjamin C. Meyer  <ben@meyerhome.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "ClearButton.h"

#include <qpainter.h>

ClearButton::ClearButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::NoFocus);
    setToolTip(tr("Clear"));
    setMinimumSize(22, 22);
    setVisible(false);

#if QT_VERSION >= 0x040600
    // First check for a style icon, current KDE provides one
    if (m_styleImage.isNull()) {
        QLatin1String iconName = (layoutDirection() == Qt::RightToLeft)
            ? QLatin1String("edit-clear-locationbar-ltr")
            : QLatin1String("edit-clear-locationbar-rtl");
        QIcon icon = QIcon::fromTheme(iconName);
        if (!icon.isNull())
            m_styleImage = icon.pixmap(16, 16).toImage();
    }
#endif
}

void ClearButton::textChanged(const QString &text)
{
    setVisible(!text.isEmpty());
}

void ClearButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    if (!m_styleImage.isNull()) {
        int x = (width() - m_styleImage.width()) / 2 - 1;
        int y = (height() - m_styleImage.height()) / 2 - 1;
        painter.drawImage(x, y, m_styleImage);
        return;
    }

    // Fall back to boring circle X
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPalette p = palette();
    QColor circleColor = isDown() ? p.color(QPalette::Dark) : p.color(QPalette::Mid);
    QColor xColor = p.color(QPalette::Window);

    // draw circle
    painter.setBrush(circleColor);
    painter.setPen(circleColor);
    int padding = width() / 5;
    int circleRadius = width() - (padding * 2);
    painter.drawEllipse(padding, padding, circleRadius, circleRadius);

    // draw X
    painter.setPen(xColor);
    padding = padding * 2;
    painter.drawLine(padding, padding,            width() - padding, width() - padding);
    painter.drawLine(padding, height() - padding, width() - padding, padding);
}


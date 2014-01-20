/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Teo Mrnjavac <teo@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCROLLINGLABEL_H
#define SCROLLINGLABEL_H

#include <QLabel>
#include <QStaticText>
#include <QTimer>


class ScrollingLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ScrollingLabel( QWidget* parent = 0 );

public slots:
    void setText( const QString& text );

protected:
    virtual void paintEvent( QPaintEvent* );
    virtual void resizeEvent( QResizeEvent* );
    virtual void enterEvent( QEvent* );
    virtual void leaveEvent( QEvent* );

private:
    bool m_isMouseOver;
    void updateText();
    QString m_text;
    QString m_separator;
    QStaticText m_staticText;
    int m_singleTextWidth;
    QSize m_wholeTextSize;
    bool m_scrollEnabled;
    int m_scrollPos;
    QImage m_alphaChannel;
    QImage m_buffer;
    QTimer m_timer;

private slots:
    virtual void onTimerTimeout();
};

#endif // SCROLLINGLABEL_H

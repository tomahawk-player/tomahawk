/**
 * Copyright (c) 2008 - 2009, Benjamin C. Meyer  <ben@meyerhome.net>
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

#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <qlineedit.h>

class QHBoxLayout;

/*
    LineEdit is a subclass of QLineEdit that provides an easy and simple
    way to add widgets on the left or right hand side of the text.

    The layout of the widgets on either side are handled by a QHBoxLayout.
    You can set the spacing around the widgets with setWidgetSpacing().

    As widgets are added to the class they are inserted from the outside
    into the center of the widget.
*/
class SideWidget;
class LineEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(QString inactiveText READ inactiveText WRITE setInactiveText)

public:
    enum WidgetPosition {
        LeftSide,
        RightSide
    };

    LineEdit(QWidget *parent = 0);
    LineEdit(const QString &contents, QWidget *parent = 0);

    void addWidget(QWidget *widget, WidgetPosition position);
    void removeWidget(QWidget *widget);
    void setWidgetSpacing(int spacing);
    int widgetSpacing() const;
    int textMargin(WidgetPosition position) const;
    QString inactiveText() const;
    void setInactiveText(const QString &text);

    void paintEvent(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event);
    bool event(QEvent *event);

protected slots:
    void updateTextMargins();

private:
    void init();
    void updateSideWidgetLocations();

    SideWidget *m_leftWidget;
    SideWidget *m_rightWidget;
    QHBoxLayout *m_leftLayout;
    QHBoxLayout *m_rightLayout;
    QString m_inactiveText;
};

#endif // LINEEDIT_H


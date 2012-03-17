/*
Copyright (C) 2011 by Mike McQuaid
Copyright (C) 2011 by Leo Franchi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "qsearchfield.h"

#include <QLineEdit>
#include <QVBoxLayout>

#include "playlist/topbar/searchlineedit.h"
#include "utils/tomahawkutilsgui.h"

class DLLEXPORT QSearchFieldPrivate
{
public:
    QSearchFieldPrivate(SearchLineEdit *lineEdit) : lineEdit(lineEdit) {}
    SearchLineEdit *lineEdit;
};

QSearchField::QSearchField(QWidget *parent) : QWidget(parent)
{
    SearchLineEdit *lineEdit = new SearchLineEdit(this);
    connect(lineEdit, SIGNAL(textChanged(QString)),
            this, SIGNAL(textChanged(QString)));
    connect(lineEdit, SIGNAL(returnPressed()),
            this, SIGNAL(returnPressed()));

    pimpl = new QSearchFieldPrivate(lineEdit);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(lineEdit);
    TomahawkUtils::unmarginLayout(layout);
    setContentsMargins(0, 0, 0, 0);

    lineEdit->setStyleSheet( "QLineEdit { border: 1px solid gray; border-radius: 6px; }" );
    lineEdit->setMinimumHeight(27);
    setFixedHeight(27);

#ifdef Q_WS_MAC
    lineEdit->setContentsMargins(0, 0, 0, 0);
#else
    lineEdit->setContentsMargins(2, 2, 2, 2);
#endif

}

void QSearchField::setText(const QString &text)
{
    pimpl->lineEdit->setText(text);
}

void QSearchField::setPlaceholderText(const QString& text)
{
    pimpl->lineEdit->setInactiveText( text );
}

void QSearchField::clear()
{
    pimpl->lineEdit->clear();
}

QString QSearchField::text() const
{
    return pimpl->lineEdit->text();
}

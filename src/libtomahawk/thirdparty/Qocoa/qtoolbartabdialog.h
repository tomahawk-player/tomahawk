/*
 Copyright (C) 2012 by Leo Franchi <lfranchi@kde.org>

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

#ifndef QTOOLBARTABWIDGET_H
#define QTOOLBARTABWIDGET_H

#include <QObject>
#include <QScopedPointer>
#include <QPixmap>

#include "DllMacro.h"

class QToolbarTabDialogPrivate;

class QAction;

/**
 Dialog with a toolbar that behaves like a tab widget.

 Note that on OS X there are no OK/Cancel buttons, every setting should be applied immediately.
 The accepted() signal will be emitted on close/hide regardless.
 */
class DLLEXPORT QToolbarTabDialog : public QObject
{
    Q_OBJECT
public:
    QToolbarTabDialog();
    virtual ~QToolbarTabDialog();

    /*
     * If the given widget has a QSizePolicy of Fixed in either direction, the dialog will not be resizable in that
     * direction.
     */
    void addTab(QWidget* page, const QPixmap& icon, const QString& label, const QString& tooltip = QString());

    void setCurrentIndex(int index);

    void show();
    void hide();

Q_SIGNALS:
    void accepted();
    void rejected();

private:
    QScopedPointer<QToolbarTabDialogPrivate> pimpl;

    friend class ::QToolbarTabDialogPrivate;
};

#endif // QTOOLBARTABWIDGET_H

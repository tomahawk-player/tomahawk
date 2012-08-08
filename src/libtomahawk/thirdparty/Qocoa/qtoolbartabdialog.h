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

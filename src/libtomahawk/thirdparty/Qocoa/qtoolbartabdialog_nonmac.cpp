#include "qtoolbartabdialog.h"

#include <QToolBar>
#include <QStackedWidget>
#include <QAction>
#include <QVBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDebug>

class QToolbarTabDialogPrivate : public QObject {
    Q_OBJECT
public:
    QToolbarTabDialogPrivate(QToolbarTabDialog* qq) : q(qq), layout(0), toolbar(0), rightSpacer(0), stack(0), separator(0), buttons(0), actionGroup(0) {}

public slots:
    void actionTriggered(QAction* action) {
        if (dialog.isNull())
            return;

        const int idx = toolbar->actions().indexOf(action);
        Q_ASSERT(idx > -1);
        if (idx < 0)
            return;

        stack->setCurrentIndex(idx);
    }

    void accepted() {
        Q_ASSERT(!dialog.isNull());
        Q_ASSERT(!q.isNull());

        dialog.data()->hide();
        emit q.data()->accepted();
    }

    void rejected() {
        Q_ASSERT(!dialog.isNull());
        Q_ASSERT(!q.isNull());

        dialog.data()->hide();
        emit q.data()->rejected();
    }

public:
    QWeakPointer<QDialog> dialog;
    QWeakPointer<QToolbarTabDialog> q;

    QVBoxLayout* layout;
    QToolBar* toolbar;
    QAction* rightSpacer;
    QStackedWidget* stack;
    QFrame* separator;
    QDialogButtonBox* buttons;
    QActionGroup* actionGroup;

};

QToolbarTabDialog::QToolbarTabDialog() :
    QObject(0),
    pimpl(new QToolbarTabDialogPrivate(this))
{
    pimpl->dialog = new QDialog;
    pimpl->dialog.data()->setModal(true);

    pimpl->toolbar = new QToolBar(pimpl->dialog.data());
    pimpl->toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    pimpl->stack = new QStackedWidget(pimpl->dialog.data());

    pimpl->separator = new QFrame(pimpl->dialog.data());
    pimpl->separator->setFrameShape(QFrame::HLine);
    pimpl->separator->setFrameShadow(QFrame::Sunken);

    pimpl->actionGroup = new QActionGroup(pimpl->dialog.data());

    connect(pimpl->toolbar, SIGNAL(actionTriggered(QAction*)), pimpl.data(), SLOT(actionTriggered(QAction*)));

    pimpl->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, pimpl->dialog.data());
    connect(pimpl->buttons, SIGNAL(accepted()), pimpl->dialog.data(), SLOT(accept()));
    connect(pimpl->buttons, SIGNAL(rejected()), pimpl->dialog.data(), SLOT(reject()));

    connect(pimpl->dialog.data(), SIGNAL(accepted()), pimpl.data(), SLOT(accepted()));
    connect(pimpl->dialog.data(), SIGNAL(rejected()), pimpl.data(), SLOT(rejected()));

    QWidget* leftSpacer = new QWidget(pimpl->toolbar);
    leftSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QWidget* rightSpacer = new QWidget(pimpl->toolbar);
    rightSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    pimpl->toolbar->addWidget(leftSpacer);
    pimpl->rightSpacer =  pimpl->toolbar->addWidget(rightSpacer);

    pimpl->layout = new QVBoxLayout;
    pimpl->layout->addWidget(pimpl->toolbar);
    pimpl->layout->addWidget(pimpl->separator);
    pimpl->layout->addWidget(pimpl->stack);
    pimpl->layout->addWidget(pimpl->buttons);
    pimpl->dialog.data()->setLayout(pimpl->layout);
}

QToolbarTabDialog::~QToolbarTabDialog()
{
    if (pimpl && !pimpl->dialog.isNull()) {
        delete pimpl->dialog.data();
    }
}

void QToolbarTabDialog::addTab(QWidget* page, const QPixmap& icon, const QString& label, const QString& tooltip)
{
    Q_ASSERT(pimpl);
    if (!pimpl)
        return;

    pimpl->toolbar->removeAction(pimpl->rightSpacer);

    QAction* action = new QAction(icon, label, pimpl->toolbar);
    action->setCheckable(true);
    action->setToolTip(tooltip);

    pimpl->actionGroup->addAction(action);

    pimpl->toolbar->addAction(action);
    pimpl->stack->addWidget(page);

    pimpl->toolbar->addAction(pimpl->rightSpacer);
}

void QToolbarTabDialog::setCurrentIndex(int index)
{
    Q_ASSERT(pimpl);
    if (!pimpl || pimpl->dialog.isNull())
        return;


    Q_ASSERT(index < pimpl->toolbar->actions().length());
    Q_ASSERT(index < pimpl->stack->count());
    if (index < 0 || index > pimpl->toolbar->actions().length())
        return;
    if (index > pimpl->stack->count())
        return;

    if (pimpl->stack->currentIndex() != index)
        pimpl->stack->setCurrentIndex(index);
}

void QToolbarTabDialog::show()
{
    Q_ASSERT(pimpl);
    Q_ASSERT(!pimpl->dialog.isNull());
    if (!pimpl || pimpl->dialog.isNull())
        return;

    pimpl->dialog.data()->show();
}

void QToolbarTabDialog::hide()
{
    Q_ASSERT(pimpl);
    Q_ASSERT(!pimpl->dialog.isNull());
    if (!pimpl || pimpl->dialog.isNull())
        return;

    pimpl->dialog.data()->hide();
}

#include "moc_qtoolbartabdialog_nonmac.cpp"

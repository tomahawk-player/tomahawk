#ifndef SOURCETREEITEMWIDGET_H
#define SOURCETREEITEMWIDGET_H

#include <QWidget>

#include "source.h"

namespace Ui
{
    class SourceTreeItemWidget;
}

class SourceTreeItemWidget : public QWidget
{
Q_OBJECT

public:
    SourceTreeItemWidget( const Tomahawk::source_ptr& source, QWidget* parent = 0 );
    ~SourceTreeItemWidget();

signals:
    void clicked();

public slots:
    void onOnline();
    void onOffline();

protected:
    void changeEvent( QEvent* e );

private slots:
    void gotStats( const QVariantMap& stats );
    void onLoadingStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State oldstate, const QString& info );

    void onInfoButtonClicked();

private:
    Tomahawk::source_ptr m_source;

    Ui::SourceTreeItemWidget* ui;
};

#endif // SOURCETREEITEMWIDGET_H

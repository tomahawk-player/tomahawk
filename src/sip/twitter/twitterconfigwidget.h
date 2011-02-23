#ifndef TWITTERCONFIGWIDGET_H
#define TWITTERCONFIGWIDGET_H

#include "sip/SipPlugin.h"

#include <qtweetstatus.h>
#include <qtweetuser.h>
#include <qtweetnetbase.h>

#include <QWidget>


namespace Ui {
    class TwitterConfigWidget;
}

class TwitterConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TwitterConfigWidget(SipPlugin* plugin = 0, QWidget *parent = 0);
    ~TwitterConfigWidget();

private slots:
    void authenticateTwitter();
    void startPostGotTomahawkStatus();
    void postGotTomahawkStatusAuthVerifyReply( const QTweetUser &user );
    void postGotTomahawkStatusUpdateReply( const QTweetStatus &status );
    void postGotTomahawkStatusUpdateError( QTweetNetBase::ErrorCode, const QString &errorMsg );

private:
    Ui::TwitterConfigWidget *ui;
    SipPlugin *m_plugin;
};

#endif // TWITTERCONFIGWIDGET_H

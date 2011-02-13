#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include <qtweetstatus.h>
#include <qtweetuser.h>
#include <qtweetnetbase.h>

class QNetworkReply;

namespace Ui
{
    class SettingsDialog;
    class ProxyDialog;
}

class ProxyDialog : public QDialog
{
Q_OBJECT

public:
    explicit ProxyDialog( QWidget *parent = 0 );
    ~ProxyDialog() {};

    void saveSettings();

private:
    Ui::ProxyDialog *ui;
};

class SettingsDialog : public QDialog
{
Q_OBJECT

public:
    static const unsigned int VERSION = 1; // config version

    explicit SettingsDialog( QWidget *parent = 0 );
    ~SettingsDialog();

Q_SIGNALS:
    void settingsChanged();
    
protected:
    void changeEvent( QEvent *e );

private slots:
    void onRejected();
    void showPathSelector();
    void doScan();
    
    void showProxySettings();

    void testLastFmLogin();
    void onLastFmFinished();
    
    void authenticateTwitter();
    void startPostGotTomahawkStatus();
    void postGotTomahawkStatusAuthVerifyReply( const QTweetUser &user );
    void postGotTomahawkStatusUpdateReply( const QTweetStatus &status );
    void postGotTomahawkStatusUpdateError( QTweetNetBase::ErrorCode, const QString &errorMsg );
    
    void addScriptResolver();
    void scriptSelectionChanged();
    void removeScriptResolver();
    
private:
    Ui::SettingsDialog *ui;

    ProxyDialog m_proxySettings;
    bool m_rejected;
    QNetworkReply* m_testLastFmQuery;
};

#endif // SETTINGSDIALOG_H

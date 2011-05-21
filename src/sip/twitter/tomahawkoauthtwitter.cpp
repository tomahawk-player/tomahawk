#include "tomahawkoauthtwitter.h"
#include <QInputDialog>
#include <QDebug>

#define CONSUMER_KEY "C4v4Wfa21rfIDck4HMR3A"
#define CONSUMER_SECRET "zXSjU6bjrvg6UVMJX4JufqHyjj3iextY14SR9uBEAo"

TomahawkOAuthTwitter::TomahawkOAuthTwitter( QNetworkAccessManager *nam, QObject* parent )
    : OAuthTwitter( CONSUMER_KEY, CONSUMER_SECRET, parent )
{
    setNetworkAccessManager( nam );
}


int
TomahawkOAuthTwitter::authorizationWidget()
{
    bool ok;
    int i = QInputDialog::getInt(0, tr( "Twitter PIN" ), tr( "After authenticating on Twitter's web site,\nenter the displayed PIN number here:" ), 0, 0, 2147483647, 1, &ok);
    if (ok)
        return i;
    
    return 0;
}

void
TomahawkOAuthTwitter::error()
{
    qDebug() << Q_FUNC_INFO;
    setOAuthToken( QString().toLatin1() );
    setOAuthTokenSecret( QString().toLatin1() );
}

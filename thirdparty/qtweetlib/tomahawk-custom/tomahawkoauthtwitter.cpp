#include "tomahawkoauthtwitter.h"
#include <QInputDialog>
#include <QDebug>

TomahawkOAuthTwitter::TomahawkOAuthTwitter( QObject* parent )
    : OAuthTwitter( parent )
{
}


int
TomahawkOAuthTwitter::authorizationWidget()
{
    bool ok;
    int i = QInputDialog::getInt(0, QString( "Twitter PIN" ), QString( "After authenticating on Twitter's web site,\nenter the displayed PIN number here:" ), 0, 0, 2147483647, 1, &ok);
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

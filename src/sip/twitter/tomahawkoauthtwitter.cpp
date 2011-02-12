#include "tomahawkoauthtwitter.h"
#include <QInputDialog>

TomahawkOAuthTwitter::TomahawkOAuthTwitter( QObject* parent )
    : OAuthTwitter( parent )
{
}


int TomahawkOAuthTwitter::authorizationWidget()
{
    bool ok;
    int i = QInputDialog::getInt(0, QString( "Twitter PIN" ), QString( "After authenticating on Twitter's web site,\nenter the displayed PIN number here:" ), 0, 0, 2147483647, 1, &ok);
    if (ok)
        return i;
    
    return 0;
}

#include "tomahawkoauthtwitter.h"

#include <QtGui/QInputDialog>

#include "utils/logger.h"


TomahawkOAuthTwitter::TomahawkOAuthTwitter( QNetworkAccessManager *nam, QObject* parent )
    : OAuthTwitter( QByteArray::fromBase64( "QzR2NFdmYTIxcmZJRGNrNEhNUjNB" ), QByteArray::fromBase64( "elhTalU2Ympydmc2VVZNSlg0SnVmcUh5amozaWV4dFkxNFNSOXVCRUFv" ), parent )
{
    setNetworkAccessManager( nam );
}


const QString
TomahawkOAuthTwitter::authorizationWidget()
{
    bool ok;
    const QString str = QInputDialog::getText(0, tr( "Twitter PIN" ), tr( "After authenticating on Twitter's web site,\nenter the displayed PIN number here:" ), QLineEdit::Normal, QString(), &ok);
    if ( ok && !str.isEmpty() )
        return str;

    return QString();
}

void
TomahawkOAuthTwitter::error()
{
    qDebug() << Q_FUNC_INFO;
    setOAuthToken( QString().toLatin1() );
    setOAuthTokenSecret( QString().toLatin1() );
}

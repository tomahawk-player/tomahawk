/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */



#include "TomahawkOAuthTwitter.h"

#include "utils/Logger.h"

#include <QInputDialog>


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

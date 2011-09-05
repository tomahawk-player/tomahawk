/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TOMAHAWKUTILS_H
#define TOMAHAWKUTILS_H

#include "dllmacro.h"

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkProxy>
#include <QtCore/QStringList>
#include <QtCore/QRect>

#define RESPATH ":/data/"

class QPainter;
class QColor;
class QDir;
class QDateTime;
class QString;
class QPixmap;
class QLayout;
class QNetworkAccessManager;

namespace TomahawkUtils
{
    enum MediaType
    {
        MediaTypeArtist,
        MediaTypeAlbum,
        MediaTypeTrack
    };

    class DLLEXPORT NetworkProxyFactory : public QNetworkProxyFactory
    {
    public:
        NetworkProxyFactory()
            : m_proxy( QNetworkProxy::NoProxy )
            {}

        virtual ~NetworkProxyFactory() {}

        virtual QList< QNetworkProxy > queryProxy( const QNetworkProxyQuery & query = QNetworkProxyQuery() );
        static QList< QNetworkProxy > proxyForQuery( const QNetworkProxyQuery & query );

        void setNoProxyHosts( const QStringList &hosts );
        QStringList noProxyHosts() const { return m_noProxyHosts; }
        void setProxy( const QNetworkProxy &proxy );
        QNetworkProxy proxy() { return m_proxy; }

    private:
        QStringList m_noProxyHosts;
        QNetworkProxy m_proxy;
    };

    DLLEXPORT QString appFriendlyVersion();

    DLLEXPORT QDir appConfigDir();
    DLLEXPORT QDir appDataDir();
    DLLEXPORT QDir appLogDir();

    DLLEXPORT QString sqlEscape( QString sql );
    DLLEXPORT QString timeToString( int seconds );
    DLLEXPORT QString ageToString( const QDateTime& time );
    DLLEXPORT QString filesizeToString( unsigned int size );
    DLLEXPORT QString extensionToMimetype( const QString& extension );

    DLLEXPORT QColor alphaBlend( const QColor& colorFrom, const QColor& colorTo, float opacity );
    DLLEXPORT QPixmap createDragPixmap( MediaType type, int itemCount = 1 );

    DLLEXPORT void drawBackgroundAndNumbers( QPainter* p, const QString& text, const QRect& rect );

    DLLEXPORT void unmarginLayout( QLayout* layout );

    DLLEXPORT NetworkProxyFactory* proxyFactory();
    DLLEXPORT QNetworkAccessManager* nam();

    DLLEXPORT void setProxyFactory( TomahawkUtils::NetworkProxyFactory* factory );
    DLLEXPORT void setNam( QNetworkAccessManager* nam );

    DLLEXPORT QWidget* tomahawkWindow();
    /// Platform-specific bringing tomahawk mainwindow to front, b/c qt's activate() and such don't seem to work well enough for us
    DLLEXPORT void bringToFront();

    DLLEXPORT QPixmap createAvatarFrame( const QPixmap &avatar );
}

#endif // TOMAHAWKUTILS_H

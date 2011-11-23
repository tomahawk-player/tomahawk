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
#include <QPalette>

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

        NetworkProxyFactory( const NetworkProxyFactory &other );
        virtual ~NetworkProxyFactory() {}

        virtual QList< QNetworkProxy > queryProxy( const QNetworkProxyQuery & query = QNetworkProxyQuery() );
        static QList< QNetworkProxy > proxyForQuery( const QNetworkProxyQuery & query );

        virtual void setNoProxyHosts( const QStringList &hosts );
        virtual QStringList noProxyHosts() const { return m_noProxyHosts; }
        virtual void setProxy( const QNetworkProxy &proxy );
        virtual QNetworkProxy proxy() { return m_proxy; }

        virtual NetworkProxyFactory& operator=( const NetworkProxyFactory &rhs );
        virtual bool operator==( const NetworkProxyFactory &other ) const;

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
    DLLEXPORT QString ageToString( const QDateTime& time, bool appendAgoString = false );
    DLLEXPORT QString filesizeToString( unsigned int size );
    DLLEXPORT QString extensionToMimetype( const QString& extension );
    DLLEXPORT bool newerVersion( const QString& oldVersion, const QString& newVersion );

    DLLEXPORT QColor alphaBlend( const QColor& colorFrom, const QColor& colorTo, float opacity );
    DLLEXPORT QPixmap createDragPixmap( MediaType type, int itemCount = 1 );

    DLLEXPORT void drawBackgroundAndNumbers( QPainter* p, const QString& text, const QRect& rect );
    DLLEXPORT void drawQueryBackground( QPainter* p, const QPalette& palette, const QRect& r, qreal lightnessFactor = 1 );

    DLLEXPORT void unmarginLayout( QLayout* layout );

    DLLEXPORT NetworkProxyFactory* proxyFactory( bool noMutexLocker = false );
    DLLEXPORT QNetworkAccessManager* nam();

    DLLEXPORT void setProxyFactory( TomahawkUtils::NetworkProxyFactory* factory, bool noMutexLocker = false );
    DLLEXPORT void setNam( QNetworkAccessManager* nam, bool noMutexLocker = false );

    DLLEXPORT QWidget* tomahawkWindow();
    /// Platform-specific bringing tomahawk mainwindow to front, b/c qt's activate() and such don't seem to work well enough for us
    DLLEXPORT void bringToFront();

    DLLEXPORT QPixmap createAvatarFrame( const QPixmap &avatar );

    DLLEXPORT void crash();

    DLLEXPORT int headerHeight();
    DLLEXPORT void setHeaderHeight( int height );

    DLLEXPORT bool removeDirectory( const QString& dir );

    DLLEXPORT quint64 infosystemRequestId();
}

#endif // TOMAHAWKUTILS_H

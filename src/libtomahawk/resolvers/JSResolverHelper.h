/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef JSRESOLVERHELPER_H
#define JSRESOLVERHELPER_H

#include "DllMacro.h"
#include "Typedefs.h"

#include <boost/function.hpp>

#include <QObject>
#include <QVariantMap>

class JSResolver;

class DLLEXPORT JSResolverHelper : public QObject
{
Q_OBJECT

public:
    JSResolverHelper( const QString& scriptPath, JSResolver* parent );
    void setResolverConfig( const QVariantMap& config );

    // Return a HMAC (md5) signature of the input text with the desired key
    Q_INVOKABLE QString hmac( const QByteArray& key, const QByteArray& input );
    Q_INVOKABLE QString md5( const QByteArray& input );

    Q_INVOKABLE void addCustomUrlHandler( const QString& protocol, const QString& callbackFuncName, const QString& isAsynchronous = "false" );
    Q_INVOKABLE void reportStreamUrl( const QString& qid, const QString& streamUrl );
    Q_INVOKABLE void addCustomUrlTranslator( const QString& protocol, const QString& callbackFuncName, const QString& isAsynchronous = "false" );
    Q_INVOKABLE void reportUrlTranslation( const QString& qid, const QString& streamUrl );

    Q_INVOKABLE QByteArray base64Encode( const QByteArray& input );
    Q_INVOKABLE QByteArray base64Decode( const QByteArray& input );

    void customIODeviceFactory( const Tomahawk::result_ptr&, const QString& url,
                                boost::function< void( QSharedPointer< QIODevice >& ) > callback ); // async
    void customUrlTranslator( const Tomahawk::result_ptr&, const QString& url,
                                boost::function< void( const QString& ) > callback ); // async

public slots:
    QByteArray readRaw( const QString& fileName );
    QString readBase64( const QString& fileName );
    QString readCompressed( const QString& fileName );
    QString instanceUUID();

    QString compress( const QString& data );
    QVariantMap resolverData();

    void log( const QString& message );
    bool fakeEnv() { return false; }

    void addTrackResults( const QVariantMap& results );

    void addArtistResults( const QVariantMap& results );
    void addAlbumResults( const QVariantMap& results );
    void addAlbumTrackResults( const QVariantMap& results );

    void addUrlResult( const QString& url, const QVariantMap& result );

    void reportCapabilities( const QVariant& capabilities );

private slots:
    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::ModelMode, const Tomahawk::collection_ptr& collection );
    void pltemplateTracksLoadedForUrl( const QString& url, const Tomahawk::playlisttemplate_ptr& pltemplate );

private:
    Tomahawk::query_ptr parseTrack( const QVariantMap& track );
    void returnStreamUrl( const QString& streamUrl, boost::function< void( QSharedPointer< QIODevice >& ) > callback );
    void returnUrlTranslation( const QString& streamUrl, boost::function< void( const QString& ) > callback );

    QString m_scriptPath, m_urlCallback, m_urlTranslator;
    QHash< QString, boost::function< void( QSharedPointer< QIODevice >& ) > > m_streamCallbacks;
    QHash< QString, boost::function< void( const QString& ) > > m_translatorCallbacks;
    bool m_urlCallbackIsAsync;
    bool m_urlTranslatorIsAsync;
    QVariantMap m_resolverConfig;
    JSResolver* m_resolver;
    QString m_pendingUrl;
    Tomahawk::album_ptr m_pendingAlbum;
};
#endif // JSRESOLVERHELPER_H

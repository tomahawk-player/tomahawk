/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef TOMAHAWK_INFOSYSTEM_H
#define TOMAHAWK_INFOSYSTEM_H

#include <QtCore/QCryptographicHash>
#include <QtCore/QObject>
#include <QtCore/QtDebug>
#include <QtCore/QMap>
#include <QtCore/QWeakPointer>
#include <QtCore/QSet>
#include <QtCore/QLinkedList>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QThread>
#include <QtCore/QStringList>

#include "DllMacro.h"
#include "utils/TomahawkUtils.h"
#include "Typedefs.h"

class QNetworkAccessManager;

namespace Tomahawk {

namespace InfoSystem {

class InfoSystemCache;
class InfoSystemWorker;

enum PushInfoFlags { // must be powers of 2
    PushNoFlag = 1,
    PushShortUrlFlag = 2
};


struct DLLEXPORT InfoRequestData {
    quint64 requestId;
    quint64 internalId; //do not assign to this; it may get overwritten by the InfoSystem
    QString caller;
    Tomahawk::InfoSystem::InfoType type;
    QVariant input;
    QVariantMap customData;
    uint timeoutMillis;
    bool allSources;

    InfoRequestData();

    InfoRequestData( const quint64 rId, const QString &callr, const Tomahawk::InfoSystem::InfoType typ, const QVariant &inputvar, const QVariantMap &custom );

private:
    void init( const QString& callr, const InfoType typ, const QVariant& inputvar, const QVariantMap& custom);
};


struct InfoPushData {
    QString caller;
    InfoType type;
    QVariant input;
    PushInfoFlags pushFlags;
    PushInfoPair infoPair;

    InfoPushData()
        : caller( QString() )
        , type( Tomahawk::InfoSystem::InfoNoInfo )
        , input( QVariant() )
        , pushFlags( Tomahawk::InfoSystem::PushNoFlag )
        , infoPair( Tomahawk::InfoSystem::PushInfoPair( QVariantMap(), QVariant() ) )
        {}

    InfoPushData( const QString &callr, const Tomahawk::InfoSystem::InfoType typ, const QVariant &inputvar, const Tomahawk::InfoSystem::PushInfoFlags pflags )
        : caller( callr )
        , type( typ )
        , input( inputvar )
        , pushFlags( pflags )
        , infoPair( Tomahawk::InfoSystem::PushInfoPair( QVariantMap(), QVariant() ) )
        {}

};


class DLLEXPORT InfoPlugin : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Creates the plugin. Do *not* perform any network-based setup tasks here; defer that to init(), which will be called automatically.
     *
     **/
    InfoPlugin();

    virtual ~InfoPlugin();

    QSet< InfoType > supportedGetTypes() const { return m_supportedGetTypes; }
    QSet< InfoType > supportedPushTypes() const { return m_supportedPushTypes; }

signals:
    void getCachedInfo( Tomahawk::InfoSystem::InfoStringHash criteria, qint64 newMaxAge, Tomahawk::InfoSystem::InfoRequestData requestData );
    void info( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );

    void updateCache( Tomahawk::InfoSystem::InfoStringHash criteria, qint64 maxAge, Tomahawk::InfoSystem::InfoType type, QVariant output );

protected slots:

    /**
     * @brief Called after the plugin has been moved to the appropriate thread. Do network-based setup tasks here.
     *
     * @return void
     **/
    virtual void init() = 0;

    virtual void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData ) = 0;
    virtual void pushInfo( Tomahawk::InfoSystem::InfoPushData pushData ) = 0;
    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData ) = 0;

protected:
    InfoType m_type;
    QSet< InfoType > m_supportedGetTypes;
    QSet< InfoType > m_supportedPushTypes;

private:
    friend class InfoSystem;
};


class InfoSystemCacheThread : public QThread
{
    Q_OBJECT

public:
    InfoSystemCacheThread( QObject *parent );
    virtual ~InfoSystemCacheThread();

    void run();

private:
    friend class InfoSystem;
    InfoSystemCache* cache() const;

    QWeakPointer< InfoSystemCache > m_cache;
};


class InfoSystemWorkerThread : public QThread
{
    Q_OBJECT

public:
    InfoSystemWorkerThread( QObject *parent );
    virtual ~InfoSystemWorkerThread();

    void run();

private:
    friend class DiagnosticsDialog;
    friend class InfoSystem;
    InfoSystemWorker* worker() const;

    QWeakPointer< InfoSystemWorker > m_worker;
};


class DLLEXPORT InfoSystem : public QObject
{
    Q_OBJECT

public:
    static InfoSystem* instance();

    InfoSystem( QObject* parent );
    ~InfoSystem();

    bool getInfo( const InfoRequestData& requestData );
    //WARNING: if changing timeoutMillis above, also change in below function in .cpp file
    bool getInfo( const QString &caller, const QVariantMap& customData, const InfoTypeMap& inputMap, const InfoTimeoutMap& timeoutMap = InfoTimeoutMap(), bool allSources = false );
    bool pushInfo( InfoPushData pushData );
    bool pushInfo( const QString& caller, const InfoTypeMap& input, const PushInfoFlags pushFlags );

    const InfoTypeSet& supportedGetTypes() const { return m_supportedGetTypes; }
    const InfoTypeSet& supportedPushTypes() const { return m_supportedPushTypes; }

    QWeakPointer< QThread > workerThread() const;

public slots:
    // InfoSystem takes ownership of InfoPlugins
    void addInfoPlugin( Tomahawk::InfoSystem::InfoPluginPtr plugin );
    void removeInfoPlugin( Tomahawk::InfoSystem::InfoPluginPtr plugin );

signals:
    void info( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void finished( QString target );
    void finished( QString target, Tomahawk::InfoSystem::InfoType type );

    void updatedSupportedGetTypes( Tomahawk::InfoSystem::InfoTypeSet supportedTypes );
    void updatedSupportedPushTypes( Tomahawk::InfoSystem::InfoTypeSet supportedTypes );

private slots:
    void init();
    void receiveUpdatedSupportedGetTypes( Tomahawk::InfoSystem::InfoTypeSet supportedTypes );
    void receiveUpdatedSupportedPushTypes( Tomahawk::InfoSystem::InfoTypeSet supportedTypes );

private:
    bool m_inited;
    InfoSystemCacheThread* m_infoSystemCacheThreadController;
    InfoSystemWorkerThread* m_infoSystemWorkerThreadController;

    InfoTypeSet m_supportedGetTypes;
    InfoTypeSet m_supportedPushTypes;

    static InfoSystem* s_instance;
};

}

}


inline uint qHash( Tomahawk::InfoSystem::InfoStringHash hash )
{
    QCryptographicHash md5( QCryptographicHash::Md5 );
    QStringList keys = hash.keys();
    keys.sort();
    foreach( QString key, keys )
    {
        md5.addData( key.toUtf8() );
        md5.addData( hash[key].toUtf8() );
    }

    QString hexData = md5.result();

    uint returnval = 0;

    foreach( uint val, hexData.toUcs4() )
        returnval += val;

    return returnval;
}

Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoRequestData );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoPushData );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoStringHash );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::PushInfoPair );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::PushInfoFlags );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoType );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoSystemCache* );
Q_DECLARE_METATYPE( QList< Tomahawk::InfoSystem::InfoStringHash > );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoPluginPtr );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoPlugin* );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoTypeSet );

Q_DECLARE_INTERFACE( Tomahawk::InfoSystem::InfoPlugin, "tomahawk.InfoPlugin/1.0" )

#endif // TOMAHAWK_INFOSYSTEM_H

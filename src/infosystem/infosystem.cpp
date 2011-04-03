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

#include <QCoreApplication>

#include "tomahawk/infosystem.h"
#include "tomahawkutils.h"
#include "infosystemcache.h"
#include "infoplugins/echonestplugin.h"
#include "infoplugins/musixmatchplugin.h"
#include "infoplugins/lastfmplugin.h"

namespace Tomahawk
{
    
namespace InfoSystem
{

InfoPlugin::InfoPlugin(QObject *parent)
        :QObject( parent )
    {
        qDebug() << Q_FUNC_INFO;
        InfoSystem *system = qobject_cast< InfoSystem* >( parent );
        if( system )
                QObject::connect(
                    system->getCache(),
                    SIGNAL( notInCache( QHash< QString, QString >, QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomDataHash ) ),
                    this,
                    SLOT( notInCacheSlot( QHash< QString, QString >, QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomDataHash ) )
                );
    }


InfoSystem::InfoSystem(QObject *parent)
    : QObject(parent)
{
    qDebug() << Q_FUNC_INFO;
    qRegisterMetaType<QMap< QString, QMap< QString, QString > > >("Tomahawk::InfoSystem::InfoGenericMap");
    qRegisterMetaType<QHash<QString, QVariant > >("Tomahawk::InfoSystem::InfoCustomDataHash");
    
    m_infoSystemCacheThreadController = new QThread( this );
    m_cache = new InfoSystemCache();
    m_cache->moveToThread( m_infoSystemCacheThreadController );
    m_infoSystemCacheThreadController->start( QThread::IdlePriority );
    
    InfoPluginPtr enptr(new EchoNestPlugin(this));
    m_plugins.append(enptr);
    InfoPluginPtr mmptr(new MusixMatchPlugin(this));
    m_plugins.append(mmptr);
    InfoPluginPtr lfmptr(new LastFmPlugin(this));
    m_plugins.append(lfmptr);
    
    Q_FOREACH( InfoPluginPtr plugin, m_plugins )
    {
        connect(plugin.data(), SIGNAL(info(QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomDataHash)),
                this,       SLOT(infoSlot(QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomDataHash)), Qt::UniqueConnection);
        connect(plugin.data(), SIGNAL(finished(QString, Tomahawk::InfoSystem::InfoType)),
                this,       SLOT(finishedSlot(QString, Tomahawk::InfoSystem::InfoType)), Qt::UniqueConnection);
    }
    connect(m_cache, SIGNAL(info(QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomDataHash)),
            this,       SLOT(infoSlot(QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomDataHash)), Qt::UniqueConnection);
    connect(m_cache, SIGNAL(finished(QString, Tomahawk::InfoSystem::InfoType)),
            this,       SLOT(finishedSlot(QString, Tomahawk::InfoSystem::InfoType)), Qt::UniqueConnection);
}

InfoSystem::~InfoSystem()
{
    qDebug() << Q_FUNC_INFO;
    Q_FOREACH( InfoPluginPtr plugin, m_plugins )
    {
        if( plugin )
            delete plugin.data();
    }

    if( m_infoSystemCacheThreadController )
    {
        m_infoSystemCacheThreadController->quit();
    
        while( !m_infoSystemCacheThreadController->isFinished() )
        {
            QCoreApplication::processEvents( QEventLoop::AllEvents, 200 );
            TomahawkUtils::Sleep::msleep( 100 );
        }
        
        if( m_cache )
        {
            delete m_cache;
            m_cache = 0;
        }
        
        delete m_infoSystemCacheThreadController;
        m_infoSystemCacheThreadController = 0;
    }
}

void InfoSystem::registerInfoTypes(const InfoPluginPtr &plugin, const QSet< InfoType >& types)
{
    qDebug() << Q_FUNC_INFO;
    Q_FOREACH(InfoType type, types)
        m_infoMap[type].append(plugin);
}

QLinkedList< InfoPluginPtr > InfoSystem::determineOrderedMatches(const InfoType type) const
{
    //Dummy function for now that returns the various items in the QSet; at some point this will
    //probably need to support ordering based on the data source
    QLinkedList< InfoPluginPtr > providers;
    Q_FOREACH(InfoPluginPtr ptr, m_infoMap[type])
        providers << ptr;
    return providers;
}

void InfoSystem::getInfo(const QString &caller, const InfoType type, const QVariant& data, InfoCustomDataHash customData)
{
    qDebug() << Q_FUNC_INFO;
    QLinkedList< InfoPluginPtr > providers = determineOrderedMatches(type);
    if (providers.isEmpty())
    {
        emit info(QString(), InfoNoInfo, QVariant(), QVariant(), customData);
        emit finished(caller);
        return;
    }
    
    InfoPluginPtr ptr = providers.first();
    if (!ptr)
    {
        emit info(QString(), InfoNoInfo, QVariant(), QVariant(), customData);
        emit finished(caller);
        return;
    }
    
    m_dataTracker[caller][type] = m_dataTracker[caller][type] + 1;
    qDebug() << "current count in dataTracker for type" << type << "is" << m_dataTracker[caller][type];
    ptr.data()->getInfo(caller, type, data, customData);
}

void InfoSystem::getInfo(const QString &caller, const InfoMap &input, InfoCustomDataHash customData)
{
    Q_FOREACH( InfoType type, input.keys() )
        getInfo(caller, type, input[type], customData);
}

void InfoSystem::infoSlot(QString target, InfoType type, QVariant input, QVariant output, InfoCustomDataHash customData)
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "current count in dataTracker is " << m_dataTracker[target][type];
    if (m_dataTracker[target][type] == 0)
    {
        qDebug() << "Caller was not waiting for that type of data!";
        return;
    }
    emit info(target, type, input, output, customData);
}

void InfoSystem::finishedSlot(QString target, InfoType type)
{
    qDebug() << Q_FUNC_INFO;
    m_dataTracker[target][type] = m_dataTracker[target][type] - 1;
    qDebug() << "current count in dataTracker is " << m_dataTracker[target][type];
    Q_FOREACH(InfoType testtype, m_dataTracker[target].keys())
    {
        if (m_dataTracker[target][testtype] != 0)
        {
            qDebug() << "found outstanding request of type" << testtype;
            return;
        }
    }
    qDebug() << "emitting finished with target" << target;
    emit finished(target);
}

} //namespace InfoSystem

} //namespace Tomahawk
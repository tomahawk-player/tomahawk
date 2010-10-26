#include "tomahawk/infosystem.h"
#include "infoplugins/echonestplugin.h"
#include "infoplugins/musixmatchplugin.h"

using namespace Tomahawk::InfoSystem;

InfoSystem::InfoSystem(QObject *parent)
    : QObject( parent )
{
    qDebug() << Q_FUNC_INFO;
    qRegisterMetaType<QMap< QString, QMap< QString, QString > > >("Tomahawk::InfoSystem::InfoGenericMap");
    qRegisterMetaType<QHash<QString, QVariant > >("Tomahawk::InfoSystem::InfoCustomDataHash");
    qRegisterMetaType<QHash<QString, QString > >("Tomahawk::InfoSystem::MusixMatchHash");
    InfoPluginPtr enptr(new EchoNestPlugin(this));
    m_plugins.append(enptr);
    InfoPluginPtr mmptr(new MusixMatchPlugin(this));
    m_plugins.append(mmptr);
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
        emit info(QString(), Tomahawk::InfoSystem::InfoNoInfo, QVariant(), QVariant(), customData);
        return;
    }
    
    InfoPluginPtr ptr = providers.first();
    if (!ptr)
    {
        emit info(QString(), Tomahawk::InfoSystem::InfoNoInfo, QVariant(), QVariant(), customData);
        return;
    }
    
    m_dataTracker[caller][type] = m_dataTracker[caller][type] + 1;
    qDebug() << "current count in dataTracker for type" << type << "is" << m_dataTracker[caller][type];
    connect(ptr.data(), SIGNAL(info(QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomDataHash)),
            this,       SLOT(infoSlot(QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomDataHash)), Qt::UniqueConnection);
    connect(ptr.data(), SIGNAL(finished(QString, Tomahawk::InfoSystem::InfoType)),
            this,       SLOT(finishedSlot(QString, Tomahawk::InfoSystem::InfoType)), Qt::UniqueConnection);
    ptr.data()->getInfo(caller, type, data, customData);
}

void InfoSystem::getInfo(const QString &caller, const InfoMap &input, InfoCustomDataHash customData)
{
    Q_FOREACH( InfoType type, input.keys() )
        getInfo(caller, type, input[type], customData);
}

void InfoSystem::infoSlot(QString target, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomDataHash customData)
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

void InfoSystem::finishedSlot(QString target, Tomahawk::InfoSystem::InfoType type)
{
    qDebug() << Q_FUNC_INFO;
    m_dataTracker[target][type] = m_dataTracker[target][type] - 1;
    qDebug() << "current count in dataTracker is " << m_dataTracker[target][type];
    Q_FOREACH(Tomahawk::InfoSystem::InfoType testtype, m_dataTracker[target].keys())
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
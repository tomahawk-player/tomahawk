#include "avatarmanager.h"


#include "utils/tomahawkutils.h"

#include <jreen/vcard.h>
#include <jreen/vcardupdate.h>
#include <jreen/presence.h>

#include <QDir>
#include <QDebug>
#include <QCryptographicHash>
#include <QPixmap>

AvatarManager::AvatarManager(jreen::Client *client) :
    m_cacheDir(TomahawkUtils::appDataDir().absolutePath().append("/jreen/"))
{
    m_client = client;

    m_cachedAvatars = m_cacheDir.entryList();

    connect(m_client, SIGNAL(serverFeaturesReceived(QSet<QString>)), SLOT(onNewConnection()));
    connect(m_client, SIGNAL(newPresence(jreen::Presence)), SLOT(onNewPresence(jreen::Presence)));
    connect(m_client, SIGNAL(newIQ(jreen::IQ)), SLOT(onNewIq(jreen::IQ)));

    connect(this, SIGNAL(newAvatar(QString)), SLOT(onNewAvatar(QString)));
}

AvatarManager::~AvatarManager()
{
}

void AvatarManager::onNewConnection()
{
    fetchVCard( m_client->jid().bare() );
}


void AvatarManager::fetchVCard(const QString &jid)
{
    qDebug() << Q_FUNC_INFO;

    jreen::IQ iq(jreen::IQ::Get, jid );
    iq.addExtension(new jreen::VCard());
    m_client->send( iq, this, SLOT( onNewIq( jreen::IQ, int ) ), 0 );
}

void AvatarManager::onNewPresence(const jreen::Presence& presence)
{
    jreen::VCardUpdate::Ptr update = presence.findExtension<jreen::VCardUpdate>();
    if(update)
    {
        qDebug() << "vcard: found update for " << presence.from().full();
        if(!isCached(update->photoHash()))
        {
            qDebug() << presence.from().full() << "vcard: photo not cached, starting request..." << update->photoHash();
            fetchVCard( presence.from().bare() );
        }
        else
        {
            qDebug() << presence.from().full() << "vcard: photo already cached no request necessary " << update->photoHash();
            m_JidsAvatarHashes.insert( update->photoHash(), presence.from().bare() );

            Q_ASSERT(!this->avatar(presence.from().bare()).isNull());
            emit newAvatar(presence.from().bare());
        }
    }
    else
    {
        qDebug() << Q_FUNC_INFO << presence.from().full() << "got no statusupdateextension";

        //TODO: do we want this? might fetch avatars for broken clients
        fetchVCard( presence.from().bare() );
    }
}

void AvatarManager::onNewIq(const jreen::IQ& iq, int context)
{
    jreen::VCard *vcard = iq.findExtension<jreen::VCard>().data();
    if(vcard)
    {
        iq.accept();

        qDebug() << Q_FUNC_INFO << "Got vcard from " << iq.from().full();

        QString id = iq.from().full();
        QString avatarHash;

        const jreen::VCard::Photo &photo = vcard->photo();
        if (!photo.data().isEmpty()) {
            qDebug() << "vcard: got photo data" << id;

            avatarHash = QCryptographicHash::hash(photo.data(), QCryptographicHash::Sha1).toHex();

            if (!m_cacheDir.exists())
                m_cacheDir.mkpath( avatarDir( avatarHash ).absolutePath() );


            QFile file(avatarPath(avatarHash));
            if (file.open(QIODevice::WriteOnly)) {
                file.write(photo.data());
                file.close();
            }

            m_cachedAvatars.append(avatarHash);
            m_JidsAvatarHashes.insert( avatarHash, iq.from().bare() );

            Q_ASSERT(!this->avatar(iq.from().bare()).isNull());
            emit newAvatar(iq.from().bare());
        }
        else
        {
            qDebug() << "vcard: got no photo data" << id;
        }

        // got own presence
        if ( m_client->jid().bare() == id )
        {
            qDebug() << Q_FUNC_INFO << "got own vcard";

            jreen::Presence presence = m_client->presence();
            jreen::VCardUpdate::Ptr update = presence.findExtension<jreen::VCardUpdate>();
            if (update->photoHash() != avatarHash)
            {
                qDebug() << Q_FUNC_INFO << "Updating own presence...";

                update->setPhotoHash(avatarHash);
                m_client->send(presence);
            }
        }
    }
}

QPixmap AvatarManager::avatar(const QString &jid) const
{
    if( isCached( avatarHash( jid ) ) )
    {
        return QPixmap( avatarPath( avatarHash( jid ) ) );
    }
    else
    {
        return QPixmap();
    }
}

QString AvatarManager::avatarHash(const QString &jid) const
{
    //qDebug() << Q_FUNC_INFO << jid << m_JidsAvatarHashes.key(jid);
    return m_JidsAvatarHashes.key(jid);
}

QDir AvatarManager::avatarDir(const QString &avatarHash) const
{
    return m_cacheDir;
}

QString AvatarManager::avatarPath(const QString &avatarHash) const
{
    Q_ASSERT(!avatarHash.contains("@"));
    return avatarDir(avatarHash).absoluteFilePath(avatarHash);
}

bool AvatarManager::isCached(const QString &avatarHash) const
{
    return m_cachedAvatars.contains( avatarHash );
}

void AvatarManager::onNewAvatar(const QString& jid)
{
    qDebug() << Q_FUNC_INFO <<  "Found new Avatar..." << jid;
}

#ifndef AVATARMANAGER_H
#define AVATARMANAGER_H

#include <jreen/client.h>

#include <QObject>
#include <QDir>


class AvatarManager : public QObject
{
Q_OBJECT

public:
    AvatarManager(jreen::Client *client);
    virtual ~AvatarManager();

    QPixmap avatar(const QString &jid) const;

signals:
    void newAvatar( const QString &jid );

private slots:
    void onNewPresence( const jreen::Presence& presence );
    void onNewIq(const jreen::IQ &iq, int context = 0 );
    void onNewConnection();
    void onNewAvatar( const QString &jid );

private:
    void fetchVCard( const QString &jid);
    QString avatarHash(const QString &jid) const;
    QString avatarPath(const QString &avatarHash) const;

    QDir avatarDir(const QString &avatarHash) const;
    bool isCached(const QString &avatarHash) const;

    jreen::Client *m_client;
    QStringList m_cachedAvatars;
    QDir m_cacheDir;
    QMap<QString, QString> m_JidsAvatarHashes;
};

#endif // AVATARMANAGER_H

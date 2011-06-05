#ifndef AVATARMANAGER_H
#define AVATARMANAGER_H

#include <jreen/client.h>

#include <QObject>
#include <QDir>

#include "../sipdllmacro.h"

class SIPDLLEXPORT AvatarManager : public QObject
{
Q_OBJECT

public:
    AvatarManager(Jreen::Client *client);
    virtual ~AvatarManager();

    QPixmap avatar(const QString &jid) const;

signals:
    void newAvatar( const QString &jid );

private slots:
    void onNewPresence( const Jreen::Presence& presence );
    void onNewIq(const Jreen::IQ &iq);
    void onNewConnection();
    void onNewAvatar( const QString &jid );

private:
    void fetchVCard( const QString &jid);
    QString avatarHash(const QString &jid) const;
    QString avatarPath(const QString &avatarHash) const;

    QDir avatarDir(const QString &avatarHash) const;
    bool isCached(const QString &avatarHash) const;

    Jreen::Client *m_client;
    QStringList m_cachedAvatars;
    QDir m_cacheDir;
    QMap<QString, QString> m_JidsAvatarHashes;
};

#endif // AVATARMANAGER_H

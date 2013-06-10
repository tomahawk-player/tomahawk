/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef CREDENTIALSMANAGER_H
#define CREDENTIALSMANAGER_H

#include "DllMacro.h"

#include <QObject>
#include <QVariantHash>
#include <QMutex>
#include <QStringList>


namespace QKeychain
{
class Job;
class ReadPasswordJob;
}

namespace Tomahawk
{

namespace Accounts
{

class CredentialsStorageKey
{
public:
    explicit CredentialsStorageKey( const QString &service, const QString &key );
    bool operator ==( const CredentialsStorageKey& other ) const;
    bool operator !=( const CredentialsStorageKey& other ) const;
    QString service() const { return m_service; }
    QString key() const { return m_key; }
private:
    QString m_service;
    QString m_key;
};

/**
 * @brief The CredentialsManager class holds an in-memory cache of whatever credentials are stored
 * in the system's QtKeychain-accessible credentials storage.
 * After instantiating the class, loadCredentials should be called, and this is the only time a read
 * operation from QtKeychain is performed. When CredentialsManager emits ready(), it can be used for
 * all other operations. The only QtKeychain operations performed at any time after startup are
 * write and delete.
 * This ensures an illusion of synchronous operations for Tomahawk's Account classes, even though all
 * QtKeychain jobs are async.
 */
class DLLEXPORT CredentialsManager : public QObject
{
    Q_OBJECT
public:
    explicit CredentialsManager( QObject* parent = 0 );
    
    void addService( const QString& service, const QStringList& accountIds );

    void loadCredentials();

    QList< CredentialsStorageKey > keys() const;

    QVariantHash credentials( const CredentialsStorageKey& key ) const;
    QVariantHash credentials( const QString& serviceName, const QString& key ) const;
    void setCredentials( const CredentialsStorageKey& key, const QVariantHash& value );
    void setCredentials( const QString& serviceName, const QString& key, const QVariantHash& value );

signals:
    void ready();

private slots:
    void keychainJobFinished( QKeychain::Job* );

private:
    QHash< QString, QStringList > m_services;
    QHash< CredentialsStorageKey, QVariantHash > m_credentials;
    QList< QKeychain::ReadPasswordJob* > m_readJobs;
    QMutex m_mutex;
};

uint qHash( const Tomahawk::Accounts::CredentialsStorageKey& key );

} //namespace Accounts

} //namespace Tomahawk

#endif // CREDENTIALSMANAGER_H

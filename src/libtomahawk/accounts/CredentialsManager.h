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

#include <QObject>
#include <QVariantHash>


namespace QKeychain
{
class Job;
class ReadPasswordJob;
class WritePasswordJob;
}


namespace Tomahawk
{

namespace Accounts
{

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
class CredentialsManager : public QObject
{
    Q_OBJECT
public:
    explicit CredentialsManager( QObject* parent = 0 );
    
    void loadCredentials( QStringList keys );

    QStringList keys() const;

    QVariantHash credentials( const QString& key ) const;
    void setCredentials( const QString& key, const QVariantHash& value );

signals:
    void ready();

private slots:
    void keychainJobFinished( QKeychain::Job* );

private:
    QHash< QString, QVariantHash > m_credentials;
    QList< QKeychain::ReadPasswordJob* > m_readJobs;
};

} //namespace Accounts

} //namespace Tomahawk
#endif // CREDENTIALSMANAGER_H

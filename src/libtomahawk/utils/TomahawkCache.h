/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Casey Link <unnamedrambler@gmail.com>
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

#ifndef TOMAHAWKCACHE_H
#define TOMAHAWKCACHE_H

#include "DllMacro.h"
#include "utils/TomahawkUtils.h"

#include <QMutex>
#include <QSettings>
#include <QObject>
#include <QTimer>

namespace TomahawkUtils {

/**
 * Internal data structure. Don't use.
 */
struct CacheData {
    CacheData(){}
    CacheData( qint64 maxAg, QVariant dat )
    : maxAge( maxAg )
    , data( dat )
    {}

    qint64 maxAge; //!< milliseconds
    QVariant data;
};

/**
 * A simple generic cache for anyone to use.
 *
 * Data is segmented according to clients, which specify
 * a client identifier.
 *
 * Structure is a basic key-value store with associated max lifetime in
 * milliseconds.
 */
class DLLEXPORT Cache : public QObject
{
Q_OBJECT

public:
    static Cache* instance();
    virtual ~Cache();

    /**
     * Store data in the cache.
     * @param identifier your unique identifier, used to segment your data.
     * @param maxAge lifetime of data in milliseconds (e.g, 3600000 = 1 hour)
     * @param key the key to store the data
     * @param value the data to store
     */
    void putData( const QString &identifier, qint64 maxAge, const QString &key, const QVariant& value );

    /**
     * Retrieve data from the cache.
     * @param identifier your unique identifier, used to segment your data.
     * @param key the key to store the data
     * @return the data, if found, if not found an invalid QVariant is returned.
     */
    QVariant getData( const QString &identifier, const QString &key );

private slots:
    void pruneTimerFired();

private:
    Cache();
    static Cache* s_instance;

    /**
     * Adds a client to the manifest.
     * Does not lock the mutex.
     */
    void addClient( const QString &identifier );

    /**
     * Removes a client to the manifest.
     * Does not lock the mutex.
     */
    void removeClient( const QString &identifier );

    QString m_cacheBaseDir;
    QSettings m_cacheManifest;
    QTimer m_pruneTimer;
    QMutex m_mutex;
};

}

inline QDataStream& operator<< ( QDataStream& in, const TomahawkUtils::CacheData& data )
{
   in << data.data << data.maxAge;
   return in;
}

inline QDataStream& operator>> ( QDataStream& out, TomahawkUtils::CacheData& data )
{
    out >> data.data;
    out >> data.maxAge;
    return out;
}

Q_DECLARE_METATYPE( TomahawkUtils::CacheData );

#endif // TOMAHAWKCACHE_H

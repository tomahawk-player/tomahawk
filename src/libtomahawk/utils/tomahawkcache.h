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

#include "dllmacro.h"
#include "utils/tomahawkutils.h"

#include <QSettings>
#include <QObject>
#include <QTimer>

namespace TomahawkUtils {

struct CacheData {
    CacheData(){}
    CacheData( qint64 maxAg, QVariant dat )
    : maxAge( maxAg )
    , data( dat )
    {}

    qint64 maxAge;
    QVariant data;
};

class DLLEXPORT TomahawkCache : public QObject
{
Q_OBJECT

public:
    static TomahawkCache* instance();
    virtual ~TomahawkCache();

    void putData( const QString &identifier, qint64 maxAge, const QString &key, const QVariant& value );
    QVariant getData( const QString &identifier, const QString &key );

private slots:
    void pruneTimerFired();

private:
    TomahawkCache();
    static TomahawkCache* s_instance;

    void addClient( const QString &identifier );
    void removeClient( const QString &identifier );

    QString m_cacheBaseDir;
    QSettings m_cacheManifest;
    QTimer m_pruneTimer;
};

}

Q_DECLARE_METATYPE( TomahawkUtils::CacheData );

#endif // TOMAHAWKCACHE_H

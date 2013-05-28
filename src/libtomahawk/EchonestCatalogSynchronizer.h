/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef ECHONESTCATALOGSYNCHRONIZER_H
#define ECHONESTCATALOGSYNCHRONIZER_H

// #include "Query.h"
// #include "database/DatabaseCommand_TrackAttributes.h"
//

#include "result_ptr.h"

#include <echonest/Catalog.h>
//
#include <QObject>
#include <QQueue>

#include "DllMacro.h"

class DatabaseCommand_SetCollectionAttributes;

namespace Tomahawk
{

class DLLEXPORT EchonestCatalogSynchronizer : public QObject
{
    Q_OBJECT
public:
    static EchonestCatalogSynchronizer* instance() {
        if ( !s_instance )
        {
            s_instance = new EchonestCatalogSynchronizer;
        }

        return s_instance;
    }

    explicit EchonestCatalogSynchronizer(QObject *parent = 0);

    Echonest::Catalog songCatalog() const { return m_songCatalog; }
    Echonest::Catalog artistCatalog() const { return m_artistCatalog; }

signals:
    void knownCatalogsChanged();

private slots:
    void checkSettingsChanged();
    void tracksAdded( const QList<unsigned int>& );
    void tracksRemoved( const QList<unsigned int>& );

    void loadedResults( const QList<Tomahawk::result_ptr>& results );

    // Echonest slots
    void songCreateFinished();
    void artistCreateFinished();
    void songUpdateFinished();
    void catalogDeleted();

    void checkTicket();

    void rawTracksAdd( const QList< QStringList >& tracks );
private:
    void uploadDb();
    QByteArray escape( const QString& in ) const;

    Echonest::CatalogUpdateEntry entryFromTrack( const QStringList&, Echonest::CatalogTypes::Action action ) const;
    void doUploadJob();

    bool m_syncing;

    Echonest::Catalog m_songCatalog;
    Echonest::Catalog m_artistCatalog;

    QQueue< Echonest::CatalogUpdateEntries > m_queuedUpdates;

    static EchonestCatalogSynchronizer* s_instance;

    friend class ::DatabaseCommand_SetCollectionAttributes;
};

}
#endif // ECHONESTCATALOGSYNCHRONIZER_H

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

#ifndef QUERY_H
#define QUERY_H

#include <QObject>
#include <QMutex>
#include <QList>
#include <QVariant>

#include "result.h"
#include "typedefs.h"

#include "dllmacro.h"

class DatabaseCommand_LogPlayback;
class DatabaseCommand_PlaybackHistory;
class DatabaseCommand_LoadPlaylistEntries;

namespace Tomahawk
{

class Resolver;

class DLLEXPORT Query : public QObject
{
Q_OBJECT

friend class ::DatabaseCommand_LogPlayback;
friend class ::DatabaseCommand_PlaybackHistory;
friend class ::DatabaseCommand_LoadPlaylistEntries;
friend class Pipeline;

public:
    static query_ptr get( const QString& artist, const QString& track, const QString& album, const QID& qid = QString(), bool autoResolve = true );
    static query_ptr get( const QString& query, const QID& qid );

    explicit Query( const QString& artist, const QString& track, const QString& album, const QID& qid, bool autoResolve );
    explicit Query( const QString& query, const QID& qid );

    virtual ~Query();

    /// returns list of all results so far
    QList< result_ptr > results() const;

    /// how many results found so far?
    unsigned int numResults() const;

    QID id() const;

    /// sorter for list of results
    static bool resultSorter( const result_ptr &left, const result_ptr& right );

    /// true when a perfect result has been found (score of 1.0)
    bool solved() const { return m_solved; }
    /// true when any result has been found (score may be less than 1.0)
    bool playable() const { return m_playable; }

    QString fullTextQuery() const { return m_fullTextQuery; }
    bool isFullTextQuery() const { return !m_fullTextQuery.isEmpty(); }
    bool resolvingFinished() const { return m_resolveFinished; }

    QPair< Tomahawk::source_ptr, unsigned int > playedBy() const { return m_playedBy; }
    Tomahawk::Resolver* currentResolver() const;
    QList< QWeakPointer< Tomahawk::Resolver > > resolvedBy() const { return m_resolvers; }

    void setArtist( const QString& artist ) { m_artist = artist; }
    void setAlbum( const QString& album ) { m_album = album; }
    void setTrack( const QString& track ) { m_track = track; }
    void setResultHint( const QString& resultHint ) { m_resultHint = resultHint; }
    void setDuration( int duration ) { m_duration = duration; }

    QVariant toVariant() const;
    QString toString() const;

    QString resultHint() const { return m_resultHint; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString track() const { return m_track; }
    int duration() const { return m_duration; }

    void setResolveFinished( bool resolved ) { m_resolveFinished = resolved; }
    void setPlayedBy( const Tomahawk::source_ptr& source, unsigned int playtime );

signals:
    void resultsAdded( const QList<Tomahawk::result_ptr>& );
    void resultsRemoved( const Tomahawk::result_ptr& );

    void resultsChanged();
    void solvedStateChanged( bool state );
    void resolvingFinished( bool hasResults );

public slots:
    /// (indirectly) called by resolver plugins when results are found
    void addResults( const QList< Tomahawk::result_ptr >& );
    void removeResult( const Tomahawk::result_ptr& );

    void onResolvingFinished();

    // resolve if not solved()
    void onResolverAdded();
    void onResolverRemoved();

private slots:
    void onResultStatusChanged();
    void refreshResults();

private:
    void setCurrentResolver( Tomahawk::Resolver* resolver );

    void clearResults();
    void checkResults();

    QList< Tomahawk::result_ptr > m_results;
    bool m_solved;
    bool m_playable;
    bool m_resolveFinished;
    mutable QID m_qid;

    QString m_artist;
    QString m_album;
    QString m_track;
    QString m_fullTextQuery;

    int m_duration;
    QString m_resultHint;

    QPair< Tomahawk::source_ptr, unsigned int > m_playedBy;
    QList< QWeakPointer< Tomahawk::Resolver > > m_resolvers;

    mutable QMutex m_mutex;
};

}; //ns

#endif // QUERY_H

/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#pragma once
#ifndef TOMAHAWK_QUERY_H
#define TOMAHAWK_QUERY_H

#include <QObject>
#include <QList>
#include <QVariant>

#include "DllMacro.h"
#include "Typedefs.h"


namespace Tomahawk
{

class DatabaseCommand_LoadPlaylistEntries;
class Resolver;
class QueryPrivate;

class DLLEXPORT Query : public QObject
{
Q_OBJECT

friend class DatabaseCommand_LoadPlaylistEntries;
friend class Pipeline;

public:
    static query_ptr get( const QString& artist, const QString& title, const QString& album, const QID& qid = QString(), bool autoResolve = true );
    static query_ptr get( const Tomahawk::track_ptr& track, const QID& qid = QString() );
    static query_ptr get( const QString& query, const QID& qid );

    /**
     * Get a Query object with a fixed Result reference which is not re-resolved.
     */
    static query_ptr getFixed( const Tomahawk::track_ptr& track, const Tomahawk::result_ptr& result );

    virtual ~Query();

    bool equals( const Tomahawk::query_ptr& other, bool ignoreCase = false, bool ignoreAlbum = false ) const;
    float howSimilar( const Tomahawk::result_ptr& r );

    QVariant toVariant() const;
    QString toString() const;

    QID id() const;

    track_ptr queryTrack() const;
    track_ptr track() const;

    /// returns list of all results so far
    QList< result_ptr > results() const;

    /// how many results found so far?
    unsigned int numResults( bool onlyPlayableResults = false ) const;

    bool resolvingFinished() const;
    /// true when a perfect result has been found (score of 1.0)
    bool solved() const;
    /// true when any result has been found (score may be less than 1.0)
    bool playable() const;

    float score() const;

    Tomahawk::Resolver* currentResolver() const;
    QList< QPointer< Tomahawk::Resolver > > resolvedBy() const;

    QString fullTextQuery() const;
    bool isFullTextQuery() const;

    void setResolveFinished( bool resolved );

    /**
     * Allow contacting the Pipeline if the state of this Query changes to
     * not solved.
     */
    void allowReresolve();

    /**
     * Disallow contacting the Pipeline if the state of this Query changes to
     * not solved.
     */
    void disallowReresolve();

    void setSaveHTTPResultHint( bool saveResultHint );
    bool saveHTTPResultHint() const;

    QString resultHint() const;
    void setResultHint( const QString& resultHint );

    QWeakPointer< Tomahawk::Query > weakRef();
    void setWeakRef( QWeakPointer< Tomahawk::Query > weakRef );

    /// sorter for list of results
    bool resultSorter( const result_ptr& left, const result_ptr& right );
    result_ptr preferredResult() const;
    void setPreferredResult( const result_ptr& result );

signals:
    void resultsAdded( const QList<Tomahawk::result_ptr>& );
    void resultsRemoved( const Tomahawk::result_ptr& );

    void albumsAdded( const QList<Tomahawk::album_ptr>& );
    void artistsAdded( const QList<Tomahawk::artist_ptr>& );

    void resultsChanged();
    void solvedStateChanged( bool state );
    void playableStateChanged( bool state );
    void resolvingFinished( bool hasResults );

public slots:
    /// (indirectly) called by resolver plugins when results are found
    void addResults( const QList< Tomahawk::result_ptr >& );
    void removeResult( const Tomahawk::result_ptr& );

    void addAlbums( const QList< Tomahawk::album_ptr >& );
    void addArtists( const QList< Tomahawk::artist_ptr >& );

    void onResolvingFinished();
    void onResolverAdded();

protected:
    QScopedPointer<QueryPrivate> d_ptr;

private slots:
    void onResultStatusChanged();
    void refreshResults();

private:
    explicit Query( const track_ptr& track, const QID& qid, bool autoResolve );
    /**
     * Respective constructor for getFixed
     */
    explicit Query( const track_ptr& track, const result_ptr& result );
    explicit Query( const QString& query, const QID& qid );

    Q_DECLARE_PRIVATE( Query )

    void init();

    void setCurrentResolver( Tomahawk::Resolver* resolver );
    void clearResults();
    void checkResults();
    void sortResults();
};

} //ns

Q_DECLARE_METATYPE( Tomahawk::query_ptr )

#endif // TOMAHAWK_QUERY_H

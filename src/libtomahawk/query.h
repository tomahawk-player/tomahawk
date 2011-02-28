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

class DLLEXPORT Query : public QObject
{
Q_OBJECT

friend class ::DatabaseCommand_LogPlayback;
friend class ::DatabaseCommand_PlaybackHistory;
friend class ::DatabaseCommand_LoadPlaylistEntries;

public:
    static query_ptr get( const QString& artist, const QString& track, const QString& album, const QID& qid = QString() );
    explicit Query( const QString& artist, const QString& track, const QString& album, const QID& qid );

    /// returns list of all results so far
    QList< result_ptr > results() const;

    /// how many results found so far?
    unsigned int numResults() const;

    QID id() const;

    /// sorter for list of results
    static bool resultSorter( const result_ptr &left, const result_ptr& right );

    /// solved=true when a perfect result has been found (score of 1.0)
    bool solved() const { return m_solved; }

    unsigned int lastPipelineWeight() const { return m_lastpipelineweight; }
    void setLastPipelineWeight( unsigned int w ) { m_lastpipelineweight = w; }

    void setArtist( const QString& artist ) { m_artist = artist; }
    void setAlbum( const QString& album ) { m_album = album; }
    void setTrack( const QString& track ) { m_track = track; }
    void setResultHint( const QString& resultHint ) { m_resultHint = resultHint; }
    void setDuration( int duration ) { m_duration = duration; }
//    void setQID( const QString& qid ) { m_qid = qid; }
    
    /// for debug output:
    QString toString() const
    {
        return QString( "Query(%1, %2 - %3)" ).arg( id() ).arg( artist() ).arg( track() );
    }

    QString resultHint() const { return m_resultHint; }
    QString artist() const { return m_artist; }
    QString album()  const { return m_album; }
    QString track()  const { return m_track; }
    int duration()   const { return m_duration; }
    
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

private slots:
    void onResultStatusChanged();
    void refreshResults();

private:
    void clearResults();
    void checkResults();
    
    mutable QMutex m_mut;
    QList< Tomahawk::result_ptr > m_results;
    bool m_solved;
    mutable QID m_qid;
    unsigned int m_lastpipelineweight;

    int m_duration;

    QString m_artist;
    QString m_album;
    QString m_track;

    QString m_resultHint;
};

}; //ns

#endif // QUERY_H

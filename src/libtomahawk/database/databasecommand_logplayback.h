#ifndef DATABASECOMMAND_LOGPLAYBACK_H
#define DATABASECOMMAND_LOGPLAYBACK_H

#include <QObject>
#include <QVariantMap>

#include "database/databasecommandloggable.h"
#include "sourcelist.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_LogPlayback : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QString artist READ artist WRITE setArtist )
Q_PROPERTY( QString track READ track WRITE setTrack )
Q_PROPERTY( unsigned int playtime READ playtime WRITE setPlaytime )
Q_PROPERTY( unsigned int secsPlayed READ secsPlayed WRITE setSecsPlayed )

public:
    explicit DatabaseCommand_LogPlayback( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_LogPlayback( const Tomahawk::result_ptr& result, unsigned int secsPlayed, QObject* parent = 0 )
        : DatabaseCommandLoggable( parent ), m_result( result ), m_secsPlayed( secsPlayed )
    {
        m_playtime = QDateTime::currentDateTimeUtc().toTime_t();
        setSource( SourceList::instance()->getLocal() );

        setArtist( result->artist()->name() );
        setTrack( result->track() );
    }

    virtual QString commandname() const { return "logplayback"; }

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return true; }
    virtual void postCommitHook();

    QString artist() const { return m_artist; }
    void setArtist( const QString& s ) { m_artist = s; }

    QString track() const { return m_track; }
    void setTrack( const QString& s ) { m_track = s; }

    unsigned int playtime() const { return m_playtime; }
    void setPlaytime( unsigned int i ) { m_playtime = i; }

    unsigned int secsPlayed() const { return m_secsPlayed; }
    void setSecsPlayed( unsigned int i ) { m_secsPlayed = i; }

private:
    Tomahawk::result_ptr m_result;

    QString m_artist;
    QString m_track;
    unsigned int m_playtime;
    unsigned int m_secsPlayed;
};

#endif // DATABASECOMMAND_LOGPLAYBACK_H

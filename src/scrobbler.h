
#ifndef TOMAHAWK_SCROBBLER_H
#define TOMAHAWK_SCROBBLER_H

#include "result.h"

#include <lastfm/Track>
#include <lastfm/Audioscrobbler>
#include <lastfm/ScrobblePoint>

#include <QObject>

class QNetworkReply;
/**
 * Simple class that listens to signals from AudioEngine and scrobbles
 *  what it is playing.
 */
class Scrobbler : public QObject
{
    Q_OBJECT
public:
    Scrobbler( QObject* parent = 0 );
    virtual ~Scrobbler();
    
public slots:
    void trackStarted( const Tomahawk::result_ptr& );
    void trackPaused();
    void trackResumed();
    void trackStopped();
    void engineTick( unsigned int secondsElapsed );
    
    void settingsChanged();
    void onAuthenticated();
    
private:
    void scrobble();
    void createScrobbler();
    
    lastfm::MutableTrack m_track;
    lastfm::Audioscrobbler* m_scrobbler;
    QString m_pw;
    bool m_reachedScrobblePoint;
    ScrobblePoint m_scrobblePoint;
    
    QNetworkReply* m_authJob;
};


#endif

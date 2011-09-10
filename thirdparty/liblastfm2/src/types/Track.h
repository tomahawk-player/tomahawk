/*
   Copyright 2009-2010 Last.fm Ltd.
      - Primarily authored by Max Howell, Jono Cole, Doug Mansell and Michael Coffey

   This file is part of liblastfm.

   liblastfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   liblastfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with liblastfm.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef LASTFM_TRACK_H
#define LASTFM_TRACK_H

#include <lastfm/AbstractType>
#include <lastfm/Album>
#include <lastfm/Mbid>
#include <QDateTime>
#include <QDomElement>
#include <QExplicitlySharedDataPointer>
#include <QString>
#include <QMap>
#include <QUrl>


namespace lastfm {

class LASTFM_DLLEXPORT TrackContext
{
public:
    enum Type
    {
        Unknown,
        User,
        Friend,
        Neighbour,
        Artist
    };

    TrackContext();
    TrackContext( const QString& type, const QList<QString>& values );

    Type type() const;
    QList<QString> values() const;
private:
    static Type getType( const QString& type );

private:
    Type m_type;
    QList<QString> m_values;
};

class TrackData : public QObject, public QSharedData
{
    Q_OBJECT

    friend class Track;
    friend class MutableTrack;
public:
    TrackData();

public:
    QString artist;
    QString albumArtist;
    QString album;
    QString title;
    QString correctedArtist;
    QString correctedAlbumArtist;
    QString correctedAlbum;
    QString correctedTitle;
    TrackContext context;
    uint trackNumber;
    uint duration;
    short source;
    short rating;
    QString mbid; /// musicbrainz id
    uint fpid;
    QUrl url;
    QDateTime time; /// the time the track was started at
    bool loved;
    QMap<lastfm::ImageSize, QUrl> m_images;
    short scrobbleStatus;
    short scrobbleError;
    QString scrobbleErrorText;

    //FIXME I hate this, but is used for radio trackauth etc.
    QMap<QString,QString> extras;
    
    bool null;

private:
    void forceLoveToggled( bool love ) { emit loveToggled( love );}
    void forceScrobbleStatusChanged() { emit scrobbleStatusChanged(); }
    void forceCorrected( QString correction ) { emit corrected( correction ); }

private slots:
    void onLoveFinished();
    void onUnloveFinished();
    void onGotInfo();

signals:
    void loveToggled( bool love );
    void loveFinished();
    void unlovedFinished();
    void gotInfo( const QByteArray& );
    void scrobbleStatusChanged();
    void corrected( QString correction );
};


/** Our track type. It's quite good, you may want to use it as your track type
  * in general. It is explicitly shared. Which means when you make a copy, they
  * both point to the same data still. This is like Qt's implicitly shared
  * classes, eg. QString, however if you mod a copy of a QString, the copy
  * detaches first, so then you have two copies. Our Track object doesn't
  * detach, which is very handy for our usage in the client, but perhaps not
  * what you want. If you need a deep copy for eg. work in a thread, call 
  * clone(). */
class LASTFM_DLLEXPORT Track : public AbstractType
{
public:
    friend class TrackSignalProxy;

    enum Source
    {
        // DO NOT UNDER ANY CIRCUMSTANCES CHANGE THE ORDER OR VALUES OF THIS ENUM!
        // you will cause broken settings and b0rked scrobbler cache submissions

        Unknown = 0,
        LastFmRadio,
        Player,
        MediaDevice,
        NonPersonalisedBroadcast, // eg Shoutcast, BBC Radio 1, etc.
        PersonalisedRecommendation, // eg Pandora, but not Last.fm
    };

    enum ScrobbleStatus
    {
        Null = 0,
        Cached,
        Submitted,
        Error
    };

    enum Corrections
    {
        Original = 0,
        Corrected
    };

    enum ScrobbleError
    {
        None = 0,
        FilteredArtistName = 113,
        FilteredTrackName = 114,
        FilteredAlbumName = 115,
        FilteredTimestamp = 116,
        ExceededMaxDailyScrobbles = 118,
        InvalidStreamAuth = 119
    };

    Track();
    explicit Track( const QDomElement& );

    /** this track and that track point to the same object, so they are the same
      * in fact. This doesn't do a deep data comparison. So even if all the 
      * fields are the same it will return false if they aren't in fact spawned
      * from the same initial Track object */
    bool sameObject( const Track& that )
    {
        return (this->d == that.d);
    }

    bool operator==( const Track& that ) const
    {
        return ( this->title() == that.title() &&
                 this->album() == that.album() &&
                 this->artist() == that.artist());
    }
    bool operator!=( const Track& that ) const
    {
        return !operator==( that );
    }

    QObject* signalProxy() const { return d.data(); }

    /** only a Track() is null */
    bool isNull() const { return d->null; }

    bool corrected() const;

    Artist artist( Corrections corrected = Original ) const;
    Artist albumArtist( Corrections corrected = Original ) const;
    Album album( Corrections corrected = Original ) const;
    QString title( Corrections corrected = Original ) const;

    uint trackNumber() const { return d->trackNumber; }
    uint duration() const { return d->duration; } /// in seconds
    Mbid mbid() const { return Mbid(d->mbid); }
    QUrl url() const { return d->url; }
    QDateTime timestamp() const { return d->time; }
    Source source() const { return static_cast<Source>(d->source); }
    uint fingerprintId() const { return d->fpid; }
    bool isLoved() const { return d->loved; }
    QUrl imageUrl( lastfm::ImageSize size, bool square ) const;

    QString durationString() const { return durationString( d->duration ); }
    static QString durationString( int seconds );

    ScrobbleStatus scrobbleStatus() const { return static_cast<ScrobbleStatus>(d->scrobbleStatus); }
    ScrobbleError scrobbleError() const { return static_cast<ScrobbleError>(d->scrobbleError); }
    QString scrobbleErrorText() const { return d->scrobbleErrorText; }

    /** default separator is an en-dash */
    QString toString() const { return toString( Corrected ); }
    QString toString( Corrections corrections ) const { return toString( QChar(8211), corrections );}
    QString toString( const QChar& separator, Corrections corrections = Original ) const;
    /** the standard representation of this object as an XML node */
    QDomElement toDomElement( class QDomDocument& ) const;

    TrackContext context() const { return d->context; }
    
    QString extra( const QString& key ) const{ return d->extras[ key ]; }

    bool operator<( const Track &that ) const
    {
        return this->d->time < that.d->time;
    }
    
    bool isMp3() const;
    
    operator QVariant() const { return QVariant::fromValue( *this ); }
    
//////////// lastfm::Ws
    
    /** See last.fm/api Track section */
    QNetworkReply* share( const QStringList& recipients, const QString& message = "", bool isPublic = true ) const;

    /** you can get any QNetworkReply TagList using Tag::list( QNetworkReply* ) */
    QNetworkReply* getTags() const; // for the logged in user
    QNetworkReply* getTopTags() const;
    QNetworkReply* getTopFans() const;
    void getInfo() const;
    QNetworkReply* getBuyLinks( const QString& country ) const;

    /** you can only add 10 tags, we submit everything you give us, but the
      * docs state 10 only. Will return 0 if the list is empty. */
    QNetworkReply* addTags( const QStringList& ) const;
    /** will return 0 if the string is "" */
    QNetworkReply* removeTag( const QString& ) const;

    /** scrobble the track */
    QNetworkReply* updateNowPlaying() const;
    QNetworkReply* updateNowPlaying( int duration ) const;
    QNetworkReply* removeNowPlaying() const;
    QNetworkReply* scrobble() const;
    static QNetworkReply* scrobble(const QList<lastfm::Track>& tracks);

    /** the url for this track's page at last.fm */
    QUrl www() const;

protected:
    QExplicitlySharedDataPointer<TrackData> d;
    QMap<QString, QString> params( const QString& method, bool use_mbid = false ) const;
    void invalidateGetInfo();
private:
    Track( TrackData* that_d ) : d( that_d )
    {}
};



/** This class allows you to change Track objects, it is easy to use:
  * MutableTrack( some_track_object ).setTitle( "Arse" );
  *
  * We have a separate MutableTrack class because in our usage, tracks
  * only get mutated once, and then after that, very rarely. This pattern
  * encourages such usage, which is generally sensible. You can feel more
  * comfortable that the data hasn't accidently changed behind your back.
  */
class LASTFM_DLLEXPORT MutableTrack : public Track
{
public:
    MutableTrack()
    {
        d->null = false;
    }

    /** NOTE that passing a Track() to this ctor will automatically make it non
      * null. Which may not be what you want. So be careful
      * Rationale: this is the most maintainable way to do it 
      */
    MutableTrack( const Track& that ) : Track( that )
    {
        d->null = false;
    }

    void setFromLfm( const XmlQuery& lfm );
    void setImageUrl( lastfm::ImageSize size, const QString& url );
    
    void setArtist( QString artist ) { d->artist = artist.trimmed(); }
    void setAlbumArtist( QString albumArtist ) { d->albumArtist = albumArtist.trimmed(); }
    void setAlbum( QString album ) { d->album = album.trimmed(); }
    void setTitle( QString title ) { d->title = title.trimmed(); }
    void setCorrections( QString title, QString album, QString artist, QString albumArtist );
    void setTrackNumber( uint n ) { d->trackNumber = n; }
    void setDuration( uint duration ) { d->duration = duration; }
    void setUrl( QUrl url ) { d->url = url; }
    void setSource( Source s ) { d->source = s; }
    void setLoved( bool loved ) { d->loved = loved; }
    
    void setMbid( Mbid id ) { d->mbid = id; }
    void setFingerprintId( uint id ) { d->fpid = id; }

    void setScrobbleStatus( ScrobbleStatus scrobbleStatus )
    {
        d->scrobbleStatus = scrobbleStatus;
        d->forceScrobbleStatusChanged();
    }
    void setScrobbleError( ScrobbleError scrobbleError ) { d->scrobbleError = scrobbleError; }
    void setScrobbleErrorText( const QString& scrobbleErrorText ) { d->scrobbleErrorText = scrobbleErrorText; }
    
    /** you also must scrobble this track for the love to become permenant */
    void love();
    void unlove();
    QNetworkReply* ban();
    
    void stamp() { d->time = QDateTime::currentDateTime(); }

    void setExtra( const QString& key, const QString& value ) { d->extras[key] = value; }
    void removeExtra( QString key ) { d->extras.remove( key ); }
    void setTimeStamp( const QDateTime& dt ) { d->time = dt; }

    void setContext( TrackContext context ) { d->context = context;}
};


} //namespace lastfm


inline QDebug operator<<( QDebug d, const lastfm::Track& t )
{
    return !t.isNull() 
            ? d << t.toString( '-' ) << t.url()
            : d << "Null Track object";
}


Q_DECLARE_METATYPE( lastfm::Track );

#endif //LASTFM_TRACK_H

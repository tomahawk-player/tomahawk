#ifndef RESULT_H
#define RESULT_H

#include <qvariant.h>

#include <QObject>
#include <QVariant>

#include "typedefs.h"

#include "dllmacro.h"

class DatabaseCommand_Resolve;
class DatabaseCommand_AllTracks;
class DatabaseCommand_AddFiles;
class DatabaseCommand_LoadFile;

namespace Tomahawk
{

class DLLEXPORT Result : public QObject
{
Q_OBJECT

friend class ::DatabaseCommand_Resolve;
friend class ::DatabaseCommand_AllTracks;
friend class ::DatabaseCommand_AddFiles;
friend class ::DatabaseCommand_LoadFile;

public:
    explicit Result();
    virtual ~Result();

    QVariant toVariant() const { return m_v; }
    QString toString() const;
    Tomahawk::query_ptr toQuery() const;

    float score() const;
    RID id() const;
    collection_ptr collection() const;
    Tomahawk::artist_ptr artist() const;
    Tomahawk::album_ptr album() const;
    QString track()     const { return m_track; }
    QString url()       const { return m_url; }
    QString mimetype()  const { return m_mimetype; }

    unsigned int duration() const { return m_duration; }
    unsigned int bitrate() const { return m_bitrate; }
    unsigned int size() const { return m_size; }
    unsigned int albumpos() const { return m_albumpos; }
    unsigned int modificationTime() const { return m_modtime; }
    int year() const { return m_year; }

    void setScore( float score ) { m_score = score; }
    void setId( unsigned int id ) { m_id = id; }
    void setRID( RID id ) { m_rid = id; }
    void setCollection( const Tomahawk::collection_ptr& collection );
    void setArtist( const Tomahawk::artist_ptr& artist );
    void setAlbum( const Tomahawk::album_ptr& album );
    void setTrack( const QString& track ) { m_track = track; }
    void setUrl( const QString& url ) { m_url = url; }
    void setMimetype( const QString& mimetype ) { m_mimetype = mimetype; }
    void setDuration( unsigned int duration ) { m_duration = duration; }
    void setBitrate( unsigned int bitrate ) { m_bitrate = bitrate; }
    void setSize( unsigned int size ) { m_size = size; }
    void setAlbumPos( unsigned int albumpos ) { m_albumpos = albumpos; }
    void setModificationTime( unsigned int modtime ) { m_modtime = modtime; }
    
    QVariantMap attributes() const { return m_attributes; }
    void setAttributes( const QVariantMap& map ) { m_attributes = map; updateAttributes(); }

    unsigned int dbid() const { return m_id; }

signals:
    // emitted when the collection this result comes from is going offline/online:
    void statusChanged();
    
private slots:
    void onOffline();
    void onOnline();

private:
    void updateAttributes();

    QVariant m_v;
    mutable RID m_rid;
    collection_ptr m_collection;

    Tomahawk::artist_ptr m_artist;
    Tomahawk::album_ptr m_album;
    QString m_track;
    QString m_url;
    QString m_mimetype;

    unsigned int m_duration;
    unsigned int m_bitrate;
    unsigned int m_size;
    unsigned int m_albumpos;
    unsigned int m_modtime;
    int m_year;
    float m_score;

    QVariantMap m_attributes;

    unsigned int m_id;
};

}; //ns

#endif // RESULT_H

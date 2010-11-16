#ifndef RESULT_H
#define RESULT_H

#include <QObject>

#include "collection.h"
#include "tomahawk/typedefs.h"

namespace Tomahawk
{

class Result : public QObject
{
Q_OBJECT

public:
    explicit Result( const QVariant& v, const collection_ptr& collection );
    QVariant toVariant() const { return m_v; }

    float score() const;
    RID id() const;
    collection_ptr collection() const { return m_collection; }

    QString artist()   const { return m_artist; }
    QString album()    const { return m_album; }
    QString track()    const { return m_track; }
    QString url()      const { return m_url; }
    QString mimetype() const { return m_mimetype; }

    unsigned int duration() const { return m_duration; }
    unsigned int bitrate() const { return m_bitrate; }
    unsigned int size() const { return m_size; }
    unsigned int albumpos() const { return m_albumpos; }
    unsigned int modificationTime() const { return m_modtime; }

    // for debug output:
    QString toString() const
    {
        return QString( "Result(%1 %2\t%3 - %4  %5" ).arg( id() ).arg( score() ).arg( artist() ).arg( track() ).arg( url() );
    }

signals:
    // emitted when the collection this result comes from is going offline:
    void becomingUnavailable();

private:
    QVariant m_v;
    mutable RID m_rid;
    collection_ptr m_collection;

    QString m_artist;
    QString m_album;
    QString m_track;
    QString m_url;
    QString m_mimetype;

    unsigned int m_duration;
    unsigned int m_bitrate;
    unsigned int m_size;
    unsigned int m_albumpos;
    unsigned int m_modtime;
};

}; //ns

#endif // RESULT_H

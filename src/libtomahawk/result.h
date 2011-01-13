
#ifndef RESULT_H
#define RESULT_H
#include <qvariant.h>

#include <QObject>
#include <QVariant>

#include "typedefs.h"

#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT Result : public QObject
{
Q_OBJECT

public:
	Result();
    explicit Result( const QVariant& v, const collection_ptr& collection );
    virtual ~Result();

    QVariant toVariant() const { return m_v; }
    QString toString() const;
    Tomahawk::query_ptr toQuery() const;

    float score() const;
    RID id() const;
    collection_ptr collection() const;
    Tomahawk::artist_ptr artist() const;
    Tomahawk::album_ptr album()   const;
    QString track()     const { return m_track; }
    QString url()       const { return m_url; }
    QString mimetype()  const { return m_mimetype; }

    unsigned int duration() const { return m_duration; }
    unsigned int bitrate() const { return m_bitrate; }
    unsigned int size() const { return m_size; }
    unsigned int albumpos() const { return m_albumpos; }
    unsigned int modificationTime() const { return m_modtime; }
    int year() const { return m_year; }

    QVariantMap attributes() const { return m_attributes; }
    void setAttributes( const QVariantMap& map ) { m_attributes = map; updateAttributes(); }

    unsigned int dbid() const { return m_id; }

signals:
    // emitted when the collection this result comes from is going offline:
    void becomingUnavailable();

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

    QVariantMap m_attributes;

    unsigned int m_id;
};

}; //ns

#endif // RESULT_H

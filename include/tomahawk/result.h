#ifndef RESULT_H
#define RESULT_H

#include <QObject>

#include "collection.h"
#include "source.h"
#include "tomahawk/typedefs.h"

namespace Tomahawk
{

class Result : public QObject
{
Q_OBJECT

public:
    explicit Result( QVariant v, collection_ptr collection );
    QVariant toVariant() const { return m_v; }

    float score() const;
    RID id() const;
    collection_ptr collection() const { return m_collection; }

    QString artist()   const { return m_v.toMap().value( "artist" ).toString(); }
    QString album()    const { return m_v.toMap().value( "album" ).toString(); }
    QString track()    const { return m_v.toMap().value( "track" ).toString(); }
    QString url()      const { return m_v.toMap().value( "url" ).toString(); }
    QString mimetype() const { return m_v.toMap().value( "mimetype" ).toString(); }

    unsigned int duration() const { return m_v.toMap().value( "duration" ).toUInt(); }
    unsigned int bitrate() const { return m_v.toMap().value( "bitrate" ).toUInt(); }
    unsigned int size() const { return m_v.toMap().value( "size" ).toUInt(); }
    unsigned int albumpos() const { return m_v.toMap().value( "albumpos" ).toUInt(); }

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
};

}; //ns

#endif // RESULT_H

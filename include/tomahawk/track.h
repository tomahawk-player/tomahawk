#ifndef TOMAHAWKTRACK_H
#define TOMAHAWKTRACK_H

#include <QObject>
#include <QSharedPointer>

#include "tomahawk/artist.h"
#include "tomahawk/typedefs.h"

namespace Tomahawk
{

class Track : public QObject
{
Q_OBJECT

public:
    Track( Tomahawk::artist_ptr artist, const QString& name )
        : m_name( name )
        , m_artist( artist )
    {}

    const QString& name() const { return m_name; }
    const artist_ptr artist() const { return m_artist; }

private:
    QString m_name;
    artist_ptr m_artist;
};

}; // ns

#endif

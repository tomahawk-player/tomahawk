#ifndef PLUGINAPI_H
#define PLUGINAPI_H

#include <QObject>
#include <QSharedPointer>

#include "tomahawk/collection.h"
#include "tomahawk/source.h"

/*
    This is the only API plugins have access to.
    This class must proxy calls to internal functions, because plugins can't
    get a pointer to any old object and start calling methods on it.
*/

namespace Tomahawk
{
class Resolver;
class Pipeline;

class PluginAPI : public QObject
{
Q_OBJECT

public:
    explicit PluginAPI( Pipeline * p );

    /// call every time new results are available for a running query
//    void reportResults( const QString& qid, const QList<QVariantMap>& results );

    /// add/remove sources (which have collections)
    void addSource( source_ptr s );
    void removeSource( source_ptr s );

    /// register object capable of searching
    void addResolver( Resolver * r );

    Pipeline * pipeline() const { return m_pipeline; }

private:
    Pipeline * m_pipeline;
};


}; //ns

#endif // PLUGINAPI_H

#ifndef RESOLVER_H
#define RESOLVER_H

#include <QObject>

#include "pluginapi.h"

#include "dllmacro.h"

// implement this if you can resolve queries to content

/*
    Weight: 1-100, 100 being the best
    Timeout: some millisecond value, after which we try the next highest
             weighted resolver

*/
namespace Tomahawk
{
class PluginAPI;

class DLLEXPORT Resolver : public QObject
{
Q_OBJECT

public:
    Resolver() {}

    virtual QString name() const = 0;
    virtual unsigned int weight() const = 0;
    virtual unsigned int preference() const { return 100; };
    virtual unsigned int timeout() const = 0;

    //virtual QWidget * configUI() { return 0; };
    //etc

    PluginAPI * api() const { return m_api; }

public slots:
    virtual void resolve( const Tomahawk::query_ptr& query ) = 0;

private:
    PluginAPI * m_api;
};

class DLLEXPORT ExternalResolver : public Resolver
{
Q_OBJECT

public:
    ExternalResolver( const QString& filePath ) { m_filePath = filePath; }

    virtual QString filePath() const { return m_filePath; }

public slots:
    virtual void stop() = 0;

private:
    QString m_filePath;
};

}; //ns

#endif // RESOLVER_H

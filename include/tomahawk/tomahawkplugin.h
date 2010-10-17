#ifndef TOMAHAWK_PLUGIN_H
#define TOMAHAWK_PLUGIN_H
#include <QString>
#include <QtPlugin>

#include "tomahawk/pluginapi.h"

class TomahawkPlugin
{
public:
    TomahawkPlugin(){};
    TomahawkPlugin(Tomahawk::PluginAPI * api)
        : m_api(api) {};

    virtual TomahawkPlugin * factory(Tomahawk::PluginAPI * api) = 0;

    virtual QString name() const = 0;
    virtual QString description() const = 0;

protected:
    Tomahawk::PluginAPI * api() const { return m_api; };

private:
    Tomahawk::PluginAPI * m_api;

};

Q_DECLARE_INTERFACE(TomahawkPlugin, "org.tomahawk.TomahawkPlugin/1.0")

#endif

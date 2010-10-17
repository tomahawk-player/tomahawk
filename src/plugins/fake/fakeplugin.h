#ifndef TOMAHAWK_LIB_PLUGIN_H
#define TOMAHAWK_LIB_PLUGIN_H
#include <QtPlugin>

#include "tomahawk/plugin_includes.h"

class FakePlugin : public QObject, public TomahawkPlugin
{
    Q_OBJECT
    Q_INTERFACES(TomahawkPlugin)

public:

    FakePlugin(){};

    FakePlugin(Tomahawk::PluginAPI* api);
    TomahawkPlugin * factory(Tomahawk::PluginAPI* api);
    QString name() const { return "FakePlugin"; };
    QString description() const { return "Fake stuff, hardcoded"; };

private:

    void init();
    
    Tomahawk::PluginAPI* m_api;
};



#endif


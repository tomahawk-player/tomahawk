#include "fakeplugin.h"
#include "fakecollection.h"

Q_EXPORT_PLUGIN2(fake, FakePlugin)

FakePlugin::FakePlugin(Tomahawk::PluginAPI* api)
    : TomahawkPlugin(api), m_api(api)
{
    init();
}

TomahawkPlugin * 
FakePlugin::factory(Tomahawk::PluginAPI* api)
{
    return new FakePlugin(api);
}

void FakePlugin::init()
{
    source_ptr src(new Source("Mr. Fake"));
    collection_ptr coll(new FakeCollection);
    src->addCollection(coll);
    m_api->addSource(src);
    coll->load();
};


#include "dynamic/GeneratorFactory.h"
#include "dynamic/GeneratorInterface.h"

using namespace Tomahawk;

QHash< QString, GeneratorFactoryInterface* > GeneratorFactory::s_factories = QHash< QString, GeneratorFactoryInterface* >();

geninterface_ptr GeneratorFactory::create ( const QString& type )
{
    if( type.isEmpty() && !s_factories.isEmpty() ) // default, return first
        return geninterface_ptr( s_factories.begin().value()->create() );
    
    if( !s_factories.contains( type ) )
        return geninterface_ptr();
    
    return geninterface_ptr( s_factories.value( type )->create() );
}

void GeneratorFactory::registerFactory ( const QString& type, GeneratorFactoryInterface* interface )
{
    s_factories.insert( type, interface );
}

QStringList GeneratorFactory::types()
{
    return s_factories.keys();
}

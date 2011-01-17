#include "dynamic/GeneratorFactory.h"
#include "dynamic/GeneratorInterface.h"

using namespace Tomahawk;

QHash< QString, GeneratorFactoryInterface* > GeneratorFactory::s_factories = QHash< QString, GeneratorFactoryInterface* >();

geninterface_ptr 
GeneratorFactory::create ( const QString& type )
{
    if( type.isEmpty() && !s_factories.isEmpty() ) // default, return first
        return geninterface_ptr( s_factories.begin().value()->create() );
    
    if( !s_factories.contains( type ) )
        return geninterface_ptr();
    
    return geninterface_ptr( s_factories.value( type )->create() );
}

dyncontrol_ptr 
GeneratorFactory::createControl(const QString& generatorType, const QString& controlType)
{
    if( generatorType.isEmpty() || !s_factories.contains( generatorType ) )
        return dyncontrol_ptr();
    
    return s_factories.value( generatorType )->createControl( controlType );
}


void 
GeneratorFactory::registerFactory ( const QString& type, GeneratorFactoryInterface* interface )
{
    s_factories.insert( type, interface );
}

QStringList 
GeneratorFactory::types()
{
    return s_factories.keys();
}

QStringList 
GeneratorFactory::typeSelectors(const QString& type)
{
    if( !s_factories.contains( type ) )
        return QStringList();
    
    return s_factories.value( type )->typeSelectors();
}

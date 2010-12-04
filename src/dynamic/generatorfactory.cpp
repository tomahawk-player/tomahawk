#include "dynamic/generatorfactory.h"
#include "dynamic/generatorinterface.h"

Tomahawk::GeneratorFactory::GeneratorFactory()
{
}

Tomahawk::GeneratorFactory::~GeneratorFactory()
{
    qDeleteAll( m_factories.values() );
}

generatorinterface_ptr Tomahawk::GeneratorFactory::create ( const QString& type )
{
    if( !m_factories.contains( type ) )
        return geninterface_ptr();
    
    return geninterface_ptr( m_factories.value( type )->create() );
}

void Tomahawk::GeneratorFactory::registerFactory ( const QString& type, Tomahawk::GeneratorFactoryInterface* interface )
{
    m_factories.insert( type, interface );
}

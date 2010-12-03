#ifndef GENERATOR_FACTORY_H
#define GENERATOR_FACTORY_H

#include <QHash>
#include <QString>

namespace Tomahawk {
    
/**
  * Generators should subclass this and have it create the custom Generator
  */
class GeneratorFactoryInterface
{
public:
    GeneratorInterface* create() = 0;
};

/**
 * Simple factory that generates Generators from string type descriptors
 */
class GeneratorFactory
{
public:
    GeneratorFactory();
    ~GeneratorFactory();
    
    generatorinterface_ptr create( const QString& type );
    
    void registerFactory( const QString& type, GeneratorFactoryInterface* interface );
    
private:
    QHash<QString, GeneratorFactoryInterface*> m_factories;
    
};


};

#endif

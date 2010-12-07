#ifndef GENERATOR_FACTORY_H
#define GENERATOR_FACTORY_H

#include <QHash>
#include <QString>

#include "dynamic/generatorinterface.h"

namespace Tomahawk {
    
/**
  * Generators should subclass this and have it create the custom Generator
  */
class GeneratorFactoryInterface
{
public:
    GeneratorFactoryInterface() {}
    
    virtual GeneratorInterface* create() = 0;
};

/**
 * Simple factory that generates Generators from string type descriptors
 */
class GeneratorFactory
{
public:
    static geninterface_ptr create( const QString& type );
    static void registerFactory( const QString& type, GeneratorFactoryInterface* interface );
    
private:
    static QHash<QString, GeneratorFactoryInterface*> s_factories;
    
};


};

#endif

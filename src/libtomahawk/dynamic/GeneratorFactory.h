#ifndef GENERATOR_FACTORY_H
#define GENERATOR_FACTORY_H

#include <QHash>
#include <QString>

#include "dynamic/GeneratorInterface.h"
#include "typedefs.h"

namespace Tomahawk {
    
/**
  * Generators should subclass this and have it create the custom Generator
  */
class GeneratorFactoryInterface
{
public:
    GeneratorFactoryInterface() {}
    
    virtual GeneratorInterface* create() = 0;
    /**
     * Create a control for this generator, not tied to this generator itself. Used when loading dynamic
     *  playlists from a dbcmd.
     */
    virtual dyncontrol_ptr createControl( const QString& controlType = QString() ) = 0;
    
    virtual QStringList typeSelectors() const = 0;
};

/**
 * Simple factory that generates Generators from string type descriptors
 */
class GeneratorFactory
{
public:
    static geninterface_ptr create( const QString& type );
    // only used when loading from dbcmd
    static dyncontrol_ptr createControl( const QString& generatorType, const QString& controlType = QString() );
    
    static void registerFactory( const QString& type, GeneratorFactoryInterface* interface );
    static QStringList types();
    static QStringList typeSelectors( const QString& type );
    
private:
    static QHash<QString, GeneratorFactoryInterface*> s_factories;
    
};


};

#endif

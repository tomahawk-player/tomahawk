#ifndef GENERATOR_INTERFACE_H
#define GENERATOR_INTERFACE_H

#include <QtCore/QObject>

namespace Tomahawk {

enum GeneratorMode {
    OnDemand = 0,
    Static
};
    
/**
 * The abstract interface for Dynamic Playlist Generators. Generators have the following features:
 *      - They create new DynamicControls that are appropriate for the generator
 *      - They expose a list of controls that this generator currently is operating on
 *      - They have a state of OnDemand or Static
 */
class GeneratorInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString         type READ type )
    Q_PROPERTY( GeneratorMode   mode READ mode WRITE setMode );
    Q_ENUMS( GeneratorMode )
    
public:
    explicit GeneratorInterface() {}
    virtual ~GeneratorInterface() {}
    
    // Can't make it pure otherwise we can't shove it in QVariants :-/
    virtual dyncontrol_ptr createControl() const {}
    
    /// The type of this generator
    virtual QString type() const { return m_type; }
    
    virtual GeneratorMode mode() const { return m_mode; }
    virtual void setMode( GeneratorMode mode ) { m_mode = mode; }
    
    // control functions
    virtual QList< dyncontrol_ptr > controls() const { return m_controls; }
    virtual void addControl( const dyncontrol_ptr& control ) { m_controls << control; }
    virtual void clearControls() { m_controls.clear(); }
    virtual void setControls( const QList< dyncontrol_ptr>& controls ) { m_controls = controls; }
private:
    QString m_type;
    GeneratorMode m_mode;
    QList< dyncontrol_ptr > m_controls;
};

};

#endif

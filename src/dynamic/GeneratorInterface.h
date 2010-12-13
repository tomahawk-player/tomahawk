/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef GENERATOR_INTERFACE_H
#define GENERATOR_INTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QStringList>

#include "tomahawk/typedefs.h"
#include "tomahawk/query.h"
#include "dynamic/DynamicControl.h"

namespace Tomahawk {
   
/**
 * The abstract interface for Dynamic Playlist Generators. Generators have the following features:
 *      - They create new DynamicControls that are appropriate for the generator
 *      - They expose a list of controls that this generator currently is operating on
 *      - They have a mode of OnDemand or Static
 * 
 *  And they generate tracks
 */
class GeneratorInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString         type READ type )
    Q_PROPERTY( GeneratorMode   mode READ mode WRITE setMode );
    
public:
    // can't inline constructors/destructors for forward declared shared pointer types
    GeneratorInterface();
    explicit GeneratorInterface( QObject* parent = 0 );
    virtual ~GeneratorInterface();
    
    // Can't make it pure otherwise we can't shove it in QVariants :-/
    // empty QString means use default
    /// The generator will keep track of all the controls it creates. No need to tell it about controls
    ///  you ask it to create
    virtual dyncontrol_ptr createControl( const QString& type = QString() );
    
    /**
     * Generate tracks from the controls in this playlist. If the current mode is
     *  OnDemand, then \p number is not taken into account. If this generator is in static
     *  mode, then it will return the desired number of tracks
     * 
     * Connect to the generated() signal for the results.
     * 
     */
    virtual void generate( int number = -1 ) {}
    
    /// The type of this generator
    QString type() const { return m_type; }
    
    GeneratorMode mode() const { return m_mode; }
    void setMode( GeneratorMode mode ) { m_mode = mode; }
    
    // control functions
    QList< dyncontrol_ptr > controls();
    void addControl( const dyncontrol_ptr& control );
    void clearControls();
    void setControls( const QList< dyncontrol_ptr>& controls );
    
    QStringList typeSelectors() const { return m_typeSelectors; }
    
signals:
    void generated( const QList< Tomahawk::query_ptr>& queries );
    
protected:
    QString m_type;
    GeneratorMode m_mode;
    QList< dyncontrol_ptr > m_controls;
    QStringList m_typeSelectors;
};

typedef QSharedPointer<GeneratorInterface> geninterface_ptr;

};

#endif

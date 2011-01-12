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

#include "dynamic/GeneratorInterface.h"

// lame
Tomahawk::GeneratorInterface::GeneratorInterface()
{

}

Tomahawk::GeneratorInterface::GeneratorInterface( QObject* parent ): QObject( parent )
{

}

Tomahawk::GeneratorInterface::~GeneratorInterface()
{

}

QList< Tomahawk::dyncontrol_ptr > 
Tomahawk::GeneratorInterface::controls()
{
//     if( m_controls.isEmpty() ) { // return a default control (so the user can add more)
//         return QList< Tomahawk::dyncontrol_ptr >() << createControl();
//     }
    
    return m_controls;
}

QPixmap 
Tomahawk::GeneratorInterface::logo()
{
    return QPixmap();
}

void 
Tomahawk::GeneratorInterface::addControl( const Tomahawk::dyncontrol_ptr& control )
{
    m_controls << control;
}

void 
Tomahawk::GeneratorInterface::clearControls()
{
    m_controls.clear();
}

void 
Tomahawk::GeneratorInterface::setControls( const QList< Tomahawk::dyncontrol_ptr >& controls )
{
    m_controls = controls;
}

void Tomahawk::GeneratorInterface::removeControl(const Tomahawk::dyncontrol_ptr& control)
{
    m_controls.removeAll( control );
}


Tomahawk::dyncontrol_ptr 
Tomahawk::GeneratorInterface::createControl(const QString& type)
{
    Q_ASSERT( false );
    return dyncontrol_ptr();
}

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

#ifndef ECHONEST_GENERATOR_H
#define ECHONEST_GENERATOR_H

#include <echonest/Playlist.h>

#include "dynamic/generatorinterface.h"
#include "dynamic/generatorfactory.h"


namespace Tomahawk 
{

class EchonestFactory : public GeneratorFactoryInterface
{
public:
    EchonestFactory();
    
    virtual GeneratorInterface* create();
    
};
    
class EchonestGenerator : public GeneratorInterface
{
    Q_OBJECT
public:
    explicit EchonestGenerator( QObject* parent );
    virtual ~EchonestGenerator();
    
    virtual dyncontrol_ptr createControl( const QString& type = QString() ) const;
    
    virtual void generate ( int number = -1 );
    
private slots:
    void staticFinished();
};

};

#endif

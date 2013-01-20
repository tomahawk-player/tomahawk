/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */


#include "ScriptCollection.h"

#include "Source.h"

using namespace Tomahawk;


ScriptCollection::ScriptCollection( const source_ptr& source,
                                    QtScriptResolver* resolver,
                                    QObject* parent )
    : Collection( source, resolver->name(), parent )
{
    Q_ASSERT( resolver != 0 );
    qDebug() << Q_FUNC_INFO << resolver->name() << source->friendlyName();

    m_resolver = resolver;
}


ScriptCollection::~ScriptCollection()
{

}

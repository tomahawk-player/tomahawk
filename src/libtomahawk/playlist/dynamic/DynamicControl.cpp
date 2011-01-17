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

#include "DynamicControl.h"

Tomahawk::DynamicControl::DynamicControl( const QStringList& typeSelectors )
    : m_typeSelectors( typeSelectors )
{

}

Tomahawk::DynamicControl::~DynamicControl()
{

}

Tomahawk::DynamicControl::DynamicControl(const QString& selectedType, const QStringList& typeSelectors, QObject* parent)
    : QObject(parent)
    , m_selectedType( selectedType )
    , m_typeSelectors( typeSelectors )
{

}

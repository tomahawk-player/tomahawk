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

#include "echonest/EchonestControl.h"

#include <echonest/Playlist.h>

#include <QComboBox>
#include <QLineEdit>


Tomahawk::EchonestControl::EchonestControl( const QString& type, QObject* parent )
    : DynamicControl ( type, parent )
{
    updateWidgets();
}

QWidget*
Tomahawk::EchonestControl::inputField()
{
    return m_input.data();
}

QWidget*
Tomahawk::EchonestControl::matchSelector()
{
    return m_match.data();
}

void 
Tomahawk::EchonestControl::setSelectedType ( const QString& type )
{
    if( !m_input.isNull() )
        delete m_input.data();
    if( !m_match.isNull() )
        delete m_match.data();
    
    Tomahawk::DynamicControl::setSelectedType ( type );
    updateWidgets();
}

Echonest::DynamicPlaylist::PlaylistParamData 
Tomahawk::EchonestControl::toENParam() const
{
    return m_data;
}

void 
Tomahawk::EchonestControl::updateWidgets()
{
    // make sure the widgets are the proper kind for the selected type, and hook up to their slots
    if( selectedType() == "Artist" ) {
        QComboBox* match = new QComboBox();
        QLineEdit* input =  new QLineEdit();
        
        match->addItem( "Limit To", Echonest::DynamicPlaylist::ArtistType );
        match->addItem( "Similar To", Echonest::DynamicPlaylist::ArtistRadioType );
        
        input->setPlaceholderText( "Artist name" );
        
        connect( match, SIGNAL( currentIndexChanged(int) ), this, SLOT( updateData() ) );
        connect( input, SIGNAL( textChanged(QString) ), this, SLOT( updateData() ) );
        
        m_match = QWeakPointer< QWidget >( match );
        m_input = QWeakPointer< QWidget >( input );
    }
}

void 
Tomahawk::EchonestControl::updateData()
{
    if( selectedType() == "Artist" ) {
        QWeakPointer<QComboBox> combo = qWeakPointerCast<QComboBox, QWidget>( m_match );
        if( !combo.isNull() )
            m_data.first = static_cast<Echonest::DynamicPlaylist::PlaylistParam>( combo.data()->itemData( combo.data()->currentIndex() ).toInt() );
        QWeakPointer<QLineEdit> edit = qWeakPointerCast<QLineEdit, QWidget>( m_input );
        if( !edit.isNull() )
            m_data.second = edit.data()->text();
    }
}

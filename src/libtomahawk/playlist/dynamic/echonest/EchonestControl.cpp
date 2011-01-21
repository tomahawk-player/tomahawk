/****************************************************************************************
 * Copyright (c) 2010-2011 Leo Franchi <lfranchi@kde.org>                               *
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

#include "dynamic/echonest/EchonestControl.h"

#include "dynamic/widgets/MiscControlWidgets.h"

#include <echonest/Playlist.h>

#include <QComboBox>
#include <QLineEdit>
#include <QLabel>


Tomahawk::EchonestControl::EchonestControl( const QString& selectedType, const QStringList& typeSelectors, QObject* parent )
    : DynamicControl ( selectedType.isEmpty() ? "Artist" : selectedType, typeSelectors, parent )
{
    setType( "echonest" );
    m_editingTimer.setInterval( 3000 ); // 3 second timeout to edits
    m_editingTimer.setSingleShot( true );
    
    connect( &m_editingTimer, SIGNAL( timeout() ), this, SIGNAL( changed() ) );
    updateWidgets();
    updateData();
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
    if( type != selectedType() ) {
        if( !m_input.isNull() )
            delete m_input.data();
        if( !m_match.isNull() )
            delete m_match.data();
        
        Tomahawk::DynamicControl::setSelectedType ( type );
        updateWidgets();
    }
}

Echonest::DynamicPlaylist::PlaylistParamData
Tomahawk::EchonestControl::toENParam() const
{
    if( m_overrideType != -1 ) {
        Echonest::DynamicPlaylist::PlaylistParamData newData = m_data;
        newData.first = static_cast<Echonest::DynamicPlaylist::PlaylistParam>( m_overrideType );
        return newData;
    }
    return m_data;
}

QString Tomahawk::EchonestControl::input() const
{
    return m_data.second.toString();
}

QString Tomahawk::EchonestControl::match() const
{
    return m_matchData;
}

QString Tomahawk::EchonestControl::matchString()
{
    return m_matchString;
}


void Tomahawk::EchonestControl::setInput(const QString& input)
{
    // TODO generate widgets
    m_data.second = input;
    updateWidgetsFromData();
}

void Tomahawk::EchonestControl::setMatch(const QString& match)
{
    // TODO generate widgets
    m_matchData = match;
    updateWidgetsFromData();
}

void 
Tomahawk::EchonestControl::updateWidgets()
{
    if( !m_input.isNull() )
        delete m_input.data();
    if( !m_match.isNull() )
        delete m_match.data();
    m_overrideType = -1;
    
    // make sure the widgets are the proper kind for the selected type, and hook up to their slots
    if( selectedType() == "Artist" ) {
        m_currentType = Echonest::DynamicPlaylist::Artist;
        
        QComboBox* match = new QComboBox();
        QLineEdit* input =  new QLineEdit();
        
        match->addItem( "Limit To", Echonest::DynamicPlaylist::ArtistType );
        match->addItem( "Similar To", Echonest::DynamicPlaylist::ArtistRadioType );
        match->addItem( "Description", Echonest::DynamicPlaylist::ArtistDescriptionType );
        m_matchString = match->currentText();
        m_matchData = match->itemData( match->currentIndex() ).toString();
        
        input->setPlaceholderText( "Artist name" );
        input->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed );
        
        connect( match, SIGNAL( currentIndexChanged(int) ), this, SLOT( updateData() ) );
        connect( match, SIGNAL( currentIndexChanged(int) ), this, SIGNAL( changed() ) );
        connect( input, SIGNAL( textChanged(QString) ), this, SLOT( updateData() ) );
        connect( input, SIGNAL( editingFinished() ), this, SLOT( editingFinished() ) );
        connect( input, SIGNAL( textEdited( QString ) ), &m_editingTimer, SLOT( stop() ) );
        
        match->hide();
        input->hide();
        m_match = QWeakPointer< QWidget >( match );
        m_input = QWeakPointer< QWidget >( input );
    } else if( selectedType() == "Variety" ) {
        m_currentType = Echonest::DynamicPlaylist::Variety;
        
        QLabel* match = new QLabel( tr( "is" ) );
        LabeledSlider* input = new LabeledSlider( tr( "Less" ), tr( "More" ) );
        input->slider()->setRange( 0, 10000 );
        input->slider()->setTickInterval( 1 );
        input->slider()->setTracking( false );

        m_matchString = match->text();
        m_matchData = match->text();
        
        
        connect( input->slider(), SIGNAL( valueChanged( int ) ), this, SLOT( updateData() ) );
        connect( input->slider(), SIGNAL( sliderMoved( int ) ), this, SLOT( editingFinished() ) );
        connect( input->slider(), SIGNAL( sliderMoved( int ) ), &m_editingTimer, SLOT( stop() ) );
        
        match->hide();
        input->hide();
        m_match = QWeakPointer< QWidget >( match );
        m_input = QWeakPointer< QWidget >( input );
    }  else if( selectedType() == "Tempo" ) {
        m_currentType = Echonest::DynamicPlaylist::MinTempo;
        
        setupMinMaxWidgets( Echonest::DynamicPlaylist::MinTempo, Echonest::DynamicPlaylist::MaxTempo, tr( "0 BPM" ), tr( "500 BPM" ), 500 );
    } else {
        m_match = QWeakPointer<QWidget>( new QWidget );
        m_input = QWeakPointer<QWidget>( new QWidget );
    }
}

void 
Tomahawk::EchonestControl::setupMinMaxWidgets( Echonest::DynamicPlaylist::PlaylistParam min, Echonest::DynamicPlaylist::PlaylistParam max, const QString& leftL, const QString& rightL, int maxRange )
{
    QComboBox* match = new QComboBox;
    match->addItem( "At Least", min );
    match->addItem( "At Most", max );
    
    LabeledSlider* input = new LabeledSlider( leftL, rightL );
    input->slider()->setRange( 0, maxRange );
    input->slider()->setTickInterval( 1 );
    input->slider()->setTracking( false );
    input->slider()->setTickPosition( QSlider::TicksBelow );
    
    m_matchString = match->currentText();
    m_matchData = match->itemData( match->currentIndex() ).toString();
    
    
    connect( input->slider(), SIGNAL( valueChanged( int ) ), this, SLOT( updateData() ) );
    connect( input->slider(), SIGNAL( sliderMoved( int ) ), this, SLOT( editingFinished() ) );
    connect( input->slider(), SIGNAL( sliderMoved( int ) ), &m_editingTimer, SLOT( stop() ) );
    
    match->hide();
    input->hide();
    m_match = QWeakPointer< QWidget >( match );
    m_input = QWeakPointer< QWidget >( input );
}


void 
Tomahawk::EchonestControl::updateData()
{
    if( selectedType() == "Artist" ) {
        QComboBox* combo = qobject_cast<QComboBox*>( m_match.data() );
        if( combo ) {
            m_matchString = combo->currentText();
            m_matchData = combo->itemData( combo->currentIndex() ).toString();
            
            // EN HACK: artist-description radio needs description= fields not artist= fields
            if( m_matchData.toInt() == Echonest::DynamicPlaylist::ArtistDescriptionType )
                m_overrideType = Echonest::DynamicPlaylist::Description;
        }
        QLineEdit* edit = qobject_cast<QLineEdit*>( m_input.data() );
        if( edit && !edit->text().isEmpty() ) {
            m_data.first = m_currentType;
            m_data.second = edit->text();
        }
    } else if( selectedType() == "Variety" ) {
        LabeledSlider* s = qobject_cast<LabeledSlider*>( m_input.data() );
        if( s ) {
            m_data.first = m_currentType;
            m_data.second = (qreal)s->slider()->value() / 10000.0;
        }
    } else if( selectedType() == "Tempo" ) {
        updateFromComboAndSlider();
    }
}

void 
Tomahawk::EchonestControl::updateFromComboAndSlider()
{
    QComboBox* combo = qobject_cast<QComboBox*>( m_match.data() );
    if( combo ) {
        m_matchString = combo->currentText();
        m_matchData = combo->itemData( combo->currentIndex() ).toString();
    }
    LabeledSlider* ls = qobject_cast<LabeledSlider*>( m_input.data() );
    if( ls && ls->slider() ) {
        m_data.first = static_cast< Echonest::DynamicPlaylist::PlaylistParam >( combo->itemData( combo->currentIndex() ).toInt() );
        m_data.second = ls->slider()->value();
    }
}


// fills in the current widget with the data from json or dbcmd (m_data.second and m_matchData)
void 
Tomahawk::EchonestControl::updateWidgetsFromData()
{
    if( selectedType() == "Artist" ) {
        QComboBox* combo = qobject_cast<QComboBox*>( m_match.data() );
        if( combo )
            combo->setCurrentIndex( combo->findData( m_matchData ) );
        QLineEdit* edit = qobject_cast<QLineEdit*>( m_input.data() );
        if( edit )
            edit->setText( m_data.second.toString() );
    } else if( selectedType() == "Variety" ) {
        LabeledSlider* s = qobject_cast<LabeledSlider*>( m_input.data() );
        if( s )
            s->slider()->setValue( m_data.second.toDouble() * 10000 );
    } else if( selectedType() == "Tempo" ) {
        updateToComboAndSlider();
    }
}

void 
Tomahawk::EchonestControl::updateToComboAndSlider()
{
    QComboBox* combo = qobject_cast<QComboBox*>( m_match.data() );
    if( combo )
        combo->setCurrentIndex( combo->findData( m_matchData ) );
    LabeledSlider* ls = qobject_cast<LabeledSlider*>( m_input.data() );
    if( ls )
        ls->slider()->setValue( m_data.second.toDouble() );
}


void 
Tomahawk::EchonestControl::editingFinished()
{
    qDebug() << Q_FUNC_INFO;
    m_editingTimer.start();
}

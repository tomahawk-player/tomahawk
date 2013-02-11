/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "playlist/dynamic/echonest/EchonestControl.h"

#include "playlist/dynamic/widgets/MiscControlWidgets.h"

#include "EchonestGenerator.h"

#include "utils/Logger.h"
#include "SourceList.h"

#include <echonest/Playlist.h>

#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QCompleter>
#include <QStringListModel>


QHash< QString, QStringList > Tomahawk::EchonestControl::s_suggestCache = QHash< QString, QStringList >();
bool Tomahawk::EchonestControl::s_fetchingMoodsStylesAndGenres = false;
int Tomahawk::EchonestControl::s_stylePollCount = 0;


Tomahawk::EchonestControl::EchonestControl( const QString& selectedType, const QStringList& typeSelectors, QObject* parent )
    : DynamicControl ( selectedType.isEmpty() ? "Artist" : selectedType, typeSelectors, parent )
{
    setType( "echonest" );
    m_editingTimer.setInterval( 500 ); //timeout to edits
    m_editingTimer.setSingleShot( true );
    connect( &m_editingTimer, SIGNAL( timeout() ), this, SLOT( editTimerFired() ) );

    m_delayedEditTimer.setInterval( 250 ); // additional timer for "just typing" without enter or focus change
    m_delayedEditTimer.setSingleShot( true );
    connect( &m_delayedEditTimer, SIGNAL( timeout() ), &m_editingTimer, SLOT( start() ) );

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
    if( type != selectedType() )
    {
        if( !m_input.isNull() )
            delete m_input.data();
        if( !m_match.isNull() )
            delete m_match.data();

        Tomahawk::DynamicControl::setSelectedType ( type );
        updateWidgets();
        updateData();
        //qDebug() << "Setting new type, set data to:" << m_data.first << m_data.second;
    }
}


Echonest::DynamicPlaylist::PlaylistParamData
Tomahawk::EchonestControl::toENParam() const
{
    if( m_overrideType != -1 )
    {
        Echonest::DynamicPlaylist::PlaylistParamData newData = m_data;
        newData.first = static_cast<Echonest::DynamicPlaylist::PlaylistParam>( m_overrideType );
        return newData;
    }
    return m_data;
}


QString
Tomahawk::EchonestControl::input() const
{
    return m_data.second.toString();
}


QString
Tomahawk::EchonestControl::match() const
{
    return m_matchData;
}


QString
Tomahawk::EchonestControl::matchString() const
{
    return m_matchString;
}


QString
Tomahawk::EchonestControl::summary() const
{
    if( m_summary.isEmpty() )
        const_cast< EchonestControl* >( this )->calculateSummary();

    return m_summary;
}


void
Tomahawk::EchonestControl::setInput( const QString& input )
{
    m_data.second = input;
    updateWidgetsFromData();
}


void
Tomahawk::EchonestControl::setMatch( const QString& match )
{
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
    if( selectedType() == "Artist" )
    {
        m_currentType = Echonest::DynamicPlaylist::Artist;

        QComboBox* match = new QComboBox();
        QLineEdit* input =  new QLineEdit();

        match->addItem( "Similar To", Echonest::DynamicPlaylist::ArtistRadioType );
        match->addItem( "Limit To", Echonest::DynamicPlaylist::ArtistType );
        m_matchString = match->currentText();
        m_matchData = match->itemData( match->currentIndex() ).toString();

        input->setPlaceholderText( "Artist name" );
        input->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed );
        input->setCompleter( new QCompleter( QStringList(), input ) );
        input->completer()->setCaseSensitivity( Qt::CaseInsensitive );

        connect( match, SIGNAL( currentIndexChanged(int) ), this, SLOT( updateData() ) );
        connect( match, SIGNAL( currentIndexChanged(int) ), this, SIGNAL( changed() ) );
        connect( input, SIGNAL( textChanged(QString) ), this, SLOT( updateData() ) );
        connect( input, SIGNAL( editingFinished() ), this, SLOT( editingFinished() ) );
        connect( input, SIGNAL( textEdited( QString ) ), &m_editingTimer, SLOT( stop() ) );
        connect( input, SIGNAL( textEdited( QString ) ), &m_delayedEditTimer, SLOT( start() ) );
        connect( input, SIGNAL( textEdited( QString ) ), this, SLOT( artistTextEdited( QString ) ) );

        match->hide();
        input->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( input );
        m_data.first = m_currentType;
    }
    else if( selectedType() == "Artist Description" )
    {
        m_currentType = Echonest::DynamicPlaylist::Description;

        QLabel* match = new QLabel( tr( "is" ) );
        QLineEdit* input =  new QLineEdit();

        m_matchString = QString();
        m_matchData = QString::number( (int)Echonest::DynamicPlaylist::ArtistDescriptionType );

        connect( input, SIGNAL( textChanged(QString) ), this, SLOT( updateData() ) );
        connect( input, SIGNAL( editingFinished() ), this, SLOT( editingFinished() ) );
        connect( input, SIGNAL( textEdited( QString ) ), &m_editingTimer, SLOT( stop() ) );
        connect( input, SIGNAL( textEdited( QString ) ), &m_delayedEditTimer, SLOT( start() ) );

        match->hide();
        input->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( input );
        m_data.first = m_currentType;
    }
    else if( selectedType() == "User Radio" )
    {
        m_currentType = Echonest::DynamicPlaylist::SourceCatalog;

        QLabel* match = new QLabel( tr( "from user" ) );
        QComboBox* combo =  new QComboBox();

        foreach( const QString& str, EchonestGenerator::userCatalogs() )
        {
            combo->addItem( str, EchonestGenerator::catalogId( str ) );
        }

        if ( EchonestGenerator::userCatalogs().isEmpty() )
            combo->addItem( tr( "No users with Echo Nest Catalogs enabled. Try enabling option in Collection settings" ) );

        if ( combo->findData( m_data.second ) < 0 )
            combo->setCurrentIndex( 0 );

        m_matchString = match->text();
        m_matchData = match->text();


        connect( combo, SIGNAL( activated( int ) ), this, SLOT( updateData() ) );
        connect( combo, SIGNAL( activated( int ) ), this, SLOT( editingFinished() ) );

        match->hide();
        combo->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( combo );
    }
    else if( selectedType() == "Song" )
    {
        m_currentType = Echonest::DynamicPlaylist::SongId;

        QLabel* match = new QLabel( tr( "similar to" ) );
        QLineEdit* input =  new QLineEdit();
        input->setPlaceholderText( "Enter any combination of song name and artist here..." );

        m_matchString = QString();
        m_matchData = QString::number( (int)Echonest::DynamicPlaylist::SongRadioType );

        connect( input, SIGNAL( textChanged(QString) ), this, SLOT( updateData() ) );
        connect( input, SIGNAL( editingFinished() ), this, SLOT( editingFinished() ) );
        connect( input, SIGNAL( textEdited( QString ) ), &m_editingTimer, SLOT( stop() ) );
        connect( input, SIGNAL( textEdited( QString ) ), &m_delayedEditTimer, SLOT( start() ) );

        match->hide();
        input->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( input );
        m_data.first = m_currentType;
    }
    else if( selectedType() == "Variety" )
    {
        m_currentType = Echonest::DynamicPlaylist::Variety;

        QLabel* match = new QLabel( tr( "is" ) );
        LabeledSlider* input = new LabeledSlider( tr( "Less" ), tr( "More" ) );
        input->slider()->setRange( 0, 10000 );
        input->slider()->setTickInterval( 1 );
        input->slider()->setTracking( false );

        m_matchString = match->text();
        m_matchData = match->text();


        connect( input->slider(), SIGNAL( valueChanged( int ) ), this, SLOT( updateData() ) );
        connect( input->slider(), SIGNAL( valueChanged( int ) ), this, SLOT( editingFinished() ) );

        match->hide();
        input->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( input );
        m_data.first = m_currentType;
    }
    else if( selectedType() == "Adventurousness" )
    {
        m_currentType = Echonest::DynamicPlaylist::Adventurousness;

        QLabel* match = new QLabel( tr( "is" ) );
        LabeledSlider* input = new LabeledSlider( tr( "Less" ), tr( "More" ) );
        input->slider()->setRange( 0, 10000 );
        input->slider()->setTickInterval( 1 );
        input->slider()->setTracking( false );
        input->slider()->setValue( 10000 * .2 );

        m_matchString = match->text();
        m_matchData = match->text();


        connect( input->slider(), SIGNAL( valueChanged( int ) ), this, SLOT( updateData() ) );
        connect( input->slider(), SIGNAL( valueChanged( int ) ), this, SLOT( editingFinished() ) );

        match->hide();
        input->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( input );
        m_data.first = m_currentType;
    }
    else if( selectedType() == "Tempo" )
    {
        m_currentType = Echonest::DynamicPlaylist::MinTempo;

        setupMinMaxWidgets( Echonest::DynamicPlaylist::MinTempo, Echonest::DynamicPlaylist::MaxTempo, tr( "0 BPM" ), tr( "500 BPM" ), 500 );
    }
    else if( selectedType() == "Duration" )
    {
        m_currentType = Echonest::DynamicPlaylist::MinDuration;

        setupMinMaxWidgets( Echonest::DynamicPlaylist::MinDuration, Echonest::DynamicPlaylist::MaxDuration, tr( "0 secs" ), tr( "3600 secs" ), 3600 );
    }
    else if( selectedType() == "Loudness" )
    {
        m_currentType = Echonest::DynamicPlaylist::MinLoudness;

        setupMinMaxWidgets( Echonest::DynamicPlaylist::MinLoudness, Echonest::DynamicPlaylist::MaxLoudness, tr( "-100 dB" ), tr( "100 dB" ), 100 );
        qobject_cast< LabeledSlider* >( m_input.data() )->slider()->setMinimum( -100 );
    }
    else if( selectedType() == "Danceability" )
    {
        m_currentType = Echonest::DynamicPlaylist::MinDanceability;

        setupMinMaxWidgets( Echonest::DynamicPlaylist::MinDanceability, Echonest::DynamicPlaylist::MaxDanceability, tr( "Less" ), tr( "More" ), 10000 );
    }
    else if( selectedType() == "Energy" )
    {
        m_currentType = Echonest::DynamicPlaylist::MinEnergy;

        setupMinMaxWidgets( Echonest::DynamicPlaylist::MinEnergy, Echonest::DynamicPlaylist::MaxEnergy, tr( "Less" ), tr( "More" ), 10000 );
    }
    else if( selectedType() == "Artist Familiarity" )
    {
        m_currentType = Echonest::DynamicPlaylist::ArtistMinFamiliarity;

        setupMinMaxWidgets( Echonest::DynamicPlaylist::ArtistMinFamiliarity, Echonest::DynamicPlaylist::ArtistMaxFamiliarity, tr( "Less" ), tr( "More" ), 10000 );
    }
    else if( selectedType() == "Artist Hotttnesss" )
    {
        m_currentType = Echonest::DynamicPlaylist::ArtistMinHotttnesss;

        setupMinMaxWidgets( Echonest::DynamicPlaylist::ArtistMinHotttnesss, Echonest::DynamicPlaylist::ArtistMaxHotttnesss, tr( "Less" ), tr( "More" ), 10000 );
    }
    else if( selectedType() == "Song Hotttnesss" )
    {
        m_currentType = Echonest::DynamicPlaylist::SongMinHotttnesss;

        setupMinMaxWidgets( Echonest::DynamicPlaylist::SongMinHotttnesss, Echonest::DynamicPlaylist::SongMaxHotttnesss, tr( "Less" ), tr( "More" ), 10000 );
    }
    else if( selectedType() == "Latitude" )
    {
        m_currentType = Echonest::DynamicPlaylist::ArtistMinLatitude;
        QString deg = QString( QChar( 0x00B0 ) );
        setupMinMaxWidgets( Echonest::DynamicPlaylist::ArtistMinLatitude, Echonest::DynamicPlaylist::ArtistMaxLatitude, QString( "-180%1" ).arg( deg ), QString( "180%1" ).arg( deg ), 180 );
        qobject_cast< LabeledSlider* >( m_input.data() )->slider()->setMinimum( -180 );
    }
    else if( selectedType() == "Longitude" )
    {
        m_currentType = Echonest::DynamicPlaylist::ArtistMinLongitude;
        QString deg = QString( QChar( 0x00B0 ) );
        setupMinMaxWidgets( Echonest::DynamicPlaylist::ArtistMinLongitude, Echonest::DynamicPlaylist::ArtistMaxLongitude, QString( "-180%1" ).arg( deg ), QString( "180%1" ).arg( deg ), 180 );
        qobject_cast< LabeledSlider* >( m_input.data() )->slider()->setMinimum( -180 );
    }
    else if( selectedType() == "Mode" )
    {
        m_currentType = Echonest::DynamicPlaylist::Mode;

        QLabel* match = new QLabel( tr( "is" ) );
        QComboBox* combo = new QComboBox;
        combo->addItem( tr( "Major" ), QString::number( 1 ) );
        combo->addItem( tr( "Minor" ), QString::number( 0 ) );

        m_matchString = match->text();
        m_matchData = match->text();


        connect( combo, SIGNAL( activated( int ) ), this, SLOT( updateData() ) );
        connect( combo, SIGNAL( activated( int ) ), this, SLOT( editingFinished() ) );

        match->hide();
        combo->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( combo );
    }
    else if( selectedType() == "Key" )
    {
        m_currentType = Echonest::DynamicPlaylist::Key;

        QLabel* match = new QLabel( tr( "is" ) );
        QComboBox* combo = new QComboBox;
        combo->addItem( tr( "C" ), QString::number( 0 ) );
        combo->addItem( tr( "C Sharp" ), QString::number( 1 ) );
        combo->addItem( tr( "D" ), QString::number( 2 ) );
        combo->addItem( tr( "E Flat" ), QString::number( 3 ) );
        combo->addItem( tr( "E" ), QString::number( 4 ) );
        combo->addItem( tr( "F" ), QString::number( 5 ) );
        combo->addItem( tr( "F Sharp" ), QString::number( 6 ) );
        combo->addItem( tr( "G" ), QString::number( 7 ) );
        combo->addItem( tr( "A Flat" ), QString::number( 8 ) );
        combo->addItem( tr( "A" ), QString::number( 9 ) );
        combo->addItem( tr( "B Flat" ), QString::number( 10 ) );
        combo->addItem( tr( "B" ), QString::number( 11 ) );

        m_matchString = match->text();
        m_matchData = match->text();


        connect( combo, SIGNAL( activated( int ) ), this, SLOT( updateData() ) );
        connect( combo, SIGNAL( activated( int ) ), this, SLOT( editingFinished() ) );

        match->hide();
        combo->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( combo );
    }
    else if( selectedType() == "Sorting" )
    {
        m_currentType = Echonest::DynamicPlaylist::Sort;

        QComboBox* match = new QComboBox();
        match->addItem( tr( "Ascending" ), 0 );
        match->addItem( tr( "Descending" ), 1 );

        QComboBox* combo = new QComboBox;
        combo->addItem( tr( "Tempo" ), QString::number( Echonest::DynamicPlaylist::SortTempoAscending ) );
        combo->addItem( tr( "Duration" ), QString::number( Echonest::DynamicPlaylist::SortDurationAscending ) );
        combo->addItem( tr( "Loudness" ), QString::number( Echonest::DynamicPlaylist::SortLoudnessAscending ) );
        combo->addItem( tr( "Artist Familiarity" ), QString::number( Echonest::DynamicPlaylist::SortArtistFamiliarityAscending ) );
        combo->addItem( tr( "Artist Hotttnesss" ), QString::number( Echonest::DynamicPlaylist::SortArtistHotttnessAscending ) );
        combo->addItem( tr( "Song Hotttnesss" ), QString::number( Echonest::DynamicPlaylist::SortSongHotttnesssAscending ) );
        combo->addItem( tr( "Latitude" ), QString::number( Echonest::DynamicPlaylist::SortLatitudeAscending ) );
        combo->addItem( tr( "Longitude" ), QString::number( Echonest::DynamicPlaylist::SortLongitudeAscending ) );
        combo->addItem( tr( "Mode" ), QString::number( Echonest::DynamicPlaylist::SortModeAscending ) );
        combo->addItem( tr( "Key" ), QString::number( Echonest::DynamicPlaylist::SortKeyAscending ) );
        combo->addItem( tr( "Energy" ), QString::number( Echonest::DynamicPlaylist::SortEnergyAscending ) );
        combo->addItem( tr( "Danceability" ), QString::number( Echonest::DynamicPlaylist::SortDanceabilityAscending ) );

        m_matchString = "Ascending"; // default
        m_matchData = Echonest::DynamicPlaylist::SortTempoAscending;

        connect( match, SIGNAL( activated( int ) ), this, SLOT( updateData() ) );
        connect( match, SIGNAL( activated( int ) ), this, SLOT( editingFinished() ) );
        connect( combo, SIGNAL( activated( int ) ), this, SLOT( updateData() ) );
        connect( combo, SIGNAL( activated( int ) ), this, SLOT( editingFinished() ) );

        match->hide();
        combo->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( combo );
    }
    else if( selectedType() == "Mood" || selectedType() == "Style" || selectedType() == "Genre" )
    {
        if( selectedType() == "Mood" )
            m_currentType = Echonest::DynamicPlaylist::Mood;
        else if ( selectedType() == "Style" )
            m_currentType = Echonest::DynamicPlaylist::Style;
        else
            m_currentType = Echonest::DynamicPlaylist::Genre;

        QLabel* match = new QLabel( tr( "is" ) );

        QComboBox* combo = new QComboBox;

        m_matchString = match->text();
        m_matchData = match->text();


        connect( combo, SIGNAL( activated( int ) ), this, SLOT( updateData() ) );
        connect( combo, SIGNAL( activated( int ) ), this, SLOT( editingFinished() ) );

        match->hide();
        combo->hide();
        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( combo );

        insertMoodsStylesAndGenres();
    }
    else if( selectedType() == "Song Type" )
    {
        m_currentType = Echonest::DynamicPlaylist::SongType;

        QComboBox* match = new QComboBox();
        match->addItem( tr( "is" ), 1 );
        match->addItem( tr( "is not" ), 0 );

        QComboBox* combo = new QComboBox();
        combo->addItem( tr( "Studio" ), "studio" );
        combo->addItem( tr( "Live" ), "live" );
        combo->addItem( tr( "Christmas" ), "christmas" );

        connect( match, SIGNAL( activated( int ) ), this, SLOT( updateData() ) );
        connect( match, SIGNAL( activated( int ) ), this, SLOT( editingFinished() ) );
        connect( combo, SIGNAL( activated( int ) ), this, SLOT( updateData() ) );
        connect( combo, SIGNAL( activated( int ) ), this, SLOT( editingFinished() ) );

        m_matchString = "is";
        m_matchData = 1;

        m_match = QPointer< QWidget >( match );
        m_input = QPointer< QWidget >( combo );
    }
    else
    {
        m_match = QPointer<QWidget>( new QWidget );
        m_input = QPointer<QWidget>( new QWidget );
    }
    updateData();
    calculateSummary();
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

    m_matchString = match->currentText();
    m_matchData = match->itemData( match->currentIndex() ).toString();

    connect( match, SIGNAL( activated( int ) ), this, SLOT( updateData() ) );
    connect( match, SIGNAL( activated( int ) ), this, SLOT( editingFinished() ) );
    connect( input->slider(), SIGNAL( valueChanged( int ) ), this, SLOT( updateData() ) );
    connect( input->slider(), SIGNAL( valueChanged( int ) ), this, SLOT( editingFinished() ) );

    match->hide();
    input->hide();
    m_match = QPointer< QWidget >( match );
    m_input = QPointer< QWidget >( input );
}


void
Tomahawk::EchonestControl::updateData()
{
    if( selectedType() == "Artist" )
    {
        QComboBox* combo = qobject_cast<QComboBox*>( m_match.data() );
        if( combo )
        {
            m_matchString = combo->currentText();
            m_matchData = combo->itemData( combo->currentIndex() ).toString();
        }
        QLineEdit* edit = qobject_cast<QLineEdit*>( m_input.data() );
        if( edit && !edit->text().isEmpty() )
        {
            m_data.first = m_currentType;
            m_data.second = edit->text();
        }
    }
    else if( selectedType() == "Artist Description" || selectedType() == "Song" )
    {
        QLineEdit* edit = qobject_cast<QLineEdit*>( m_input.data() );
        if( edit && !edit->text().isEmpty() )
        {
            m_data.first = m_currentType;
            m_data.second = edit->text();
        }
    }
    else if( selectedType() == "Variety" || selectedType() == "Adventurousness" )
    {
        LabeledSlider* s = qobject_cast<LabeledSlider*>( m_input.data() );
        if( s )
        {
            m_data.first = m_currentType;
            m_data.second = (qreal)s->slider()->value() / 10000.0;
        }
    }
    else if( selectedType() == "Tempo" || selectedType() == "Duration" || selectedType() == "Loudness" || selectedType() == "Latitude" || selectedType() == "Longitude" )
    {
        updateFromComboAndSlider();
    }
    else if( selectedType() == "Danceability" || selectedType() == "Energy" || selectedType() == "Artist Familiarity" || selectedType() == "Artist Hotttnesss" || selectedType() == "Song Hotttnesss" )
    {
        updateFromComboAndSlider( true );
    }
    else if( selectedType() == "Mode" || selectedType() == "Key" || selectedType() == "Mood" || selectedType() == "Style" || selectedType() == "Genre" || selectedType() == "User Radio" )
    {
        updateFromLabelAndCombo();
    }
    else if( selectedType() == "Sorting" )
    {
        QComboBox* match = qobject_cast<QComboBox*>( m_match.data() );
        QComboBox* input = qobject_cast< QComboBox* >( m_input.data() );
        if( match && input ) {
            m_matchString = match->currentText();
            m_matchData = match->itemData( match->currentIndex() ).toString();

            // what a HACK
            int enumVal = input->itemData( input->currentIndex() ).toInt() + m_matchData.toInt();
            m_data.first = Echonest::DynamicPlaylist::Sort;
            m_data.second = enumVal;
//             qDebug() << "SAVING" << input->currentIndex() << "AS" << enumVal << "(" << input->itemData( input->currentIndex() ).toInt() << "+" << m_matchData.toInt() << ")";
        }
    }
    else if( selectedType() == "Song Type" )
    {
        QComboBox* match = qobject_cast<QComboBox*>( m_match.data() );
        QComboBox* combo = qobject_cast< QComboBox* >( m_input.data() );
        if ( match && combo )
        {
            m_matchString = match->currentText();
            m_matchData = match->itemData( match->currentIndex() ).toString();

            QString songType = combo->itemData( combo->currentIndex() ).toString();
            if ( match->currentIndex() == 1 )
               songType.append( ":false" );

            m_data.first = Echonest::DynamicPlaylist::SongType;
            m_data.second = songType;
        }

    }

    calculateSummary();
}


void
Tomahawk::EchonestControl::updateFromComboAndSlider( bool smooth )
{
    QComboBox* combo = qobject_cast<QComboBox*>( m_match.data() );
    if( combo )
    {
        m_matchString = combo->currentText();
        m_matchData = combo->itemData( combo->currentIndex() ).toString();
    }
    LabeledSlider* ls = qobject_cast<LabeledSlider*>( m_input.data() );
    if( ls && ls->slider() )
    {
        m_data.first = static_cast< Echonest::DynamicPlaylist::PlaylistParam >( combo->itemData( combo->currentIndex() ).toInt() );
        m_data.second = ls->slider()->value() / ( smooth ? 10000. : 1.0 );
    }
}


void
Tomahawk::EchonestControl::updateFromLabelAndCombo()
{
    QComboBox* s = qobject_cast<QComboBox*>( m_input.data() );
    if( s )
    {
        m_data.first = m_currentType;
        m_data.second = s->itemData( s->currentIndex() );
    }
}


// fills in the current widget with the data from json or dbcmd (m_data.second and m_matchData)
void
Tomahawk::EchonestControl::updateWidgetsFromData()
{
    if( selectedType() == "Artist" )
    {
        QComboBox* combo = qobject_cast<QComboBox*>( m_match.data() );
        if( combo )
            combo->setCurrentIndex( combo->findData( m_matchData ) );
        QLineEdit* edit = qobject_cast<QLineEdit*>( m_input.data() );
        if( edit )
            edit->setText( m_data.second.toString() );
    }
    else if( selectedType() == "Artist Description" || selectedType() == "Song" )
    {
        QLineEdit* edit = qobject_cast<QLineEdit*>( m_input.data() );
        if( edit )
            edit->setText( m_data.second.toString() );
    }
    else if ( selectedType() == "User Radio"  )
    {
        QComboBox* combo = qobject_cast< QComboBox* >( m_input.data() );
        if ( combo )
        {
            combo->clear();

            foreach( const QString& str, EchonestGenerator::userCatalogs() )
            {
                combo->addItem( str, EchonestGenerator::catalogId( str ) );
            }

            if ( EchonestGenerator::userCatalogs().isEmpty() )
                combo->addItem( tr( "No users with Echo Nest Catalogs enabled. Try enabling option in Collection settings" ) );

            if ( combo->findData( m_data.second ) < 0 )
                combo->setCurrentIndex( 0 );

            combo->setCurrentIndex( combo->findData( m_data.second ) );
        }
    }
    else if( selectedType() == "Variety" || selectedType() == "Adventurousness" )
    {
        LabeledSlider* s = qobject_cast<LabeledSlider*>( m_input.data() );
        if( s )
            s->slider()->setValue( m_data.second.toDouble() * 10000 );
    }
    else if( selectedType() == "Tempo" || selectedType() == "Duration" || selectedType() == "Loudness"  || selectedType() == "Latitude" || selectedType() == "Longitude" )
    {
        updateToComboAndSlider();
    }
    else if( selectedType() == "Danceability" || selectedType() == "Energy" || selectedType() == "Artist Familiarity" || selectedType() == "Artist Hotttnesss" || selectedType() == "Song Hotttnesss" )
    {
        updateToComboAndSlider( true );
    }
    else if( selectedType() == "Mode" || selectedType() == "Key" || selectedType() == "Mood" || selectedType() == "Style" || selectedType() == "Genre" )
    {
        updateToLabelAndCombo();
    }
    else if( selectedType() == "Sorting" )
    {
        QComboBox* match = qobject_cast<QComboBox*>( m_match.data() );
        QComboBox* input = qobject_cast< QComboBox* >( m_input.data() );
        if( match && input ) {
            match->setCurrentIndex( match->findData( m_matchData ));

            // HACK alert. if it's odd, subtract 1
            int val = ( m_data.second.toInt() - ( m_data.second.toInt() % 2 ) ) / 2;
            input->setCurrentIndex( val );
            //qDebug() << "LOADING" << m_data.second.toInt() << "AS" << val;
        }
    }
    else if( selectedType() == "Song Type" )
    {
        QComboBox* match = qobject_cast<QComboBox*>( m_match.data() );
        QComboBox* combo = qobject_cast< QComboBox* >( m_input.data() );
        if ( match && combo ) {
            match->setCurrentIndex( match->findData( m_matchData ));

            QString songType = m_data.second.toString().split( ":" ).at( 0 );
            combo->setCurrentIndex( combo->findData( songType ) );
        }
    }
    calculateSummary();
}


void
Tomahawk::EchonestControl::updateToComboAndSlider( bool smooth )
{
    QComboBox* combo = qobject_cast<QComboBox*>( m_match.data() );
    if( combo )
        combo->setCurrentIndex( combo->findData( m_matchData ) );
    LabeledSlider* ls = qobject_cast<LabeledSlider*>( m_input.data() );
    if( ls )
        ls->slider()->setValue( m_data.second.toDouble() * ( smooth ? 10000. : 1 ) );
}


void
Tomahawk::EchonestControl::updateToLabelAndCombo()
{
    QComboBox* s = qobject_cast< QComboBox* >( m_input.data() );
    if( s )
        s->setCurrentIndex( s->findData( m_data.second ) );
}


void
Tomahawk::EchonestControl::editingFinished()
{
//    qDebug() << Q_FUNC_INFO;
    m_editingTimer.start();
}


void
Tomahawk::EchonestControl::editTimerFired()
{
    // make sure it's really changed
    if( m_cacheData != m_data.second ) { // new, so emit changed
        emit changed();
    }

    m_cacheData = m_data.second;
}


void
Tomahawk::EchonestControl::artistTextEdited( const QString& text )
{
    // if the user is editing an artist field, try to help him out and suggest from echonest
    QLineEdit* l = qobject_cast<QLineEdit*>( m_input.data() );
    Q_ASSERT( l );
    Q_UNUSED( l );
//     l->setCompleter( new QCompleter( this ) ); // clear

    foreach( QNetworkReply* r, m_suggestWorkers ) {
        r->abort();
        r->deleteLater();
    }
    m_suggestWorkers.clear();

    if( !text.isEmpty() ) {
        if( s_suggestCache.contains( text ) ) {
            addArtistSuggestions( s_suggestCache[ text ] );
        } else { // gotta look it up
            QNetworkReply* r = Echonest::Artist::suggest( text );
            qDebug() << "Asking echonest for suggestions to help our completion..." << r->url().toString();
            r->setProperty( "curtext", text );

            m_suggestWorkers.insert( r );
            connect( r, SIGNAL( finished() ), this, SLOT( suggestFinished() ) );
        }
    }
}


void
Tomahawk::EchonestControl::suggestFinished()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );
    QLineEdit* l = qobject_cast<QLineEdit*>( m_input.data() );
    Q_ASSERT( l );

    m_suggestWorkers.remove( r );

    if( r->error() != QNetworkReply::NoError )
        return;

    QString origText = r->property( "curtext" ).toString();
    if( origText != l->text() ) { // user might have kept on typing, then ignore
        qDebug() << "Text changed meanwhile, stopping suggestion parsing";
        return;
    }

    QStringList suggestions;
    try {
        Echonest::Artists artists = Echonest::Artist::parseSuggest( r );
        foreach( const Echonest::Artist& artist, artists )
            suggestions << artist.name();
    } catch( Echonest::ParseError& e ) {
        qWarning() << "libechonest failed to parse this artist/suggest call..." << e.errorType() << e.what();
        return;
    }

    s_suggestCache[ origText ] = suggestions;
    addArtistSuggestions( suggestions );
}


void
Tomahawk::EchonestControl::addArtistSuggestions( const QStringList& suggestions )
{
    // if the user is editing an artist field, try to help him out and suggest from echonest
    QLineEdit* l = qobject_cast<QLineEdit*>( m_input.data() );
    Q_ASSERT( l );

    l->completer()->setModel( new QStringListModel( suggestions, l->completer() ) );
    l->completer()->complete();
}


void
Tomahawk::EchonestControl::calculateSummary()
{
    // turns the current control into an english phrase suitable for embedding into a sentence summary
    QString summary;
    if ( selectedType() == "Artist" )
    {
        // magic char is used by EchonestGenerator to split the prefix from the artist name
        if ( static_cast< Echonest::DynamicPlaylist::ArtistTypeEnum >( m_matchData.toInt() ) == Echonest::DynamicPlaylist::ArtistType )
            summary = tr( "only by ~%1" ).arg( m_data.second.toString() );
        else if ( static_cast< Echonest::DynamicPlaylist::ArtistTypeEnum >( m_matchData.toInt() ) == Echonest::DynamicPlaylist::ArtistRadioType )
            summary = tr( "similar to ~%1" ).arg( m_data.second.toString() );
    }
    else if( selectedType() == "Artist Description" )
    {
        summary = tr( "with genre ~%1" ).arg( m_data.second.toString() );
    }
    else if( selectedType() == "User Radio" )
    {
        QComboBox* b = qobject_cast< QComboBox* >( m_input.data() );
        if ( b )
        {
            if ( b->currentText().isEmpty() || b->itemData( b->currentIndex() ).isNull() )
                summary = tr( "from no one" );
            else
            {
                QString subSum;
                if ( b->currentText() == tr( "My Collection" ) )
                    summary = tr( "from my radio" );
                else
                    summary = tr( "from %1 radio" ).arg( b->currentText() );
            }
        }
        else
            summary = tr( "from no one" );
    }
    else if ( selectedType() == "Artist Description" || selectedType() == "Song" )
    {
        summary = tr( "similar to ~%1" ).arg( m_data.second.toString() );
    }
    else if ( selectedType() == QT_TR_NOOP( "Variety" ) || selectedType() == QT_TR_NOOP( "Danceability" ) ||
              selectedType() == QT_TR_NOOP( "Artist Hotttnesss" ) || selectedType() == QT_TR_NOOP( "Energy" ) ||
              selectedType() == QT_TR_NOOP( "Artist Familiarity" ) || selectedType() == QT_TR_NOOP( "Song Hotttnesss" ) ||
              selectedType() == QT_TR_NOOP( "Adventurousness" ) )
    {
        QString modifier;
        qreal sliderVal = m_data.second.toReal();
        // divide into avpproximate chunks
        if( 0.0 <= sliderVal && sliderVal < 0.2 )
            modifier = tr( "very low" );
        else if( 0.2 <= sliderVal && sliderVal < 0.4 )
            modifier = tr( "low" );
        else if( 0.4 <= sliderVal && sliderVal < 0.6 )
            modifier = tr( "moderate" );
        else if( 0.6 <= sliderVal && sliderVal < 0.8 )
            modifier = tr( "high" );
        else if( 0.8 <= sliderVal && sliderVal <= 1 )
            modifier = tr( "very high" );
        summary = tr( "with %1 %2" ).arg( modifier ).arg( tr( selectedType().toStdString().c_str() ) );
    }
    else if ( selectedType() == "Tempo" )
    {
        summary = tr( "about %1 BPM" ).arg( m_data.second.toString() );
    }
    else if ( selectedType() == "Duration" )
    {
        summary = tr( "about %n minute(s) long", "", m_data.second.toInt() / 60 );
    }
    else if ( selectedType() == "Loudness" )
    {
        summary = tr( "about %1 dB" ).arg( m_data.second.toString() );
    }
    else if ( selectedType() == "Latitude" || selectedType() == "Longitude" )
    {
        summary = tr( "at around %1%2 %3" ).arg( m_data.second.toString() ).arg( QString( QChar( 0x00B0 ) ) ).arg( tr( selectedType().toStdString().c_str() ) );
    }
    else if ( selectedType() == "Key" )
    {
        Q_ASSERT( !m_input.isNull() );
        Q_ASSERT( qobject_cast< QComboBox* >( m_input.data() ) );
        QString keyName = qobject_cast< QComboBox* >( m_input.data() )->currentText().toLower();
        summary = tr( "in %1" ).arg( keyName );
    }
    else if ( selectedType() == "Mode" )
    {
        Q_ASSERT( !m_input.isNull() );
        Q_ASSERT( qobject_cast< QComboBox* >( m_input.data() ) );
        QString modeName = qobject_cast< QComboBox* >( m_input.data() )->currentText().toLower();
        summary = tr( "in a %1 key" ).arg( modeName );
    }
    else if ( selectedType() == "Sorting" )
    {
        Q_ASSERT( !m_input.isNull() );
        Q_ASSERT( qobject_cast< QComboBox* >( m_input.data() ) );
        QString sortType = qobject_cast< QComboBox* >( m_input.data() )->currentText().toLower();

        Q_ASSERT( !m_match.isNull() );
        Q_ASSERT( qobject_cast< QComboBox* >( m_match.data() ) );
        QString ascdesc = qobject_cast< QComboBox* >( m_match.data() )->currentText().toLower();

        summary = tr( "sorted in %1 %2 order" ).arg( ascdesc ).arg( sortType );
    }
    else if ( selectedType() == "Mood" )
    {
        Q_ASSERT( !m_input.isNull() );
        Q_ASSERT( qobject_cast< QComboBox* >( m_input.data() ) );
        QString text = qobject_cast< QComboBox* >( m_input.data() )->currentText().toLower();
        summary = tr( "with a %1 mood" ).arg( text );
    }
    else if ( selectedType() == "Style" )
    {
        Q_ASSERT( !m_input.isNull() );
        Q_ASSERT( qobject_cast< QComboBox* >( m_input.data() ) );
        QString text = qobject_cast< QComboBox* >( m_input.data() )->currentText().toLower();
        summary = tr( "in a %1 style" ).arg( text );
    }
    else if ( selectedType() == "Genre" )
    {
        Q_ASSERT( !m_input.isNull() );
        Q_ASSERT( qobject_cast< QComboBox* >( m_input.data() ) );
        QString text = qobject_cast< QComboBox* >( m_input.data() )->currentText().toLower();
        summary = tr( "where genre is %1" ).arg( text );
    }
    else if ( selectedType() == "Song Type" )
    {
        Q_ASSERT( !m_input.isNull() );
        Q_ASSERT( qobject_cast< QComboBox* >( m_input.data() ) );
        QString text = qobject_cast< QComboBox* >( m_input.data() )->currentText();


        Q_ASSERT( !m_match.isNull() );
        Q_ASSERT( qobject_cast< QComboBox* >( m_match.data() ) );
        QComboBox* combo = qobject_cast< QComboBox* >( m_match.data() );
        if ( combo->currentIndex() == 0 )
            summary = tr( "where song type is %1" ).arg( text );
        else
            summary = tr( "where song type is not %1" ).arg( text );
    }

    m_summary = summary;
}


void
Tomahawk::EchonestControl::checkForMoodsStylesOrGenresFetched()
{
    s_fetchingMoodsStylesAndGenres = false;
    if( selectedType() == "Mood" || selectedType() == "Style" || selectedType() == "Genre" ) {
        QComboBox* cb = qobject_cast< QComboBox* >( m_input.data() );
        if( cb && cb->count() == 0 ) { // got nothing, so lets populate
            if( insertMoodsStylesAndGenres() )
                updateWidgetsFromData();
        }
    }
}


bool
Tomahawk::EchonestControl::insertMoodsStylesAndGenres()
{
    QStringList src;
    if ( selectedType() == "Mood" )
    {
        src = EchonestGenerator::moods();
    }
    else if ( selectedType() == "Style" )
    {
        src = EchonestGenerator::styles();
    }
    else
    {
        src = EchonestGenerator::genres();
    }

    QComboBox* combo = qobject_cast< QComboBox* >( m_input.data() );
    if( !combo )
        return false;

    foreach( const QString& item, src ) {
        combo->addItem( item, item );
    }

    if( src.isEmpty() && !combo->count() )
    {
        if( s_stylePollCount <= 20 && !s_fetchingMoodsStylesAndGenres )
        { // try for 20s to get the styles...
            s_fetchingMoodsStylesAndGenres = true;
            QTimer::singleShot( 1000, this, SLOT( checkForMoodsStylesOrGenresFetched() ) );
        }
        s_stylePollCount++;
        return false;
    }

    return true;
}

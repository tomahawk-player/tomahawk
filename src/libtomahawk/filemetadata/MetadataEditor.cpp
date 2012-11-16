/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christopher Reichert <creichert07@gmail.com>
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

#include "MetadataEditor.h"
#include "ui_MetadataEditor.h"

#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>

#include "Result.h"
#include "Artist.h"
#include "Album.h"
#include "Typedefs.h"
#include "ScanManager.h"
#include "PlaylistInterface.h"
#include "AlbumPlaylistInterface.h"

#include "taglib/fileref.h"
#include "filemetadata/taghandlers/tag.h"
#include "utils/TomahawkUtils.h"
#include "utils/Closure.h"


MetadataEditor::MetadataEditor( const Tomahawk::result_ptr& result, const Tomahawk::playlistinterface_ptr& interface, QWidget* parent )
    : QDialog( parent )
    , ui( new Ui::MetadataEditor )
    , m_result( result )
    , m_interface( interface )
    , m_index( 0 )
{
    ui->setupUi( this );
    setWindowTitle( QString( result->track() + tr( " - Properties" ) ) );
    setAttribute( Qt::WA_DeleteOnClose );

    NewClosure( ui->buttonBox, SIGNAL( accepted() ),
                this, SLOT( writeMetadata( bool ) ), true )->setAutoDelete( false );

    connect( ui->buttonBox, SIGNAL( rejected() ), SLOT( close() ) );
    connect( ui->forwardPushButton, SIGNAL( clicked() ), SLOT( loadNextResult() ) );
    connect( ui->previousPushButton, SIGNAL( clicked() ), SLOT( loadPreviousResult() ) );

    loadResult( result );
}


void
MetadataEditor::writeMetadata( bool closeDlg )
{
    QFileInfo fi( QUrl( m_result->url() ).toLocalFile() );

    bool changed = false;
    QByteArray fileName = QFile::encodeName( fi.canonicalFilePath() );
    const char *encodedName = fileName.constData();

    TagLib::FileRef f( encodedName );
    Tomahawk::Tag* tag = Tomahawk::Tag::fromFile( f );

    if ( title() != m_result->track() )
    {
        tag->setTitle( title() );
        m_result->setTrack( title() );

        changed = true;
    }

    Tomahawk::artist_ptr newArtist = Tomahawk::Artist::get( artist(), true );
    if ( newArtist != m_result->artist() )
    {
        tag->setArtist( artist() );
        m_result->setArtist( newArtist );

        changed = true;

    }

    Tomahawk::album_ptr newAlbum = Tomahawk::Album::get( newArtist, album(), true );
    if ( newAlbum != m_result->album() )
    {
        tag->setAlbum( album() );
        m_result->setAlbum( newAlbum );

        changed = true;
    }

    if ( albumPos() != (int)m_result->albumpos() )
    {
        tag->setTrack( albumPos() );
        m_result->setAlbumPos( albumPos() );

        changed = true;
    }

    if ( year() != m_result->year() )
    {
        tag->setYear( year() );
        m_result->setYear( year() );

        changed = true;
    }

    if ( changed )
    {
        f.save();

        m_editFiles.append( fileName );
        m_result->doneEditing();

        tDebug() << Q_FUNC_INFO << m_result->toString();
        tDebug() << Q_FUNC_INFO << m_result->toQuery()->toString();
    }

    if ( closeDlg )
    {
        ScanManager::instance()->runFileScan( m_editFiles, false );
        close();
    }
}


void
MetadataEditor::loadResult( const Tomahawk::result_ptr& result )
{
    if ( result.isNull() )
        return;

    m_result = result;
    setTitle( result->track() );
    setArtist( result->artist()->name() );
    setAlbum( result->album()->name() );
    setAlbumPos( result->albumpos() );
    setDuration( result->duration() );
    setYear( result->year() );
    setBitrate( result->bitrate() );

    QFileInfo fi( QUrl( m_result->url() ).toLocalFile() );
    setFileName( fi.fileName() );
    setFileSize( TomahawkUtils::filesizeToString( fi.size() ) );

    if ( m_interface )
    {
        m_index = m_interface->indexOfResult( result );

        if ( m_index >= 0 )
            enablePushButtons();
    }
}


void
MetadataEditor::enablePushButtons()
{
    if ( m_interface->itemAt( m_index + 1 ) )
        ui->forwardPushButton->setEnabled( true );
    else
        ui->forwardPushButton->setEnabled( false );

    if ( m_interface->itemAt( m_index - 1 ) )
        ui->previousPushButton->setEnabled( true );
    else
        ui->previousPushButton->setEnabled( false );
}


void
MetadataEditor::loadNextResult()
{
    writeMetadata();

    m_index++;

    if ( m_interface->itemAt( m_index )->numResults() )
        loadResult( m_interface->itemAt( m_index )->results().first() );
}


void
MetadataEditor::loadPreviousResult()
{
    writeMetadata();

    m_index--;

    if ( m_interface->itemAt( m_index )->numResults() )
        loadResult( m_interface->itemAt( m_index )->results().first() );
}


void
MetadataEditor::setTitle( const QString& title )
{
    ui->titleLineEdit->setText( title );
}


void
MetadataEditor::setArtist( const QString& artist )
{
    ui->artistLineEdit->setText( artist );
}


void
MetadataEditor::setAlbum( const QString& album )
{
    ui->albumLineEdit->setText( album );
}


void
MetadataEditor::setAlbumPos( unsigned int num )
{
    ui->albumPosSpinBox->setValue( num );
}


void
MetadataEditor::setDuration( unsigned int duration )
{
    ui->durationLineEdit->setText( TomahawkUtils::timeToString( duration ) );
}


void
MetadataEditor::setYear( int year )
{
    ui->yearSpinBox->setValue( year );
}


void
MetadataEditor::setBitrate( unsigned int bitrate )
{
    ui->bitrateSpinBox->setValue( bitrate );
}


void
MetadataEditor::setFileName( const QString& fn )
{
    ui->fileNameLineEdit->setText( fn );
}


void
MetadataEditor::setFileSize( const QString& size )
{
    ui->fileSizeLineEdit->setText( size );
}

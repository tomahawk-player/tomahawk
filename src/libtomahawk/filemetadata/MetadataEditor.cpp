/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christopher Reichert <creichert07@gmail.com>
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "Source.h"
#include "Result.h"
#include "Artist.h"
#include "Album.h"
#include "Track.h"
#include "Query.h"
#include "collection/Collection.h"
#include "ScanManager.h"
#include "PlaylistInterface.h"
#include "AlbumPlaylistInterface.h"

#include "filemetadata/taghandlers/tag.h"
#include "utils/TomahawkUtils.h"
#include "utils/Closure.h"
#include "utils/Logger.h"
#include "taglib/fileref.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFile>


MetadataEditor::MetadataEditor( const Tomahawk::query_ptr& query, const Tomahawk::playlistinterface_ptr& plInterface, QWidget* parent )
    : QDialog( parent )
{
    init( plInterface );

    loadQuery( query );
}


MetadataEditor::MetadataEditor( const Tomahawk::result_ptr& result, const Tomahawk::playlistinterface_ptr& plInterface, QWidget* parent )
    : QDialog( parent )
{
    init( plInterface );

    loadResult( result );
}


void
MetadataEditor::init( const Tomahawk::playlistinterface_ptr& plInterface )
{
    ui = new Ui::MetadataEditor();
    ui->setupUi( this );

    setAttribute( Qt::WA_DeleteOnClose );

    m_interface = plInterface;
    m_index = 0;
    m_editable = false;

    NewClosure( ui->buttonBox, SIGNAL( accepted() ), this, SLOT( writeMetadata( bool ) ), true )->setAutoDelete( false );

    connect( ui->buttonBox, SIGNAL( rejected() ), SLOT( close() ) );
    connect( ui->forwardPushButton, SIGNAL( clicked() ), SLOT( loadNextQuery() ) );
    connect( ui->previousPushButton, SIGNAL( clicked() ), SLOT( loadPreviousQuery() ) );
}


void
MetadataEditor::writeMetadata( bool closeDlg )
{
    if ( m_result )
    {
        QFileInfo fi( QUrl( m_result->url() ).toLocalFile() );

        bool changed = false;
        QByteArray fileName = QFile::encodeName( fi.canonicalFilePath() );
        const char *encodedName = fileName.constData();

        TagLib::FileRef f( encodedName );
        Tomahawk::Tag* tag = Tomahawk::Tag::fromFile( f );

        if ( title() != m_result->track()->track() )
        {
            tDebug() << Q_FUNC_INFO << "Track changed" << title() << m_result->track();

            tag->setTitle( title() );
            //FIXME
//            m_result->track()->setTrack( title() );

            changed = true;
        }

        Tomahawk::artist_ptr newArtist = Tomahawk::Artist::get( artist(), true );
        if ( newArtist != m_result->track()->artistPtr() )
        {
            tDebug() << Q_FUNC_INFO << "Artist changed" << artist() << m_result->track()->artist();

            tag->setArtist( artist() );
            //FIXME
//            m_result->track()->setArtist( artist() );

            changed = true;
        }

        Tomahawk::album_ptr newAlbum = Tomahawk::Album::get( newArtist, album(), true );
        if ( newAlbum != m_result->track()->albumPtr() )
        {
            tDebug() << Q_FUNC_INFO << "Album changed" << album() << newAlbum->id() << m_result->track()->album() << m_result->track()->albumPtr()->id() << newAlbum.data() << m_result->track()->albumPtr().data();
            if ( newAlbum->id() != m_result->track()->albumPtr()->id() )
            {
                tag->setAlbum( album() );
                m_result->track()->setAlbum( album() );

                changed = true;
            }
            else
                Q_ASSERT( false );
        }

        // FIXME: Ugly workaround for the min value of 0
        if ( albumPos() != 0 && albumPos() != (int)m_result->track()->albumpos() )
        {
            tag->setTrack( albumPos() );
            //FIXME:m_result->track()->setAlbumPos( albumPos() );

            tDebug() << Q_FUNC_INFO << "Albumpos changed";
            changed = true;
        }

        // FIXME: Ugly workaround for the min value of 1900
        if ( year() != 1900 && year() != m_result->track()->year() )
        {
            tag->setYear( year() );
            {
                QVariantMap attr;
                attr[ "releaseyear" ] = year();
                //FIXME
//                m_result->track()->setAttributes( attr );
            }

            tDebug() << Q_FUNC_INFO << "Year changed";
            changed = true;
        }

        if ( changed )
        {
            f.save();

            m_editFiles.append( fileName );
            m_result->doneEditing();

            tDebug() << Q_FUNC_INFO << m_result->toString() << m_result->track()->toString();
        }
    }

    if ( closeDlg )
    {
        if ( m_editFiles.count() )
            ScanManager::instance()->runFileScan( m_editFiles, false );

        close();
    }
}


void
MetadataEditor::loadQuery( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    if ( query->numResults() )
    {
        loadResult( query->results().first() );
        return;
    }

    m_result = Tomahawk::result_ptr();
    m_query = query;
    setEditable( false );

    setTitle( query->track()->track() );
    setArtist( query->track()->artist() );
    setAlbum( query->track()->album() );
    setAlbumPos( query->track()->albumpos() );
    setDuration( query->track()->duration() );
    setYear( 0 );
    setBitrate( 0 );

    setFileName( QString() );
    setFileSize( 0 );

    setWindowTitle( query->track()->track() );

    if ( m_interface )
    {
        m_index = m_interface->indexOfQuery( query );

        if ( m_index >= 0 )
            enablePushButtons();
    }
}


void
MetadataEditor::loadResult( const Tomahawk::result_ptr& result )
{
    if ( result.isNull() )
        return;

    m_result = result;
    setEditable( result->collection() && result->collection()->source()->isLocal() );

    setTitle( result->track()->track() );
    setArtist( result->track()->artist() );
    setAlbum( result->track()->album() );
    setAlbumPos( result->track()->albumpos() );
    setDuration( result->track()->duration() );
    setYear( result->track()->year() );
    setBitrate( result->bitrate() );

    if ( result->collection() && result->collection()->source()->isLocal() )
    {
        QString furl = m_result->url();
        if ( furl.startsWith( "file://" ) )
            furl = furl.right( furl.length() - 7 );

        QFileInfo fi( furl );
        setFileName( fi.absoluteFilePath() );
        setFileSize( TomahawkUtils::filesizeToString( fi.size() ) );
    }

    setWindowTitle( result->track()->track() );

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
    if ( m_interface->siblingIndex( 1, m_index ) > 0 )
        ui->forwardPushButton->setEnabled( true );
    else
        ui->forwardPushButton->setEnabled( false );

    if ( m_interface->siblingIndex( -1, m_index ) > 0 )
        ui->previousPushButton->setEnabled( true );
    else
        ui->previousPushButton->setEnabled( false );
}


void
MetadataEditor::loadNextQuery()
{
    writeMetadata();

    if ( m_interface->siblingIndex( 1, m_index ) > 0 )
    {
        m_index = m_interface->siblingIndex( 1, m_index );
        loadQuery( m_interface->queryAt( m_index ) );
    }
}


void
MetadataEditor::loadPreviousQuery()
{
    writeMetadata();

    if ( m_interface->siblingIndex( -1, m_index ) > 0 )
    {
        m_index = m_interface->siblingIndex( -1, m_index );
        loadQuery( m_interface->queryAt( m_index ) );
    }
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


void
MetadataEditor::setEditable( bool editable )
{
    ui->artistLineEdit->setReadOnly( !editable );
    ui->albumLineEdit->setReadOnly( !editable );
    ui->titleLineEdit->setReadOnly( !editable );
    ui->albumPosSpinBox->setReadOnly( !editable );
    ui->yearSpinBox->setReadOnly( !editable );

    m_editable = editable;
}


void
MetadataEditor::setWindowTitle( const QString& title )
{
    QDialog::setWindowTitle( title + " - " + tr( "Properties" ) );
}

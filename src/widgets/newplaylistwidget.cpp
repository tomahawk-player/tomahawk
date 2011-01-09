#include "newplaylistwidget.h"
#include "ui_newplaylistwidget.h"

#include <QPushButton>
#include <QDialogButtonBox>

#include "tomahawk/tomahawkapp.h"
#include "utils/tomahawkutils.h"

#include "playlist/playlistmanager.h"
#include "playlist/playlistmodel.h"

#include "pipeline.h"
#include "xspfloader.h"

#include "sourcelist.h"

#define FILTER_TIMEOUT 280


NewPlaylistWidget::NewPlaylistWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::NewPlaylistWidget )
{
    ui->setupUi( this );

    QPushButton* saveButton = new QPushButton( tr( "&Create Playlist" ) );
    saveButton->setDefault( true );

    ui->buttonBox->addButton( saveButton, QDialogButtonBox::AcceptRole );

    connect( ui->tagEdit, SIGNAL( textChanged( QString ) ), SLOT( tagChanged() ) );
    connect( ui->buttonBox, SIGNAL( accepted() ), SLOT( savePlaylist() ) );
    connect( ui->buttonBox, SIGNAL( rejected() ), SLOT( cancel() ) );

    m_suggestionsModel = new PlaylistModel( ui->suggestionsView );
    ui->suggestionsView->setModel( m_suggestionsModel );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( updateSuggestions() ) );
}


NewPlaylistWidget::~NewPlaylistWidget()
{
    delete ui;
}


void
NewPlaylistWidget::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
NewPlaylistWidget::tagChanged()
{
    m_tag = ui->tagEdit->text();

    m_filterTimer.stop();
    m_filterTimer.setInterval( FILTER_TIMEOUT );
    m_filterTimer.setSingleShot( true );
    m_filterTimer.start();
}


void
NewPlaylistWidget::updateSuggestions()
{
    QUrl url( QString( "http://ws.audioscrobbler.com/1.0/tag/%1/toptracks.xspf" ).arg( m_tag ) );

    XSPFLoader* loader = new XSPFLoader( false );
    connect( loader, SIGNAL( ok( Tomahawk::playlist_ptr ) ), SLOT( suggestionsFound() ) );

    loader->load( url );
}


void
NewPlaylistWidget::suggestionsFound()
{
    XSPFLoader* loader = qobject_cast<XSPFLoader*>( sender() );

    m_entries = loader->entries();

    delete m_suggestionsModel;
    m_suggestionsModel = new PlaylistModel( ui->suggestionsView );
    ui->suggestionsView->setModel( m_suggestionsModel );

    QList<Tomahawk::query_ptr> ql;
    foreach( const Tomahawk::plentry_ptr& entry, m_entries )
    {
        m_suggestionsModel->appendTrack( entry->query() );
        ql.append( entry->query() );
    }

    Tomahawk::Pipeline::instance()->add( ql );

    loader->deleteLater();
}


void
NewPlaylistWidget::savePlaylist()
{
    Tomahawk::playlist_ptr playlist;

    playlist = Tomahawk::Playlist::create( SourceList::instance()->getLocal(), uuid(), ui->titleEdit->text(), "", "", false );
    playlist->createNewRevision( uuid(), playlist->currentrevision(), m_entries );

    APP->playlistManager()->show( playlist );
    cancel();
}


void
NewPlaylistWidget::cancel()
{
    emit destroyed( this );
    deleteLater();
}

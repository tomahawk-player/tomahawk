#include "sourceinfowidget.h"
#include "ui_sourceinfowidget.h"

#include "utils/tomahawkutils.h"

#include "playlist/playlistmanager.h"
#include "playlist/albummodel.h"
#include "playlist/collectionflatmodel.h"
#include "playlist/playlistmodel.h"

#include "database/databasecommand_alltracks.h"
#include "database/databasecommand_allalbums.h"

#include "widgets/overlaywidget.h"


SourceInfoWidget::SourceInfoWidget( const Tomahawk::source_ptr& source, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SourceInfoWidget )
{
    ui->setupUi( this );

    ui->historyView->overlay()->setEnabled( false );

    m_recentCollectionModel = new CollectionFlatModel( ui->recentCollectionView );
    ui->recentCollectionView->setModel( m_recentCollectionModel );
    m_recentCollectionModel->addFilteredCollection( source->collection(), 250, DatabaseCommand_AllTracks::ModificationTime );

    m_historyModel = new PlaylistModel( ui->historyView );
    ui->historyView->setModel( m_historyModel );
    m_historyModel->loadHistory( source );

    connect( source.data(), SIGNAL( playbackFinished( Tomahawk::query_ptr ) ), SLOT( onPlaybackFinished( Tomahawk::query_ptr ) ) );

    ui->recentCollectionView->setColumnHidden( TrackModel::Bitrate, true );
    ui->recentCollectionView->setColumnHidden( TrackModel::Origin, true );
    ui->recentCollectionView->setColumnHidden( TrackModel::Filesize, true );

    ui->historyView->setColumnHidden( TrackModel::Bitrate, true );
    ui->historyView->setColumnHidden( TrackModel::Origin, true );
    ui->historyView->setColumnHidden( TrackModel::Filesize, true );

    m_recentAlbumModel = new AlbumModel( ui->recentAlbumView );
    ui->recentAlbumView->setModel( m_recentAlbumModel );
    m_recentAlbumModel->addFilteredCollection( source->collection(), 20, DatabaseCommand_AllAlbums::ModificationTime );

    m_title = tr( "Info about %1" ).arg( source->isLocal() ? tr( "Your Collection" ) : source->friendlyName() );
}


SourceInfoWidget::~SourceInfoWidget()
{
    delete ui;
}


void
SourceInfoWidget::onPlaybackFinished( const Tomahawk::query_ptr& query )
{
    m_historyModel->insert( 0, query );
}


void
SourceInfoWidget::changeEvent( QEvent* e )
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

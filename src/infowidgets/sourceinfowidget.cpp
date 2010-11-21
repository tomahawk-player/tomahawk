#include "sourceinfowidget.h"
#include "ui_sourceinfowidget.h"

#include "tomahawk/tomahawkapp.h"
#include "utils/tomahawkutils.h"

#include "playlist/playlistmanager.h"
#include "playlist/albummodel.h"
#include "playlist/collectionflatmodel.h"

#include "database/databasecommand_alltracks.h"
#include "database/databasecommand_allalbums.h"


SourceInfoWidget::SourceInfoWidget( const Tomahawk::source_ptr& source, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SourceInfoWidget )
{
    ui->setupUi( this );

    ui->sourceLabel->setText( source->friendlyName() );

    m_recentCollectionModel = new CollectionFlatModel( ui->recentCollectionView );
    ui->recentCollectionView->setModel( m_recentCollectionModel );
    m_recentCollectionModel->addFilteredCollection( source->collection(), 250, DatabaseCommand_AllTracks::ModificationTime );

//    ui->recentCollectionView->setColumnHidden( TrackModel::Bitrate, true );
//    ui->recentCollectionView->setColumnHidden( TrackModel::Origin, true );

    m_recentAlbumModel = new AlbumModel( ui->recentAlbumView );
    ui->recentAlbumView->setModel( m_recentAlbumModel );
    m_recentAlbumModel->addFilteredCollection( source->collection(), 20, DatabaseCommand_AllAlbums::ModificationTime );
}


SourceInfoWidget::~SourceInfoWidget()
{
    delete ui;
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

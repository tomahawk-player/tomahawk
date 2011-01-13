#include "sourcetreeitemwidget.h"

#include "ui_sourcetreeitemwidget.h"

#include "tomahawk/tomahawkapp.h"
#include "album.h"
#include "database/database.h"
#include "database/databasecommand_collectionstats.h"
#include "network/dbsyncconnection.h"
#include "playlistmanager.h"

using namespace Tomahawk;


SourceTreeItemWidget::SourceTreeItemWidget( const source_ptr& source, QWidget* parent ) :
    QWidget( parent ),
    m_source( source ),
    ui( new Ui::SourceTreeItemWidget )
{
//    qDebug() << Q_FUNC_INFO;

    ui->setupUi( this );
    ui->verticalLayout->setSpacing( 3 );
    ui->activityLabel->setType( QueryLabel::ArtistAndTrack );

    QFont font = ui->nameLabel->font();
//    font.setPointSize( font.pointSize() - 1 );
    ui->nameLabel->setFont( font );

    font.setPointSize( font.pointSize() - 1 );
    ui->infoLabel->setFont( font );
    ui->activityLabel->setFont( font );

    QString displayname;
    if ( source.isNull() )
    {
        ui->avatarImage->setPixmap( QPixmap( RESPATH "images/user-avatar.png" ) );

        displayname = tr( "Super Collection" );
        ui->infoLabel->setText( tr( "All available tracks" ) );

        ui->onOffButton->hide();
    }
    else
    {
        connect( source.data(), SIGNAL( loadingStateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ),
                                  SLOT( onLoadingStateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ) );

        connect( source.data(), SIGNAL( stats( QVariantMap ) ), SLOT( gotStats( QVariantMap ) ) );

        connect( source.data(), SIGNAL( playbackStarted( Tomahawk::query_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::query_ptr ) ) );

        ui->avatarImage->setPixmap( QPixmap( RESPATH "images/user-avatar.png" ) );

        displayname = source->friendlyName();
        if( displayname.isEmpty() )
            displayname = source->userName();

        ui->infoLabel->setText( "???" );

        ui->onOffButton->hide();
        ui->infoButton->setPixmap( QPixmap( RESPATH "images/source-info.png" ) .scaledToHeight( 32, Qt::SmoothTransformation ) );
    }

    ui->nameLabel->setText( displayname );
    ui->activityLabel->setText( tr( "Idle" ) );
    ui->infoLabel->setForegroundRole( QPalette::Dark );
    ui->activityLabel->setForegroundRole( QPalette::Dark );

    ui->nameLabel->setContentsMargins( 4, 0, 0, 0 );
    ui->infoLabel->setContentsMargins( 4, 0, 0, 0 );
    ui->activityLabel->setContentsMargins( 4, 0, 0, 0 );

    connect( ui->onOffButton, SIGNAL( clicked() ), SIGNAL( clicked() ) );
    connect( ui->infoButton, SIGNAL( clicked() ), SLOT( onInfoButtonClicked() ) );

    onOffline();
}


SourceTreeItemWidget::~SourceTreeItemWidget()
{
    qDebug() << Q_FUNC_INFO;
    delete ui;
}


void
SourceTreeItemWidget::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
//            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
SourceTreeItemWidget::gotStats( const QVariantMap& stats )
{
    ui->infoLabel->setText( tr( "%L1 tracks" ).arg( stats.value( "numfiles" ).toInt() ) );
}


void
SourceTreeItemWidget::onLoadingStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State, const QString& info )
{
    QString msg;
    switch( newstate )
    {
        case DBSyncConnection::CHECKING:
            msg = tr( "Checking" );
            break;
        case DBSyncConnection::FETCHING:
            msg = tr( "Fetching" );
            break;
        case DBSyncConnection::PARSING:
            msg = tr( "Parsing" );
            break;
        case DBSyncConnection::SAVING:
            msg = tr( "Saving" );
            break;
        case DBSyncConnection::SYNCED:
            msg = tr( "Synced" );
            break;
        case DBSyncConnection::SCANNING:
            msg = tr( "Scanning (%L1 tracks)" ).arg( info );
            break;

        default:
            msg = "???";
    }

    ui->infoLabel->setText( msg );
}


void
SourceTreeItemWidget::onPlaybackStarted( const Tomahawk::query_ptr& query )
{
    qDebug() << Q_FUNC_INFO << query->toString();
//    ui->activityLabel->setText( tr( "Playing: %1 by %2" ).arg( query->track() ).arg( query->artist() ) );
    ui->activityLabel->setQuery( query );
}


void
SourceTreeItemWidget::onOnline()
{
    return;

    if ( !m_source.isNull() )
        ui->onOffButton->setPixmap( RESPATH "images/source-on-rest.png" );
}


void
SourceTreeItemWidget::onOffline()
{
    return;

    if ( !m_source.isNull() )
        ui->onOffButton->setPixmap( RESPATH "images/source-off-rest.png" );
}


void
SourceTreeItemWidget::onInfoButtonClicked()
{
    APP->playlistManager()->show( m_source );
}

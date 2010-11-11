#include "sourcetreeitemwidget.h"

#include "ui_sourcetreeitemwidget.h"

#include "tomahawk/tomahawkapp.h"
#include "database.h"
#include "databasecommand_collectionstats.h"
#include "dbsyncconnection.h"

using namespace Tomahawk;


SourceTreeItemWidget::SourceTreeItemWidget( const source_ptr& source, QWidget* parent ) :
    QWidget( parent ),
    m_source( source ),
    ui( new Ui::SourceTreeItemWidget )
{
//    qDebug() << Q_FUNC_INFO;

    ui->setupUi( this );
    ui->verticalLayout->setSpacing( 3 );

    QString displayname;
    if ( source.isNull() )
    {
        ui->avatarImage->setPixmap( QPixmap( RESPATH "images/user-avatar.png" ) );

        displayname = tr( "Super Collection" );
        ui->infoLabel->setText( tr( "All available tracks" ) );
    }
    else
    {
        connect( source.data(), SIGNAL( loadingStateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ),
                                  SLOT( onLoadingStateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ) );

        connect( source.data(), SIGNAL( stats( QVariantMap ) ), SLOT( gotStats( QVariantMap ) ) );

        ui->avatarImage->setPixmap( QPixmap( RESPATH "images/user-avatar.png" ) );

        displayname = source->friendlyName();
        if( displayname.isEmpty() )
            displayname = source->userName();

        ui->infoLabel->setText( "???" );
    }

    ui->nameLabel->setText( displayname );
    ui->infoLabel->setForegroundRole( QPalette::Dark );

    connect( ui->onOffButton, SIGNAL( clicked() ), SIGNAL( clicked() ) );

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
SourceTreeItemWidget::onOnline()
{
    if ( !m_source.isNull() )
        ui->onOffButton->setPixmap( RESPATH "images/source-on-rest.png" );
}


void
SourceTreeItemWidget::onOffline()
{
    if ( !m_source.isNull() )
        ui->onOffButton->setPixmap( RESPATH "images/source-off-rest.png" );
}

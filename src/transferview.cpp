#include "transferview.h"

#include <QHeaderView>

#include "tomahawk/tomahawkapp.h"
#include "network/filetransferconnection.h"
#include "network/servent.h"


TransferView::TransferView( QWidget* parent )
    : QTreeWidget( parent )
{
    connect( &APP->servent(), SIGNAL( fileTransferStarted( FileTransferConnection* ) ), SLOT( fileTransferRegistered( FileTransferConnection* ) ) );
    connect( &APP->servent(), SIGNAL( fileTransferFinished( FileTransferConnection* ) ), SLOT( fileTransferFinished( FileTransferConnection* ) ) );

    QStringList headers;
    headers << tr( "Peer" ) << tr( "Rate" ) << tr( "Track" );
    setHeaderLabels( headers );

    setColumnCount( 3 );
    setColumnWidth( 0, 80 );
    setColumnWidth( 1, 65 );
    setColumnWidth( 2, 10 );

    header()->setStretchLastSection( true );
    setRootIsDecorated( false );
}


void
TransferView::fileTransferRegistered( FileTransferConnection* ftc )
{
    connect( ftc, SIGNAL( updated() ), SLOT( onTransferUpdate() ) );
}


void
TransferView::fileTransferFinished( FileTransferConnection* ftc )
{
    if ( !m_index.contains( ftc ) )
        return;

/*    int i = m_index.take( ftc );
    delete invisibleRootItem()->takeChild( i ); */

    if ( m_index.contains( ftc ) )
    {
        int i = m_index.value( ftc );
        invisibleRootItem()->child( i )->setText( 1, tr( "Finished" ) );
    }
}


void
TransferView::onTransferUpdate()
{
    FileTransferConnection* ftc = (FileTransferConnection*)sender();
    QTreeWidgetItem* ti = 0;

    if ( m_index.contains( ftc ) )
    {
        int i = m_index.value( ftc );
        ti = invisibleRootItem()->child( i );
    }
    else
    {
        ti = new QTreeWidgetItem( this );
        m_index.insert( ftc, invisibleRootItem()->childCount() - 1 );
    }

    ti->setText( 0, ftc->source()->friendlyName() );
    ti->setText( 1, QString( "%1 kb/s" ).arg( ftc->transferRate() / 1024 ) );
    ti->setText( 2, QString( "%1 - %2" ).arg( ftc->track()->artist()->name() ).arg( ftc->track()->track() ) );
}

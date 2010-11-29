#include "transferview.h"

#include <QHeaderView>
#include <QVBoxLayout>

#include "tomahawk/tomahawkapp.h"
#include "network/filetransferconnection.h"
#include "network/servent.h"


TransferView::TransferView( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
    , m_parent( parent )
{
    setHiddenSize( QSize( 0, 0 ) );
    setLayout( new QVBoxLayout() );
    m_tree = new QTreeWidget( this );

    layout()->setMargin( 0 );
    layout()->addWidget( m_tree );

    connect( &APP->servent(), SIGNAL( fileTransferStarted( FileTransferConnection* ) ), SLOT( fileTransferRegistered( FileTransferConnection* ) ) );
    connect( &APP->servent(), SIGNAL( fileTransferFinished( FileTransferConnection* ) ), SLOT( fileTransferFinished( FileTransferConnection* ) ) );

    QStringList headers;
    headers << tr( "Peer" ) << tr( "Rate" ) << tr( "Track" );
    m_tree->setHeaderLabels( headers );

    m_tree->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    m_tree->setColumnCount( 3 );
    m_tree->setColumnWidth( 0, 80 );
    m_tree->setColumnWidth( 1, 65 );
    m_tree->setColumnWidth( 2, 10 );

    m_tree->header()->setStretchLastSection( true );
    m_tree->setRootIsDecorated( false );
}


void
TransferView::fileTransferRegistered( FileTransferConnection* ftc )
{
    qDebug() << Q_FUNC_INFO;
    connect( ftc, SIGNAL( updated() ), SLOT( onTransferUpdate() ) );
    emit showWidget();
}


void
TransferView::fileTransferFinished( FileTransferConnection* ftc )
{
    if ( !m_index.contains( ftc ) )
        return;

    int i = m_index.take( ftc );
    delete m_tree->invisibleRootItem()->takeChild( i );

    if ( m_tree->invisibleRootItem()->childCount() > 0 )
        emit showWidget();
    else
        emit hideWidget();

/*    if ( m_index.contains( ftc ) )
    {
        int i = m_index.value( ftc );
        m_tree->invisibleRootItem()->child( i )->setText( 1, tr( "Finished" ) );
    }*/
}


void
TransferView::onTransferUpdate()
{
    FileTransferConnection* ftc = (FileTransferConnection*)sender();
    qDebug() << Q_FUNC_INFO << ftc->track().isNull() << ftc->source().isNull();
    if ( ftc->track().isNull() || ftc->source().isNull() )
        return;

    QTreeWidgetItem* ti = 0;

    if ( m_index.contains( ftc ) )
    {
        int i = m_index.value( ftc );
        ti = m_tree->invisibleRootItem()->child( i );
    }
    else
    {
        ti = new QTreeWidgetItem( m_tree );
        m_index.insert( ftc, m_tree->invisibleRootItem()->childCount() - 1 );
    }

    if ( !ti )
        return;

    ti->setText( 0, ftc->source()->friendlyName() );
    ti->setText( 1, QString( "%1 kb/s" ).arg( ftc->transferRate() / 1024 ) );
    ti->setText( 2, QString( "%1 - %2" ).arg( ftc->track()->artist()->name() ).arg( ftc->track()->track() ) );

    if ( isHidden() )
        emit showWidget();
}


QSize
TransferView::sizeHint() const
{
    unsigned int y = 0;
    y += m_tree->header()->height();
    y += m_tree->contentsMargins().top() + m_tree->contentsMargins().bottom();

    if ( m_tree->invisibleRootItem()->childCount() )
    {
        unsigned int rowheight = m_tree->sizeHintForRow( 0 );
        y += rowheight * m_tree->invisibleRootItem()->childCount() + 2;
    }

    return QSize( 0, y );
}

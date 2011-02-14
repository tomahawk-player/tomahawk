#include "trackheader.h"

#include <QContextMenuEvent>
#include <QDebug>
#include <QMenu>

#include "tomahawksettings.h"
#include "playlist/trackmodel.h"
#include "playlist/trackview.h"


TrackHeader::TrackHeader( TrackView* parent )
    : QHeaderView( Qt::Horizontal, parent )
    , m_parent( parent )
    , m_menu( new QMenu( this ) )
    , m_sigmap( new QSignalMapper( this ) )
    , m_init( false )
{
    setResizeMode( QHeaderView::Interactive );
    setMinimumSectionSize( 60 );
    setDefaultAlignment( Qt::AlignLeft );
    setMovable( true );
    setStretchLastSection( true );
//    setCascadingSectionResizes( true );

//    m_menu->addAction( tr( "Resize columns to fit window" ), this, SLOT( onToggleResizeColumns() ) );
//    m_menu->addSeparator();

//    connect( this, SIGNAL( sectionResized( int, int, int ) ), SLOT( onSectionResized( int ) ) );
    connect( m_sigmap, SIGNAL( mapped( int ) ), SLOT( toggleVisibility( int ) ) );
}


TrackHeader::~TrackHeader()
{
    qDebug() << "Storing for:" << m_parent->guid();
    TomahawkSettings::instance()->setPlaylistColumnSizes( m_parent->guid(), saveState() );
}


int
TrackHeader::visibleSectionCount() const
{
    return count() - hiddenSectionCount();
}


void
TrackHeader::checkState()
{
    if ( !count() || m_init )
        return;

    qDebug() << "Restoring for:" << m_parent->guid();
    m_init = true;
    QByteArray state = TomahawkSettings::instance()->playlistColumnSizes( m_parent->guid() );

    if ( !state.isEmpty() )
        restoreState( state );
    else
    {
        QList< double > m_columnWeights;
        m_columnWeights << 0.21 << 0.22 << 0.20 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05; // << 0.12;

        for ( int i = 0; i < count() - 1; i++ )
        {
            if ( isSectionHidden( i ) )
                continue;

            double nw = (double)m_parent->width() * m_columnWeights.at( i );
            qDebug() << "Setting default size:" << i << nw;
            resizeSection( i, qMax( minimumSectionSize(), int( nw - 0.5 ) ) );
        }
    }
}


void
TrackHeader::addColumnToMenu( int index )
{
    QString title = m_parent->model()->headerData( index, Qt::Horizontal, Qt::DisplayRole ).toString();

    QAction* action = m_menu->addAction( title, m_sigmap, SLOT( map() ) );
    action->setCheckable( true );
    action->setChecked( !isSectionHidden( index ) );
    m_visActions << action;

    m_sigmap->setMapping( action, index );
}


void
TrackHeader::contextMenuEvent( QContextMenuEvent* e )
{
    qDeleteAll( m_visActions );
    m_visActions.clear();

    for ( int i = 0; i < count(); i++ )
        addColumnToMenu( i );

    m_menu->popup( e->globalPos() );
}


void
TrackHeader::onToggleResizeColumns()
{
}


void
TrackHeader::toggleVisibility( int index )
{
    qDebug() << Q_FUNC_INFO << index;

    if ( isSectionHidden( index ) )
        showSection( index );
    else
        hideSection( index );
}

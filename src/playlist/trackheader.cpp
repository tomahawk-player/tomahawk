#include "trackheader.h"

#include <QContextMenuEvent>
#include <QDebug>
#include <QMenu>

#include "tomahawk/tomahawkapp.h"
#include "tomahawksettings.h"
#include "playlist/trackmodel.h"
#include "playlist/trackview.h"


TrackHeader::TrackHeader( TrackView* parent )
    : QHeaderView( Qt::Horizontal, parent )
    , m_parent( parent )
    , m_menu( new QMenu( this ) )
    , m_sigmap( new QSignalMapper( this ) )
    , m_hiddenWidth( 0 )
    , m_hiddenPct( 0.0 )
    , m_init( false )
{
    setStretchLastSection( false );
    setResizeMode( QHeaderView::Interactive );
    setMinimumSectionSize( 60 );
    setDefaultAlignment( Qt::AlignLeft );
    setMovable( true );
    setCascadingSectionResizes( true );

    m_menu->addAction( tr( "Resize columns to fit window" ), this, SLOT( onToggleResizeColumns() ) );
    m_menu->addSeparator();

    connect( this, SIGNAL( sectionResized( int, int, int ) ), SLOT( onSectionResized( int, int, int ) ) );
    connect( m_sigmap, SIGNAL( mapped( int ) ), SLOT( toggleVisibility( int ) ) );
}


TrackHeader::~TrackHeader()
{
    saveColumnsState();
}


void
TrackHeader::onSectionResized( int logicalidx, int oldSize, int newSize )
{
    if ( !m_init )
        return;

    int width = m_parent->viewport()->width();
    for ( int x = 0; x < m_columnWeights.count(); x++ )
    {
        if ( sectionSize( x ) )
        {
            // not hidden
            m_columnWeights[x] = (double)sectionSize( x ) / (double)width;
        }
    }
}


void
TrackHeader::onResized()
{
    if ( !m_init && count() )
        restoreColumnsState();

    m_init = false;

    int width = m_parent->viewport()->width();
    for ( int x = 0; x < m_columnWeights.count(); x++ )
    {
        if ( sectionSize( x ) )
        {
            // not hidden
            resizeSection( x, int( (double)width * m_columnWeights[x] ) );
        }
    }

    m_init = true;
}


void
TrackHeader::restoreColumnsState()
{
    QList<QVariant> list = TomahawkSettings::instance()->playlistColumnSizes();

    if ( list.count() != count() ) // FIXME: const
    {
        m_columnWeights << 0.21 << 0.22 << 0.20 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05 << 0.12;
    }
    else
    {
        foreach( const QVariant& v, list )
            m_columnWeights << v.toDouble();
    }
}


void
TrackHeader::saveColumnsState()
{
    QList<QVariant> wlist;

    foreach( double w, m_columnWeights )
        wlist << QVariant( w );

    TomahawkSettings::instance()->setPlaylistColumnSizes( wlist );
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

    onResized();
}

#include "dropmenu.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDragEnterEvent>



DropMenu::DropMenu( QWidget *parent )
    : QWidget( parent )
{
    setLayout( new QHBoxLayout() );
    layout()->setSpacing( 0 );
}


void
DropMenu::addEntry( DropMenuEntry *entry, bool isDefault )
{

    layout()->addWidget( entry );

    if( m_entries.isEmpty() || isDefault )
        m_activeEntry = entry;

    m_entries.append( entry );
    connect( entry, SIGNAL( mouseEntered( DropMenuEntry* ) ), this, SLOT( entryHovered( DropMenuEntry* ) ) );
    connect( entry, SIGNAL( mouseLeft( DropMenuEntry* ) ), this, SLOT( entryLeft(DropMenuEntry*) ) );
    connect( entry, SIGNAL( dropReceived( QDropEvent* ) ) , this, SIGNAL( dropReceived( QDropEvent* ) ) );

    if( isDefault )
        m_defaultEntry = entry;
}


bool
DropMenu::hovered() const
{
    foreach( DropMenuEntry *entry, m_entries )
    {
        if( entry->hovered() )
            return true;
    }
    return false;
}


DropMenuEntry*
DropMenu::activeEntry()
{
    return m_activeEntry;
}


void
DropMenu::entryHovered( DropMenuEntry *entry )
{
    m_activeEntry->setActive( false );
    m_activeEntry = entry;
    entry->setActive( true );
}


void
DropMenu::entryLeft( DropMenuEntry *entry )
{
    entry->setActive( false );
    m_defaultEntry->setActive( true );
    m_activeEntry = m_defaultEntry;
}


DropMenuEntry::DropMenuEntry( const QPixmap &icon, const QString &text, DropJob::DropFlags flags, QWidget *parent )
    : QWidget( parent )
    , m_hovered( false )
    , m_flags( flags )
{

    QVBoxLayout *layout = new QVBoxLayout();

    QLabel* image = new QLabel;
    image->setAlignment( Qt::AlignHCenter );
    image->setPixmap( icon );
    layout->addWidget( image );

    m_label = new QLabel( text );
    m_label->setAlignment( Qt::AlignHCenter );
    layout->addWidget( m_label );

    setLayout( layout );

    setAcceptDrops( true );
    setMouseTracking( true );

}

void
DropMenuEntry::dragEnterEvent( QDragEnterEvent *event )
{
    event->acceptProposedAction();
    emit mouseEntered( this );
    m_hovered = true;
}


void
DropMenuEntry::dragLeaveEvent( QDragLeaveEvent *event )
{
    emit mouseLeft( this );
    m_hovered = false;
}

void
DropMenuEntry::setActive( bool active )
{
    QFont font = m_label->font();
    font.setBold( active );
    m_label->setFont( font );
}


void
DropMenuEntry::dropEvent( QDropEvent *event )
{
    emit dropReceived( event );
    m_hovered = false;
}


bool
DropMenuEntry::hovered() const
{
    return m_hovered;
}


DropJob::DropFlags
DropMenuEntry::dropFlags() const
{
    return m_flags;
}

#include "querylabel.h"

#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

#include "tomahawkutils.h"
#include "artist.h"
#include "album.h"
#include "query.h"


#define BOXMARGIN 2
#define DASH "  -  "

QueryLabel::QueryLabel( QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
    , m_type( Complete )
{
    init();
}


QueryLabel::QueryLabel( DisplayType type, QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
    , m_type( type )
{
    init();
}


QueryLabel::QueryLabel( const Tomahawk::result_ptr& result, DisplayType type, QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
    , m_type( type )
    , m_result( result )
{
    init();
}


QueryLabel::QueryLabel( const Tomahawk::query_ptr& query, DisplayType type, QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
    , m_type( type )
    , m_query( query )
{
    init();
}


QueryLabel::~QueryLabel()
{
}


void
QueryLabel::init()
{
    setContentsMargins( 0, 0, 0, 0 );
    setMouseTracking( true );

    align = Qt::AlignLeft;
    mode = Qt::ElideMiddle;
}


QString
QueryLabel::text() const
{
    QString text;

    if ( m_result.isNull() && m_query.isNull() )
        return m_text;

    if ( !m_result.isNull() )
    {
        if ( m_type & Artist )
        {
            text += m_result->artist()->name();
        }
        if ( m_type & Album )
        {
            smartAppend( text, m_result->album()->name() );
        }
        if ( m_type & Track )
        {
            smartAppend( text, m_result->track() );
        }
    }
    else
    {
        if ( m_type & Artist )
        {
            text += m_query->artist();
        }
        if ( m_type & Album )
        {
            smartAppend( text, m_query->album() );
        }
        if ( m_type & Track )
        {
            smartAppend( text, m_query->track() );
        }
    }

    return text;
}


QString
QueryLabel::artist() const
{
    if ( m_result.isNull() && m_query.isNull() )
        return QString();

    if ( !m_result.isNull() )
        return m_result->artist()->name();
    else
        return m_query->artist();
}


QString
QueryLabel::album() const
{
    if ( m_result.isNull() && m_query.isNull() )
        return QString();

    if ( !m_result.isNull() )
        return m_result->album()->name();
    else
        return m_query->album();
}


QString
QueryLabel::track() const
{
    if ( m_result.isNull() && m_query.isNull() )
        return QString();

    if ( !m_result.isNull() )
        return m_result->track();
    else
        return m_query->track();
}


void
QueryLabel::setText( const QString& text )
{
    setContentsMargins( m_textMargins );

    m_result.clear();
    m_query.clear();
    m_text = text;

    updateLabel();

    emit textChanged( m_text );
    emit resultChanged( m_result );
}


void
QueryLabel::setResult( const Tomahawk::result_ptr& result )
{
    if ( result.isNull() )
        return;

    if ( !m_text.isEmpty() && contentsMargins().left() != 0 ) // FIXME: hacky
        m_textMargins = contentsMargins();

    setContentsMargins( BOXMARGIN * 2, BOXMARGIN / 2, BOXMARGIN * 2, BOXMARGIN / 2);

    if ( m_result.isNull() || m_result.data() != result.data() )
    {
        m_result = result;

        m_query = m_result->toQuery();
        QList<Tomahawk::result_ptr> rl;
        rl << m_result;
        m_query->addResults( rl );

        updateLabel();

        emit textChanged( text() );
        emit resultChanged( m_result );
    }
}


void
QueryLabel::setQuery( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    setContentsMargins( BOXMARGIN * 2, BOXMARGIN / 2, BOXMARGIN * 2, BOXMARGIN / 2 );

    if ( m_query.isNull() || m_query.data() != query.data() )
    {
        m_query = query;
        m_result.clear();
        updateLabel();

        emit textChanged( text() );
        emit queryChanged( m_query );
    }
}


Qt::Alignment
QueryLabel::alignment() const
{
    return align;
}


void
QueryLabel::setAlignment( Qt::Alignment alignment )
{
    if ( this->align != alignment )
    {
        this->align = alignment;
        update(); // no geometry change, repaint is sufficient
    }
}


Qt::TextElideMode
QueryLabel::elideMode() const
{
    return mode;
}


void
QueryLabel::setElideMode( Qt::TextElideMode mode )
{
    if ( this->mode != mode )
    {
        this->mode = mode;
        updateLabel();
    }
}


void
QueryLabel::updateLabel()
{
    m_hoverArea = QRect();

    updateGeometry();
    update();
}


QSize
QueryLabel::sizeHint() const
{
    const QFontMetrics& fm = fontMetrics();
    QSize size( fm.width( text() ) + contentsMargins().left() * 2, fm.height() + contentsMargins().top() * 2 );
    return size;
}


QSize
QueryLabel::minimumSizeHint() const
{
    switch ( mode )
    {
        case Qt::ElideNone:
            return sizeHint();

        default:
        {
            const QFontMetrics& fm = fontMetrics();
            QSize size( fm.width( "..." ), fm.height() + contentsMargins().top() * 2  );
            return size;
        }
    }
}


void
QueryLabel::paintEvent( QPaintEvent* event )
{
    QFrame::paintEvent( event );
    QPainter p( this );
    QRect r = contentsRect();
    QString s = text();
    const QString elidedText = fontMetrics().elidedText( s, mode, r.width() );

    p.save();
    p.setRenderHint( QPainter::Antialiasing );

    if ( m_hoverArea.width() )
    {
        if ( elidedText != s )
        {
            m_hoverArea.setLeft( 0 );
            m_hoverArea.setRight( fontMetrics().width( elidedText ) + contentsMargins().left() * 2 );
        }

        p.setPen( palette().mid().color() );
        p.setBrush( palette().highlight() );
        p.drawRoundedRect( m_hoverArea, 4.0, 4.0 );
    }

    if ( elidedText != s || ( m_result.isNull() && m_query.isNull() ) )
    {
        p.setBrush( palette().window() );
        p.setPen( palette().color( foregroundRole() ) );
        p.drawText( r, align, elidedText );
    }
    else
    {
        const QFontMetrics& fm = fontMetrics();
        int dashX = fm.width( DASH );
        int artistX = m_type & Artist ? fm.width( artist() ) : 0;
        int albumX = m_type & Album ? fm.width( album() ) : 0;
        int trackX = m_type & Track ? fm.width( track() ) : 0;

        if ( m_type & Artist )
        {
            p.setBrush( palette().window() );
            p.setPen( palette().color( foregroundRole() ) );

            if ( m_hoverArea.width() && m_hoverArea.left() + contentsMargins().left() == r.left() )
            {
                p.setPen( palette().highlightedText().color() );
                p.setBrush( palette().highlight() );
            }

            p.drawText( r, align, artist() );
            r.adjust( artistX, 0, 0, 0 );
        }
        if ( m_type & Album )
        {
            p.setBrush( palette().window() );
            p.setPen( palette().color( foregroundRole() ) );

            if ( m_type & Artist )
            {
                p.drawText( r, align, DASH );
                r.adjust( dashX, 0, 0, 0 );
            }
            if ( m_hoverArea.width() && m_hoverArea.left() + contentsMargins().left() == r.left() )
            {
                p.setPen( palette().highlightedText().color() );
                p.setBrush( palette().highlight() );
            }

            p.drawText( r, align, album() );
            r.adjust( albumX, 0, 0, 0 );
        }
        if ( m_type & Track )
        {
            p.setBrush( palette().window() );
            p.setPen( palette().color( foregroundRole() ) );

            if ( m_type & Artist || m_type & Album )
            {
                p.drawText( r, align, DASH );
                r.adjust( dashX, 0, 0, 0 );
            }
            if ( m_hoverArea.width() && m_hoverArea.left() + contentsMargins().left() == r.left() )
            {
                p.setPen( palette().highlightedText().color() );
                p.setBrush( palette().highlight() );
            }

            p.drawText( r, align, track() );
            r.adjust( trackX, 0, 0, 0 );
        }
    }

    p.restore();
}


void
QueryLabel::changeEvent( QEvent* event )
{
    QFrame::changeEvent( event );
    switch ( event->type() )
    {
        case QEvent::FontChange:
        case QEvent::ApplicationFontChange:
            updateLabel();
            break;

        default:
            break;
    }
}


void
QueryLabel::mousePressEvent( QMouseEvent* event )
{
    QFrame::mousePressEvent( event );
    time.start();
}


void
QueryLabel::mouseReleaseEvent( QMouseEvent* event )
{
    QFrame::mouseReleaseEvent( event );

    m_dragPos = QPoint();
    if ( time.elapsed() < qApp->doubleClickInterval() )
        emit clicked();
}


void
QueryLabel::mouseMoveEvent( QMouseEvent* event )
{
    QFrame::mouseMoveEvent( event );
    int x = event->x();

    if ( event->buttons() & Qt::LeftButton &&
       ( m_dragPos - event->pos() ).manhattanLength() >= QApplication::startDragDistance() )
    {
        startDrag();
        leaveEvent( 0 );
        return;
    }

    if ( m_query.isNull() && m_result.isNull() )
    {
        m_hoverArea = QRect();
        return;
    }

    const QFontMetrics& fm = fontMetrics();
    int dashX = fm.width( DASH );
    int artistX = m_type & Artist ? fm.width( artist() ) : 0;
    int albumX = m_type & Album ? fm.width( album() ) : 0;
    int trackX = m_type & Track ? fm.width( track() ) : 0;

    if ( m_type & Track )
    {
        trackX += contentsMargins().left();
    }
    if ( m_type & Album )
    {
        trackX += albumX + dashX;
        albumX += contentsMargins().left();
    }
    if ( m_type & Artist )
    {
        albumX += artistX + dashX;
        trackX += artistX + dashX;
        artistX += contentsMargins().left();
    }

    QRect hoverArea;
    if ( m_type & Artist && x < artistX )
    {
        hoverArea.setLeft( 0 );
        hoverArea.setRight( artistX + contentsMargins().left() );
    }
    else if ( m_type & Album && x < albumX && x > artistX )
    {
        int spacing = ( m_type & Artist ) ? dashX : 0;
        hoverArea.setLeft( artistX + spacing );
        hoverArea.setRight( albumX + spacing + contentsMargins().left() );
    }
    else if ( m_type & Track && x < trackX && x > albumX )
    {
        int spacing = ( m_type & Album ) ? dashX : 0;
        hoverArea.setLeft( albumX + spacing );
        hoverArea.setRight( trackX + contentsMargins().left() );
    }

    if ( hoverArea.width() )
    {
        hoverArea.setY( 1 );
        hoverArea.setHeight( height() - 2 );
    }
    if ( hoverArea != m_hoverArea )
    {
        m_hoverArea = hoverArea;
        repaint();
    }
}


void
QueryLabel::leaveEvent( QEvent* event )
{
    m_hoverArea = QRect();
    repaint();
}


void
QueryLabel::startDrag()
{
    if ( m_query.isNull() )
        return;

    QByteArray queryData;
    QDataStream queryStream( &queryData, QIODevice::WriteOnly );
    QMimeData* mimeData = new QMimeData();
    mimeData->setText( text() );

    queryStream << qlonglong( &m_query );

    mimeData->setData( "application/tomahawk.query.list", queryData );
    QDrag *drag = new QDrag( this );
    drag->setMimeData( mimeData );
    drag->setPixmap( TomahawkUtils::createDragPixmap() );

//    QPoint hotSpot = event->pos() - child->pos();
//    drag->setHotSpot( hotSpot );

    drag->exec( Qt::CopyAction );
}


QString
QueryLabel::smartAppend( QString& text, const QString& appendage ) const
{
    QString s;
    if ( !text.isEmpty() )
        s = DASH;

    text += s + appendage;
    return text;
}

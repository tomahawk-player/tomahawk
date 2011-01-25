#include "welcomewidget.h"
#include "ui_welcomewidget.h"

#include "utils/tomahawkutils.h"

#include "playlist/playlistmanager.h"
#include "playlist/playlistmodel.h"

#include "widgets/overlaywidget.h"

#include "sourcelist.h"
#include "tomahawksettings.h"

#include <QPainter>

#define FILTER_TIMEOUT 280


WelcomeWidget::WelcomeWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::WelcomeWidget )
{
    ui->setupUi( this );

    ui->playlistWidget->setItemDelegate( new PlaylistDelegate() );
    ui->tracksView->overlay()->setEnabled( false );

    m_tracksModel = new PlaylistModel( ui->tracksView );
    ui->tracksView->setModel( m_tracksModel );
    m_tracksModel->loadHistory( Tomahawk::source_ptr() );

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );

    connect( ui->playlistWidget, SIGNAL( itemActivated( QListWidgetItem* ) ), SLOT( onPlaylistActivated( QListWidgetItem* ) ) );
}


WelcomeWidget::~WelcomeWidget()
{
    delete ui;
}


void
WelcomeWidget::updatePlaylists()
{
    ui->playlistWidget->clear();

    QList<Tomahawk::playlist_ptr> playlists = TomahawkSettings::instance()->recentlyPlayedPlaylists();

    foreach( const Tomahawk::playlist_ptr& playlist, playlists )
    {
        connect( playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), SLOT( updatePlaylists() ) );

        PlaylistWidgetItem* item = new PlaylistWidgetItem( playlist );
        ui->playlistWidget->addItem( item );
        item->setData( Qt::DisplayRole, playlist->title() );
    }

    if ( !playlists.count() )
    {
        ui->playlistWidget->overlay()->setText( tr( "You have not played any playlists yet." ) );
        ui->playlistWidget->overlay()->show();
    }
    else
        ui->playlistWidget->overlay()->hide();
}


void
WelcomeWidget::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source->collection().data(), SIGNAL( playlistsAdded( QList<Tomahawk::playlist_ptr> ) ), SLOT( updatePlaylists() ) );
    connect( source->collection().data(), SIGNAL( playlistsDeleted( QList<Tomahawk::playlist_ptr> ) ), SLOT( updatePlaylists() ) );

    connect( source.data(), SIGNAL( playbackFinished( Tomahawk::query_ptr ) ), SLOT( onPlaybackFinished( Tomahawk::query_ptr ) ) );
}


void
WelcomeWidget::onPlaybackFinished( const Tomahawk::query_ptr& query )
{
    m_tracksModel->insert( 0, query );
}


void
WelcomeWidget::onPlaylistActivated( QListWidgetItem* item )
{
    qDebug() << Q_FUNC_INFO;

    PlaylistWidgetItem* pwi = dynamic_cast<PlaylistWidgetItem*>(item);
    PlaylistManager::instance()->show( pwi->playlist() );
}


void
WelcomeWidget::changeEvent( QEvent* e )
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


QVariant
PlaylistWidgetItem::data( int role ) const
{
    if ( role == ArtistRole )
    {
        if ( m_artists.isEmpty() )
        {
            QStringList artists;

            foreach( const Tomahawk::plentry_ptr& entry, m_playlist->entries() )
            {
                if ( !artists.contains( entry->query()->artist() ) )
                    artists << entry->query()->artist();
            }

            m_artists = artists.join( ", " );
        }

        return m_artists;
    }

    if ( role == TrackCountRole )
    {
        return m_playlist->entries().count();
    }

    if ( role == Qt::DisplayRole )
    {
        return m_playlist->title();
    }

    return QListWidgetItem::data( role );
}


QSize
PlaylistDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    return QSize( 0, 64 );
}


void
PlaylistDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, QModelIndex() );
    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( option.state & QStyle::State_Selected )
    {
        opt.palette.setColor( QPalette::Text, opt.palette.color( QPalette::HighlightedText ) );
    }

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    painter->setPen( opt.palette.color( QPalette::Text ) );

    QTextOption to;
    to.setAlignment( Qt::AlignCenter );
    QFont font = opt.font;
    QFont boldFont = opt.font;
    boldFont.setBold( true );

    painter->drawPixmap( option.rect.adjusted( 10, 12, -option.rect.width() + 42, -12 ), m_playlistIcon );

    painter->drawText( option.rect.adjusted( 56, 26, -100, -8 ), index.data( PlaylistWidgetItem::ArtistRole ).toString() );

    QString trackCount = tr( "%1 tracks" ).arg( index.data( PlaylistWidgetItem::TrackCountRole ).toString() );
    painter->drawText( option.rect.adjusted( option.rect.width() - 96, 2, 0, -2 ), trackCount, to );

    painter->setFont( boldFont );
    painter->drawText( option.rect.adjusted( 56, 6, -100, -option.rect.height() + 20 ), index.data().toString() );

    painter->restore();
}


PlaylistWidget::PlaylistWidget( QWidget* parent )
    : QListWidget( parent )
{
    m_overlay = new OverlayWidget( this );
}

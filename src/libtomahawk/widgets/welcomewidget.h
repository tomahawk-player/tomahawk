#ifndef WELCOMEWIDGET_H
#define WELCOMEWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QStyledItemDelegate>

#include "playlistinterface.h"

#include "playlist.h"
#include "result.h"
#include "viewpage.h"

#include "utils/tomahawkutils.h"

#include "dllmacro.h"

class PlaylistModel;
class OverlayWidget;

namespace Ui
{
    class WelcomeWidget;
}

class DLLEXPORT PlaylistDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    PlaylistDelegate()
    {
        m_playlistIcon = QPixmap( RESPATH "images/playlist-icon.png" );
    }

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

private:
    QPixmap m_playlistIcon;
};


class DLLEXPORT PlaylistWidgetItem : public QListWidgetItem
{
public:
    enum ItemRoles
    { ArtistRole = Qt::UserRole, TrackCountRole };

    PlaylistWidgetItem( const Tomahawk::playlist_ptr& playlist ) : QListWidgetItem() { m_playlist = playlist; }
    ~PlaylistWidgetItem() {}

    virtual QVariant data( int role ) const;

    Tomahawk::playlist_ptr playlist() const { return m_playlist; }

private:
    Tomahawk::playlist_ptr m_playlist;

    mutable QString m_artists;
};


class DLLEXPORT PlaylistWidget : public QListWidget
{
public:
    PlaylistWidget( QWidget* parent = 0 );

    OverlayWidget* overlay() const { return m_overlay; }

private:
    OverlayWidget* m_overlay;
};


class DLLEXPORT WelcomeWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    WelcomeWidget( QWidget* parent = 0 );
    ~WelcomeWidget();

    virtual QWidget* widget() { return this; }
    virtual PlaylistInterface* playlistInterface() const { return 0; }

    virtual QString title() const { return tr( "Welcome to Tomahawk" ); }
    virtual QString description() const { return QString(); }

    virtual bool showStatsBar() const { return false; }

    virtual bool jumpToCurrentTrack() { return false; }

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

public slots:
    void updatePlaylists();

private slots:
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onPlaylistActivated( QListWidgetItem* item );
    void onPlaybackFinished( const Tomahawk::query_ptr& query );

private:
    Ui::WelcomeWidget *ui;

    PlaylistModel* m_tracksModel;
};

#endif // WELCOMEWIDGET_H

#ifndef WELCOMEWIDGET_H
#define WELCOMEWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QStyledItemDelegate>

#include "tomahawk/tomahawkapp.h"
#include "playlistinterface.h"

#include "playlist.h"
#include "result.h"

class PlaylistModel;

namespace Ui
{
    class WelcomeWidget;
}

class PlaylistDelegate : public QStyledItemDelegate
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


class PlaylistWidgetItem : public QListWidgetItem
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


class WelcomeWidget : public QWidget
{
Q_OBJECT

public:
    WelcomeWidget( QWidget* parent = 0 );
    ~WelcomeWidget();

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

public slots:
    void updatePlaylists();

private slots:
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onPlaylistActivated( QListWidgetItem* item );

private:
    Ui::WelcomeWidget *ui;

    PlaylistModel* m_tracksModel;
};

#endif // WELCOMEWIDGET_H

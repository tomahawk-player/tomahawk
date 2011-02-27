#ifndef NEWPLAYLISTWIDGET_H
#define NEWPLAYLISTWIDGET_H

#include <QWidget>
#include <QTimer>

#include "album.h"
#include "result.h"
#include "playlistinterface.h"
#include "viewpage.h"

#include "dllmacro.h"

class QPushButton;
class PlaylistModel;

namespace Ui
{
    class NewPlaylistWidget;
}

class DLLEXPORT NewPlaylistWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    NewPlaylistWidget( QWidget* parent = 0 );
    ~NewPlaylistWidget();

    virtual QWidget* widget() { return this; }
    virtual PlaylistInterface* playlistInterface() const { return 0; }
    
    virtual QString title() const { return tr( "Create a new playlist" ); }
    virtual QString description() const { return QString(); }

    virtual bool showStatsBar() const { return false; }

    virtual bool jumpToCurrentTrack() { return false; }

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

private slots:
    void onTitleChanged( const QString& title );
    void onTagChanged();

    void updateSuggestions();
    void suggestionsFound();

    void savePlaylist();
    void cancel();

private:
    Ui::NewPlaylistWidget *ui;

    PlaylistModel* m_suggestionsModel;
    QList< Tomahawk::plentry_ptr > m_entries;

    QTimer m_filterTimer;
    QString m_tag;
    QPushButton* m_saveButton;
};

#endif // NEWPLAYLISTWIDGET_H

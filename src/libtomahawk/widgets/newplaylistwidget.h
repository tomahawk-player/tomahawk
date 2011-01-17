#ifndef NEWPLAYLISTWIDGET_H
#define NEWPLAYLISTWIDGET_H

#include <QWidget>
#include <QTimer>

#include "album.h"
#include "result.h"
#include "playlistinterface.h"

#include "dllmacro.h"

class PlaylistModel;

namespace Ui
{
    class NewPlaylistWidget;
}

class DLLEXPORT NewPlaylistWidget : public QWidget
{
Q_OBJECT

public:
    NewPlaylistWidget( QWidget* parent = 0 );
    ~NewPlaylistWidget();

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

private slots:
    void tagChanged();
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
};

#endif // NEWPLAYLISTWIDGET_H

#ifndef SOURCEINFOWIDGET_H
#define SOURCEINFOWIDGET_H

#include <QWidget>

#include "album.h"
#include "result.h"
#include "playlistinterface.h"

#include "dllmacro.h"

class AlbumModel;
class CollectionFlatModel;
class PlaylistModel;

namespace Ui
{
    class SourceInfoWidget;
}

class DLLEXPORT SourceInfoWidget : public QWidget
{
Q_OBJECT

public:
    SourceInfoWidget( const Tomahawk::source_ptr& source, QWidget* parent = 0 );
    ~SourceInfoWidget();

protected:
    void changeEvent( QEvent* e );

private slots:
    void onPlaybackFinished( const Tomahawk::query_ptr& query );

private:
    Ui::SourceInfoWidget *ui;

    CollectionFlatModel* m_recentCollectionModel;
    PlaylistModel* m_historyModel;
    AlbumModel* m_recentAlbumModel;
};

#endif // SOURCEINFOWIDGET_H

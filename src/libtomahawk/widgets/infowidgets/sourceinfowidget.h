#ifndef SOURCEINFOWIDGET_H
#define SOURCEINFOWIDGET_H

#include <QWidget>

#include "album.h"
#include "result.h"
#include "playlistinterface.h"
#include "viewpage.h"

#include "dllmacro.h"

class AlbumModel;
class CollectionFlatModel;
class PlaylistModel;

namespace Ui
{
    class SourceInfoWidget;
}

class DLLEXPORT SourceInfoWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    SourceInfoWidget( const Tomahawk::source_ptr& source, QWidget* parent = 0 );
    ~SourceInfoWidget();

    virtual QWidget* widget() { return this; }
    virtual PlaylistInterface* playlistInterface() const { return 0; }
    
    virtual QString title() const { return m_title; }
    virtual QString description() const { return m_description; }

    virtual bool showStatsBar() const { return false; }

    virtual bool jumpToCurrentTrack() { return false; }
    
protected:
    void changeEvent( QEvent* e );

private slots:
    void onPlaybackFinished( const Tomahawk::query_ptr& query );

private:
    Ui::SourceInfoWidget *ui;

    CollectionFlatModel* m_recentCollectionModel;
    PlaylistModel* m_historyModel;
    AlbumModel* m_recentAlbumModel;

    QString m_title;
    QString m_description;
};

#endif // SOURCEINFOWIDGET_H

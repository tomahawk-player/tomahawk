#ifndef TOPBAR_H
#define TOPBAR_H

#include <QWidget>
#include <QLabel>
#include <QList>

#include "playlist/playlistmanager.h"
#include "sourcelist.h"
#include "dllmacro.h"

namespace Ui
{
    class TopBar;
}

class DLLEXPORT TopBar : public QWidget
{
Q_OBJECT

public:
    TopBar( QWidget* parent = 0 );
    ~TopBar();

signals:
    void filterTextChanged( const QString& newtext );

    void flatMode();
    void artistMode();
    void albumMode();

public slots:
    void setNumSources( unsigned int );
    void setNumTracks( unsigned int );
    void setNumArtists( unsigned int );
    void setNumShown( unsigned int );

    void setStatsVisible( bool b );
    void setModesVisible( bool b );

    void addSource();
    void removeSource();

    void setFilter( const QString& filter );

private slots:
    void onModeChanged( PlaylistInterface::ViewMode mode );
    void onFlatMode();
    void onArtistMode();
    void onAlbumMode();
    
protected:
    void changeEvent( QEvent* e );
    void resizeEvent( QResizeEvent* e );

private:
    void fadeOutDude( unsigned int i );
    void fadeInDude( unsigned int i );

    Ui::TopBar* ui;

    unsigned int m_sources, m_tracks, m_artists, m_shown;
    QList<QLabel*> m_dudes;

    Tomahawk::source_ptr m_onesource;
};

#endif // TOPBAR_H

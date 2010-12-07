#ifndef TOPBAR_H
#define TOPBAR_H

#include <QWidget>
#include <QList>
#include <QLabel>

#include "tomahawk/sourcelist.h"

namespace Ui
{
    class TopBar;
}

class TopBar : public QWidget
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

protected:
    void changeEvent( QEvent* e );

private:
    void fadeOutDude( unsigned int i );
    void fadeInDude( unsigned int i );

    Ui::TopBar* ui;

    unsigned int m_sources, m_tracks, m_artists, m_shown;
    QList<QLabel*> m_dudes;

    Tomahawk::source_ptr m_onesource;
};

#endif // TOPBAR_H

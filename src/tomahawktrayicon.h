#ifndef TOMAHAWK_TRAYICON_H
#define TOMAHAWK_TRAYICON_H

#include <QSystemTrayIcon>
#include <QTimer>
#include <QMenu>

#include "tomahawk/result.h"

class TomahawkTrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    TomahawkTrayIcon( QObject* parent );
    virtual bool event( QEvent* e );

public slots:
    void setResult( const Tomahawk::result_ptr& result );

private slots:
    void onAnimationTimer();
    void onActivated( QSystemTrayIcon::ActivationReason reason );

private:
    void refreshToolTip();
    ~TomahawkTrayIcon();

    QTimer m_animationTimer;
    Tomahawk::result_ptr m_currentTrack;

    QList<QPixmap> m_animationPixmaps;
    int m_currentAnimationFrame;
    
    QMenu* m_contextMenu;
    QAction* m_playAction;
    QAction* m_pauseAction;
    QAction* m_stopAction;
    QAction* m_prevAction;
    QAction* m_nextAction;
    QAction* m_quitAction;
};

#endif // TOMAHAWK_TRAYICON_H


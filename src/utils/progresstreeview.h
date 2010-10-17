#ifndef PROGRESSTREEVIEW_H
#define PROGRESSTREEVIEW_H

#include <QTreeView>
#include <QProgressBar>

class ProgressTreeView : public QTreeView
{
Q_OBJECT

public:
    ProgressTreeView( QWidget* parent );

    void connectProgressBar( QProgressBar* progressBar ) { m_progressBar = progressBar; progressBar->setVisible( false ); }

    void setProgressStarted( int length ) { if ( m_progressBar ) { m_progressBar->setRange( 0, length ); m_progressBar->setValue( 0 ); m_progressBar->setVisible( true ); } }
    void setProgressEnded() { if ( m_progressBar ) m_progressBar->setVisible( false );  }
    void setProgressCompletion( int completion ) { if ( m_progressBar ) m_progressBar->setValue( completion ); }

private:
    QProgressBar* m_progressBar;
};

#endif // PROGRESSTREEVIEW_H

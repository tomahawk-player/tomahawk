/*
    Fetches and parses an XSPF document from a QFile or QUrl.
 */

#ifndef XSPFLOADER_H
#define XSPFLOADER_H

#include <QDebug>
#include <QObject>
#include <QUrl>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "playlist.h"
#include "typedefs.h"

class XSPFLoader : public QObject
{
Q_OBJECT

public:
    explicit XSPFLoader( QObject* parent = 0 )
        : QObject( parent )
    {}

    virtual ~XSPFLoader()
    {
        qDebug() << Q_FUNC_INFO;
    }

signals:
    void failed();
    void ok( const Tomahawk::playlist_ptr& );

public slots:
    void load( const QUrl& url );
    void load( QFile& file );

private slots:
    void networkLoadFinished();
    void networkError( QNetworkReply::NetworkError e );

private:
    void reportError();
    void gotBody();

    QByteArray m_body;
    Tomahawk::playlist_ptr m_playlist;
};

#endif // XSPFLOADER_H

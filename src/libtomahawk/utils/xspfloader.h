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

#include "dllmacro.h"

class DLLEXPORT XSPFLoader : public QObject
{
Q_OBJECT

public:
    explicit XSPFLoader( bool autoCreate = true, QObject* parent = 0 )
        : QObject( parent )
        , m_autoCreate( autoCreate )
        , NS("http://xspf.org/ns/0/")
    {}

    virtual ~XSPFLoader()
    {
        qDebug() << Q_FUNC_INFO;
    }

    QList< Tomahawk::plentry_ptr > entries() const { return m_entries; }

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
    QString NS;
    void reportError();
    void gotBody();

    bool m_autoCreate;
    QList< Tomahawk::plentry_ptr > m_entries;
    QString m_title, m_info, m_creator;

    QByteArray m_body;
    Tomahawk::playlist_ptr m_playlist;
};

#endif // XSPFLOADER_H

#ifndef TOMAHAWKUTILS_H
#define TOMAHAWKUTILS_H

#include "dllmacro.h"
#include <QObject>

#define RESPATH ":/data/"

class QDir;
class QDateTime;
class QString;
class QPixmap;
class QNetworkAccessManager;
class QNetworkProxy;

class JDnsShared;
class JDnsSharedRequest;

namespace TomahawkUtils
{
    class DLLEXPORT DNSResolver : public QObject
    {
        Q_OBJECT
    public:
        explicit DNSResolver();
        ~DNSResolver() {}
        
        void resolve( QString &host, QString &type );
        
    signals:
        void result( QString &result );
        
    public slots:
        void resultsReady();
        
    private:
        JDnsShared* m_dnsShared;
        JDnsSharedRequest* m_dnsSharedRequest;
    };   
    
    DLLEXPORT QDir appConfigDir();
    DLLEXPORT QDir appDataDir();

    DLLEXPORT QString timeToString( int seconds );
    DLLEXPORT QString ageToString( const QDateTime& time );
    DLLEXPORT QString filesizeToString( unsigned int size );

    DLLEXPORT QPixmap createDragPixmap( int itemCount = 1 );

    DLLEXPORT QNetworkAccessManager* nam();
    DLLEXPORT QNetworkProxy* proxy();

    DLLEXPORT void setNam( QNetworkAccessManager* nam );
    DLLEXPORT void setProxy( QNetworkProxy* proxy );

    DLLEXPORT DNSResolver* dnsResolver();
}

#endif // TOMAHAWKUTILS_H

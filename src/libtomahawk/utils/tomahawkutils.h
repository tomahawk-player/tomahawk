#ifndef TOMAHAWKUTILS_H
#define TOMAHAWKUTILS_H

#include "dllmacro.h"

#define RESPATH ":/data/"

class QDir;
class QDateTime;
class QString;
class QPixmap;
class QNetworkAccessManager;
class QNetworkProxy;

namespace TomahawkUtils
{
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
}

#endif // TOMAHAWKUTILS_H

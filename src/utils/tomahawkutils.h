#ifndef TOMAHAWKUTILS_H
#define TOMAHAWKUTILS_H

class QDir;
class QDateTime;
class QString;
class QPixmap;

namespace TomahawkUtils
{
    QDir appConfigDir();
    QDir appDataDir();

    QString timeToString( int seconds );
    QString ageToString( const QDateTime& time );
    QString filesizeToString( unsigned int size );

    QPixmap createDragPixmap( int itemCount = 1 );
}

#endif // TOMAHAWKUTILS_H

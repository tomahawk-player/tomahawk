#ifndef TOMAHAWKUTILS_H
#define TOMAHAWKUTILS_H

class QDir;
class QDateTime;
class QString;

namespace TomahawkUtils
{
    QDir appConfigDir();
    QDir appDataDir();

    QString timeToString( int seconds );
    QString ageToString( const QDateTime& time );
}

#endif // TOMAHAWKUTILS_H

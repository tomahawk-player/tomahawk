#ifndef TOMAHAWKUTILS_H
#define TOMAHAWKUTILS_H

#include <QDir>

namespace TomahawkUtils
{
    QDir appConfigDir();
    QDir appDataDir();

    QString timeToString( int seconds );
}

#endif // TOMAHAWKUTILS_H

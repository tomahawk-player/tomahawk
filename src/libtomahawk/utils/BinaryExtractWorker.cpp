/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BinaryExtractWorker.h"

#include <QVariant>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"


namespace TomahawkUtils
{

void
BinaryExtractWorker::run()
{
    ScopedDeleter deleter( this );
    // We unzip directly to the target location, just like normal attica resolvers
    Q_ASSERT( m_receiver );
    if ( !m_receiver )
        return;

    const QString resolverId = m_receiver->property( "resolverid" ).toString();

    Q_ASSERT( !resolverId.isEmpty() );
    if ( resolverId.isEmpty() )
        return;

    const QDir resolverPath( extractScriptPayload( m_zipFileName, resolverId ) );

#ifdef Q_OS_WIN
    const QStringList files = resolverPath.entryList( QStringList() << "*.exe", QDir::Files );
#else
    const QStringList files = resolverPath.entryList( QStringList() << "*_tomahawkresolver", QDir::Files );
#endif

    qDebug() << "Found executables in unzipped binary resolver dir:" << files;
    Q_ASSERT( files.size() == 1 );
    if ( files.size() < 1 )
        return;

    const QString resolverToUse = resolverPath.absoluteFilePath( files.first() );

    QFile file( resolverToUse );
    file.setPermissions( file.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther );

    QMetaObject::invokeMethod( m_receiver, "installSucceeded", Qt::QueuedConnection, Q_ARG( QString, resolverToUse ) );
}

}

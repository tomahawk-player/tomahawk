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

#ifdef Q_OS_MAC
        // Platform-specific handling of resolver payload now. We know it's good
        // Unzip the file.
        QFileInfo info( m_zipFileName );
        QDir tmpDir = QDir::tempPath();
        if  ( !tmpDir.mkdir( info.baseName() ) )
        {
            qWarning() << "Failed to create temporary directory to unzip in:" << tmpDir.absolutePath();
            return;
        }
        tmpDir.cd( info.baseName() );
        TomahawkUtils::unzipFileInFolder( info.absoluteFilePath(), tmpDir );

        // On OSX it just contains 1 file, the resolver executable itself. For now. We just copy it to
        // the Tomahawk.app/Contents/MacOS/ folder alongside the Tomahawk executable.
        const QString dest = QCoreApplication::applicationDirPath();
        // Find the filename
        const QDir toList( tmpDir.absolutePath() );
        const QStringList files = toList.entryList( QStringList(), QDir::Files );
        Q_ASSERT( files.size() == 1 );

        const QString src = toList.absoluteFilePath( files.first() );
        qDebug() << "OS X: Copying binary resolver from to:" << src << dest;

        copyWithAuthentication( src, dest, m_receiver );

        return;
#elif  defined(Q_OS_WIN) || defined(Q_OS_LINUX)
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
#elif defined(Q_OS_LINUX)
        const QStringList files = resolverPath.entryList( QStringList() << "*_tomahawkresolver", QDir::Files );
#endif

        qDebug() << "Found executables in unzipped binary resolver dir:" << files;
        Q_ASSERT( files.size() == 1 );
        if ( files.size() < 1 )
            return;

        const QString resolverToUse = resolverPath.absoluteFilePath( files.first() );

#ifdef Q_OS_LINUX
        QProcess p;
        p.start( "chmod", QStringList() << "744" << resolverToUse, QIODevice::ReadOnly );
        p.waitForFinished();
#endif

        QMetaObject::invokeMethod( m_receiver, "installSucceeded", Qt::QueuedConnection, Q_ARG( QString, resolverToUse ) );

#endif
}

}

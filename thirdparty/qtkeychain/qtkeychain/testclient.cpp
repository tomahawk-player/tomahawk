/******************************************************************************
 *   Copyright (C) 2011 Frank Osterfeld <frank.osterfeld@gmail.com>           *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#include <QCoreApplication>
#include <QStringList>

#include "keychain.h"
#include <iostream>

using namespace QKeychain;

static int printUsage() {
    std::cerr << "testclient store <account> <password>" << std::endl;
    std::cerr << "testclient restore <account>" << std::endl;
    std::cerr << "testclient delete <account>" << std::endl;
    return 1;
}

int main( int argc, char** argv ) {
    QCoreApplication app( argc, argv );
    const QStringList args = app.arguments();
    if ( args.count() < 2 )
        return printUsage();

    QStringList::ConstIterator it = args.constBegin();
    ++it;

    if ( *it == QLatin1String("store") ) {
        if ( ++it == args.constEnd() )
            return printUsage();
        const QString acc = *it;
        if ( ++it == args.constEnd() )
            return printUsage();
        const QString pass = *it;
        if ( ++it != args.constEnd() )
            return printUsage();
        WritePasswordJob job( QLatin1String("qtkeychain-testclient") );
        job.setAutoDelete( false );
        job.setKey( acc );
        job.setTextData( pass );
        QEventLoop loop;
        job.connect( &job, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()) );
        job.start();
        loop.exec();
     if ( job.error() ) {
            std::cerr << "Storing password failed: " << qPrintable(job.errorString()) << std::endl;
            return 1;
        }
        std::cout << "Password stored successfully" << std::endl;
    } else if ( *it == QLatin1String("restore") ) {
        if ( ++it == args.constEnd() )
            return printUsage();
        const QString acc = *it;
        if ( ++it != args.constEnd() )
            return printUsage();
        ReadPasswordJob job( QLatin1String("qtkeychain-testclient") );
        job.setAutoDelete( false );
        job.setKey( acc );
        QEventLoop loop;
        job.connect( &job, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()) );
        job.start();
        loop.exec();

        const QString pw = job.textData();
        if ( job.error() ) {
            std::cerr << "Restoring password failed: " << qPrintable(job.errorString()) << std::endl;
            return 1;
        }
        std::cout << qPrintable(pw) << std::endl;
    } else if ( *it == QLatin1String("delete") ) {
        if ( ++it == args.constEnd() )
            return printUsage();
        const QString acc = *it;
        if ( ++it != args.constEnd() )
            return printUsage();
        DeletePasswordJob job( QLatin1String("qtkeychain-testclient") );
        job.setAutoDelete( false );
        job.setKey( acc );
        QEventLoop loop;
        job.connect( &job, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()) );
        job.start();
        loop.exec();

        if ( job.error() ) {
            std::cerr << "Deleting password failed: " << qPrintable(job.errorString()) << std::endl;
            return 1;
        }
        std::cout << "Password deleted successfully" << std::endl;
    } else {
        return printUsage();
    }
}


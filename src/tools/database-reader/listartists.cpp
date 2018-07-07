#include "database/Database.h"
#include "database/DatabaseCommand_AllArtists.h"
#include "utils/TomahawkUtils.h"
#include "TomahawkVersion.h"
#include "Typedefs.h"

#include <QCoreApplication>
#include <QDir>

#include <iostream>


class Tasks: public QObject
{
Q_OBJECT
public:
    Q_INVOKABLE void startDatabase( QString dbpath )
    {
        database = QSharedPointer<Tomahawk::Database>( new Tomahawk::Database( dbpath ) );
        connect( database.data(), SIGNAL( ready() ), SLOT( runDbCmd() ), Qt::QueuedConnection );
        database->loadIndex();
    }

    Tomahawk::dbcmd_ptr cmd;
    QSharedPointer<Tomahawk::Database> database;

public slots:
    void runDbCmd()
    {
        database->enqueue( cmd );
    }

    void dbCmdDone( const QList<Tomahawk::artist_ptr>& artists )
    {
        std::cout << "=== ARTISTS - START ===" << std::endl;
        foreach ( const Tomahawk::artist_ptr& artist, artists )
        {
            std::cout << artist->name().toStdString() << std::endl;
        }
        std::cout << "=== ARTISTS - END ===" << std::endl;
        QMetaObject::invokeMethod( thread(), "quit", Qt::QueuedConnection );
    }
};

// Include needs to go here as Tasks needs to be defined before.
#include "listartists.moc"

int main( int argc, char* argv[] )
{
    QCoreApplication app( argc, argv );
    // TODO: Add an argument to change the path
    app.setOrganizationName( TOMAHAWK_ORGANIZATION_NAME );

    // Helper QObject to connect slots and actions in the correct thread.
    Tasks tasks;

    // Start a thread so we can actually block main until the end of the DbCmd
    QThread thread( nullptr );
    thread.start();

    // We need to do this or the finished() signal/quit() SLOT is not called.
    thread.moveToThread( &thread );
    tasks.moveToThread( &thread );

    // Load the Database
    Tomahawk::DatabaseCommand_AllArtists* cmd = new Tomahawk::DatabaseCommand_AllArtists();
    tasks.cmd = Tomahawk::dbcmd_ptr( cmd );
    tasks.cmd->moveToThread( &thread );
    qRegisterMetaType< QList< Tomahawk::artist_ptr > >();
    QObject::connect( cmd, SIGNAL( artists( QList<Tomahawk::artist_ptr>  ) ),
                      &tasks, SLOT( dbCmdDone( QList<Tomahawk::artist_ptr> ) ),
                      Qt::QueuedConnection );
    QString dbpath = TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.db" );
    QMetaObject::invokeMethod( &tasks, "startDatabase", Qt::QueuedConnection, Q_ARG( QString, dbpath ) );

    // Wait until the dbcmd was executed.
    thread.wait();
}

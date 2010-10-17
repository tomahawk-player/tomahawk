#include "databasecommand_updatesearchindex.h"
DatabaseCommand_UpdateSearchIndex::DatabaseCommand_UpdateSearchIndex( const QString& t, int p )
    : DatabaseCommand()
    , table( t )
    , pkey( p )
{
    if( table != "artist" && table != "track" && table != "album" )
    {
        Q_ASSERT(false);
        return;
    }
}

void DatabaseCommand_UpdateSearchIndex::exec(DatabaseImpl *db)
{
    qDebug() << Q_FUNC_INFO;

    if( table != "artist" && table != "track" && table != "album" )
    {
        Q_ASSERT(false);
        return;
    }

    // if pkey is 0, consult DB to see what needs indexing
    if( pkey == 0 )
    {
        TomahawkSqlQuery q = db->newquery();
        q.exec( QString("SELECT coalesce(max(id),0) from %1_search_index").arg(table) );
        q.next();
        pkey = 1 + q.value(0).toInt();
        qDebug() << "updateSearchIndex" << table << "consulted DB, starting at" << pkey;
    }

    TomahawkSqlQuery query = db->newquery();
    qDebug() << "Building index for" << table << ">= id" << pkey;
    QString searchtable( table + "_search_index" );
    query.exec(QString( "SELECT id, sortname FROM %1 WHERE id >= %2" ).arg( table ).arg(pkey ) );

    TomahawkSqlQuery upq = db->newquery();
    TomahawkSqlQuery inq = db->newquery();
    inq.prepare( "INSERT INTO "+ searchtable +" (ngram, id, num) VALUES (?,?,?)" );
    upq.prepare( "UPDATE "+ searchtable +" SET num=num+? WHERE ngram=? AND id=?" );

    int num_names = 0;
    int num_ngrams = 0;
    int id;
    QString name;
    QMap<QString, int> ngrammap;

    // this is the new ngram map we build up, to be merged into the
    // main one in FuzzyIndex:
    QHash< QString, QMap<quint32, quint16> > idx;

    while( query.next() )
    {
        id = query.value( 0 ).toInt();
        name = query.value( 1 ).toString();
        num_names++;
        inq.bindValue( 1, id ); // set id
        upq.bindValue( 2, id ); // set id
        ngrammap = DatabaseImpl::ngrams( name );
        QMapIterator<QString, int> i( ngrammap );

        while ( i.hasNext() )
        {
            i.next();
            num_ngrams++;
            upq.bindValue( 0, i.value() ); //num
            upq.bindValue( 1, i.key() ); // ngram
            upq.exec();

            if( upq.numRowsAffected() == 0 )
            {
                inq.bindValue( 0, i.key() ); //ngram
                inq.bindValue( 2, i.value() ); //num
                inq.exec();
                if( inq.numRowsAffected() == 0 )
                {
                    qDebug() << "Error updating search index:" << id << name;
                    continue;
                }
            }

            // update ngram cache:
            QMapIterator<QString, int> iter( ngrammap );
            while ( iter.hasNext() )
            {
                iter.next();
                if( idx.contains( iter.key() ) )
                {
                    idx[ iter.key() ][ id ] += iter.value();
                }
                else
                {
                    QMap<quint32, quint16> tmp;
                    tmp.insert( id, iter.value() );
                    idx.insert( iter.key(), tmp );
                }
            }


        }
    }

    // merge in our ngrams into the main index
    QMetaObject::invokeMethod( &(db->m_fuzzyIndex),
                               "mergeIndex",
                               Qt::QueuedConnection,
                               Q_ARG( QString, table ),
                               QGenericArgument( "QHash< QString, QMap<quint32, quint16> >", &idx )
                              );

    qDebug() << "Finished indexing" << num_names <<" names," << num_ngrams << "ngrams.";
}

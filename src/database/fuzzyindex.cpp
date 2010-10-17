#include "fuzzyindex.h"
#include "databaseimpl.h"
#include <QTime>

FuzzyIndex::FuzzyIndex(DatabaseImpl &db) :
    QObject(), m_db( db ), m_loaded( false )
{
}

void
FuzzyIndex::loadNgramIndex()
{
    // this updates the index in the DB, if needed:
    qDebug() << "Checking catalogue is fully indexed..";
    m_db.updateSearchIndex("artist",0);
    m_db.updateSearchIndex("album",0);
    m_db.updateSearchIndex("track",0);

    // loads index from DB into memory:
    qDebug() << "Loading search index for catalogue metadata..." << thread();
    loadNgramIndex_helper( m_artist_ngrams, "artist" );
    loadNgramIndex_helper( m_album_ngrams, "album" );
    loadNgramIndex_helper( m_track_ngrams, "track" );
    m_loaded = true;
    emit indexReady();
}


void
FuzzyIndex::loadNgramIndex_helper( QHash< QString, QMap<quint32, quint16> >& idx, const QString& table, unsigned int fromkey )
{
    QTime t;
    t.start();
    TomahawkSqlQuery query = m_db.newquery();
    query.exec( QString( "SELECT ngram, id, num "
                         "FROM %1_search_index "
                         "WHERE id >= %2 "
                         "ORDER BY ngram" ).arg( table ).arg( fromkey ) );

    QMap<quint32, quint16> ngram_idx;
    QString lastngram;
    while( query.next() )
    {
        if( lastngram.isEmpty() )
            lastngram = query.value(0).toString();

        if( query.value( 0 ).toString() != lastngram )
        {
            idx.insert( lastngram, ngram_idx );
            lastngram = query.value( 0 ).toString();
            ngram_idx.clear();
        }

        ngram_idx.insert( query.value( 1 ).toUInt(),
                          query.value( 2 ).toUInt() );
    }

    idx.insert( lastngram, ngram_idx );
    qDebug() << "Loaded" << idx.size()
             << "ngram entries for" << table
             << "in" << t.elapsed();
}

void FuzzyIndex::mergeIndex(const QString& table, QHash< QString, QMap<quint32, quint16> > tomerge)
{
    qDebug() << Q_FUNC_INFO << table << tomerge.keys().size();

    QHash< QString, QMap<quint32, quint16> >* idx;
    if     ( table == "artist" ) idx = &m_artist_ngrams;
    else if( table == "album"  ) idx = &m_album_ngrams;
    else if( table == "track"  ) idx = &m_track_ngrams;
    else Q_ASSERT(false);

    if( tomerge.size() == 0 ) return;

    if( idx->size() == 0 )
    {
        *idx = tomerge;
    }
    else
    {
        foreach( const QString& ngram, tomerge.keys() )
        {

            if( idx->contains( ngram ) )
            {
                foreach( quint32 id, tomerge[ngram].keys() )
                {
                    (*idx)[ ngram ][ id ] += tomerge[ngram][id];
                }
            }
            else
            {
                idx->insert( ngram, tomerge[ngram] );
            }
        }
    }
    qDebug() << Q_FUNC_INFO << table << "merge complete, num items:" << tomerge.size();
}

QMap< int, float > FuzzyIndex::search( const QString& table, const QString& name )
{
    QMap< int, float > resultsmap;

    QHash< QString, QMap<quint32, quint16> >* idx;
    if( table == "artist" ) idx = &m_artist_ngrams;
    else if( table == "album" ) idx = &m_album_ngrams;
    else if( table == "track" ) idx = &m_track_ngrams;

    QMap<QString,int> ngramsmap = DatabaseImpl::ngrams( name );
    foreach( const QString& ngram, ngramsmap.keys() )
    {
        if( !idx->contains( ngram ) )
            continue;
        //qDebug() << name_orig << "NGRAM:" << ngram << "candidates:" << (*idx)[ngram].size();
        QMapIterator<quint32, quint16> iter( (*idx)[ngram] );
        while( iter.hasNext() )
        {
            iter.next();
            resultsmap[ (int) iter.key() ] += (float) iter.value();
        }
    }
    return resultsmap;
}

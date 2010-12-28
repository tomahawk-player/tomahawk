#ifndef FUZZYINDEX_H
#define FUZZYINDEX_H

#include <QObject>
#include <QMap>
#include <QHash>
#include <QString>

class DatabaseImpl;


class FuzzyIndex : public QObject
{
Q_OBJECT
public:
    explicit FuzzyIndex( DatabaseImpl &db );

signals:
    void indexReady();

public slots:
    void loadNgramIndex();
    QMap< int, float > search( const QString& table, const QString& name );
    void mergeIndex( const QString& table, QHash< QString, QMap<quint32, quint16> > tomerge );

private:
    void loadNgramIndex_helper( QHash< QString, QMap<quint32, quint16> >& idx, const QString& table, unsigned int fromkey = 0 );

    // maps an ngram to {track id, num occurences}
    QHash< QString, QMap<quint32, quint16> > m_artist_ngrams, m_album_ngrams, m_track_ngrams;

    DatabaseImpl & m_db;
    bool m_loaded;
};

#endif // FUZZYINDEX_H

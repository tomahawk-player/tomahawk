#ifndef TOMAHAWKARTIST_H
#define TOMAHAWKARTIST_H

#include <QObject>
#include <QSharedPointer>

namespace Tomahawk
{

class Artist : public QObject
{
Q_OBJECT

public:
    Artist( const QString& name )
        : m_name( name )
    {};

    QString name() const { return m_name; }

private:
    QString m_name;
};

}; // ns

#endif

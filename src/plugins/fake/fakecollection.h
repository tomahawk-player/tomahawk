#ifndef FAKECOLLECTION_H
#define FAKECOLLECTION_H
#include "tomahawk/collection.h"

class FakeCollection : public Collection
{
Q_OBJECT
public:
    explicit FakeCollection(QObject *parent = 0);
    ~FakeCollection()
    {
        qDebug() << Q_FUNC_INFO;
    }

    virtual void load();

signals:

public slots:

};

#endif // FAKECOLLECTION_H

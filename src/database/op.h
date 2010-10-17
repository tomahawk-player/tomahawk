#ifndef OP_H
#define OP_H
#include <QString>
#include <QByteArray>
#include <QSharedPointer>

struct DBOp
{
    QString guid;
    QString command;
    QByteArray payload;
    bool compressed;
};

typedef QSharedPointer<DBOp> dbop_ptr;

#endif // OP_H

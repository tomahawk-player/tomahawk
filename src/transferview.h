#ifndef TRANSFERVIEW_H
#define TRANSFERVIEW_H

#include <QDebug>
#include <QTreeWidget>

#include "tomahawk/typedefs.h"

class FileTransferConnection;

class TransferView : public QTreeWidget
{
Q_OBJECT

public:
    explicit TransferView( QWidget* parent = 0 );
    virtual ~TransferView()
    {
        qDebug() << Q_FUNC_INFO;
    }

signals:

public slots:

private slots:
    void fileTransferRegistered( FileTransferConnection* ftc );
    void fileTransferFinished( FileTransferConnection* ftc );

    void onTransferUpdate();

private:
    QHash< FileTransferConnection*, int > m_index;
};

#endif // TRANSFERVIEW_H

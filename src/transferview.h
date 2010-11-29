#ifndef TRANSFERVIEW_H
#define TRANSFERVIEW_H

#include <QDebug>
#include <QTreeWidget>

#include "tomahawk/typedefs.h"
#include "utils/animatedsplitter.h"

class FileTransferConnection;

class TransferView : public AnimatedWidget
{
Q_OBJECT

public:
    explicit TransferView( AnimatedSplitter* parent = 0 );
    virtual ~TransferView()
    {
        qDebug() << Q_FUNC_INFO;
    }

    QSize sizeHint() const;

signals:

private slots:
    void fileTransferRegistered( FileTransferConnection* ftc );
    void fileTransferFinished( FileTransferConnection* ftc );

    void onTransferUpdate();

private:
    QHash< FileTransferConnection*, QPersistentModelIndex > m_index;
    QTreeWidget* m_tree;
    AnimatedSplitter* m_parent;
};

#endif // TRANSFERVIEW_H

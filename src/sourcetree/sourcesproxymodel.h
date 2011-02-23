#ifndef SOURCESPROXYMODEL_H
#define SOURCESPROXYMODEL_H

#include <QSortFilterProxyModel>

class SourcesModel;

class SourcesProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
    explicit SourcesProxyModel( SourcesModel* model, QObject* parent = 0 );

public slots:
    void showOfflineSources();
    void hideOfflineSources();

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;

private:
    SourcesModel* m_model;

    bool m_filtered;
};

#endif // SOURCESPROXYMODEL_H

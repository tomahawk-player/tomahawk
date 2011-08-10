#ifndef SETTINGSLISTDELEGATE_H
#define SETTINGSLISTDELEGATE_H

#include <QStyledItemDelegate>

class SettingsListDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SettingsListDelegate(QObject *parent = 0);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

signals:

public slots:

};

#endif // SETTINGSLISTDELEGATE_H

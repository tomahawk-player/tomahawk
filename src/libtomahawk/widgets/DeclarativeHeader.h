#ifndef DECLARATIVEHEADER_H
#define DECLARATIVEHEADER_H

#include "DeclarativeView.h"
#include "utils/TomahawkUtils.h"

#include <QTimer>

namespace Tomahawk
{

class DeclarativeHeader: public DeclarativeView
{
    Q_OBJECT
public:
    DeclarativeHeader(QWidget *parent = 0);

    void setIconSource(const QString &iconSource);
    void setCaption(const QString &caption);
    void setDescription(const QString &description);

    QSize sizeHint() const;

signals:
    void viewModeChanged(TomahawkUtils::ViewMode viewMode);
    void filterTextChanged(const QString &filterText);

public slots:
    void viewModeSelected(int viewMode);
    void setFilterText(const QString &filterText);

private slots:
    void applyFilter();

private:
    QTimer m_filterTimer;
    QString m_filter;
};

}

#endif

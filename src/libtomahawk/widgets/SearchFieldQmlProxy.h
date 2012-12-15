#ifndef SEARCHFIELDQMLPROXY
#define SEARCHFIELDQMLPROXY

#include <QGraphicsProxyWidget>

class QSearchField;

class SearchFieldQmlProxy: public QGraphicsProxyWidget
{
    Q_OBJECT
public:
    SearchFieldQmlProxy(QGraphicsItem* parent = 0);

private:
    QSearchField *m_searchField;
};

#endif

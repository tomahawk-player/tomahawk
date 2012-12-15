#ifndef SEARCHFIELDQMLPROXY
#define SEARCHFIELDQMLPROXY

#include <QGraphicsProxyWidget>

class QSearchField;

class SearchFieldQmlProxy: public QGraphicsProxyWidget
{
    Q_OBJECT
public:
    SearchFieldQmlProxy(QGraphicsItem* parent = 0);

signals:
    void textChanged(const QString &text);
    void editingFinished();
    void returnPressed();

private:
    QSearchField *m_searchField;
};

#endif

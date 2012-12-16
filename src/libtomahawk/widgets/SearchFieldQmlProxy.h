#ifndef SEARCHFIELDQMLPROXY
#define SEARCHFIELDQMLPROXY

#include <QGraphicsProxyWidget>

class QSearchField;

class SearchFieldQmlProxy: public QGraphicsProxyWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
public:
    SearchFieldQmlProxy(QGraphicsItem* parent = 0);

    QString text() const;
    void setText(const QString &text);

signals:
    void textChanged(const QString &text);
    void editingFinished();
    void returnPressed();

private:
    QSearchField *m_searchField;
};

#endif

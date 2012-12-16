#include "SearchFieldQmlProxy.h"

#include "thirdparty/Qocoa/qsearchfield.h"

SearchFieldQmlProxy::SearchFieldQmlProxy(QGraphicsItem *parent) :
    QGraphicsProxyWidget(parent)
{
    m_searchField = new QSearchField();
    m_searchField->setAttribute(Qt::WA_NoSystemBackground);
    setWidget(m_searchField);

    connect(m_searchField, SIGNAL(textChanged(QString)), SIGNAL(textChanged(QString)));
    connect(m_searchField, SIGNAL(editingFinished()), SIGNAL(editingFinished()));
    connect(m_searchField, SIGNAL(returnPressed()), SIGNAL(returnPressed()));

}

QString SearchFieldQmlProxy::text() const
{
    return m_searchField->text();
}

void SearchFieldQmlProxy::setText(const QString &text)
{
    m_searchField->setText(text);
}

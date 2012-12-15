#include "SearchFieldQmlProxy.h"

#include "thirdparty/Qocoa/qsearchfield.h"

SearchFieldQmlProxy::SearchFieldQmlProxy(QGraphicsItem *parent) :
    QGraphicsProxyWidget(parent)
{
    m_searchField = new QSearchField();
    m_searchField->setAttribute(Qt::WA_NoSystemBackground);
    setWidget(m_searchField);

}

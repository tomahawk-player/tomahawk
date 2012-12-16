#include "DeclarativeHeader.h"
#include "utils/TomahawkUtilsGui.h"

#include <QDeclarativeContext>

namespace Tomahawk
{

DeclarativeHeader::DeclarativeHeader(QWidget *parent)
    : DeclarativeView(parent)
{

    QStringList buttonList;
    buttonList << "view-toggle-icon-artist"
               << "view-toggle-icon-list"
               << "view-toggle-icon-grid";
    rootContext()->setContextProperty("buttonList", buttonList );

    setIconSource(QString());
    setCaption(QString());
    setDescription(QString());

    connect(&m_filterTimer, SIGNAL(timeout()), SLOT(applyFilter()));


    setSource( QUrl( "qrc" RESPATH "qml/DeclarativeHeader.qml" ) );
}

void DeclarativeHeader::setIconSource(const QString &iconSource)
{
    rootContext()->setContextProperty("iconSource", iconSource);
}

void DeclarativeHeader::setCaption(const QString &caption)
{
    rootContext()->setContextProperty("caption", caption);
}

void DeclarativeHeader::setDescription(const QString &description)
{
    rootContext()->setContextProperty("description", description);
}

QSize DeclarativeHeader::sizeHint() const
{
    return QSize(0, TomahawkUtils::defaultFontHeight() * 4);
}

void DeclarativeHeader::viewModeSelected(int viewMode)
{
    emit viewModeChanged((TomahawkUtils::ViewMode)viewMode);
}

void DeclarativeHeader::setFilterText(const QString &filterText)
{
    m_filter = filterText;

    m_filterTimer.stop();
    m_filterTimer.setInterval( 280 );
    m_filterTimer.setSingleShot( true );
    m_filterTimer.start();
}

void
DeclarativeHeader::applyFilter()
{
    emit filterTextChanged( m_filter );
}


}

#include "Tasteometer.h"

#include "lastfm.h"

lastfm::Tasteometer::Tasteometer()
{
}


QNetworkReply*
lastfm::Tasteometer::compare( const User& left, const User& right )
{
    QMap<QString, QString> map;
    map["method"] = "Tasteometer.compare";
    map["type1"] = "user";
    map["value1"] = left.name();
    map["type2"] = "user";
    map["value2"] = right.name();
    return lastfm::ws::get( map );
}

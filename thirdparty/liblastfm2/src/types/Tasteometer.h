#ifndef TASTEOMETER_H
#define TASTEOMETER_H

#include <lastfm/global.h>

namespace lastfm
{
    class LASTFM_DLLEXPORT Tasteometer
    {
    public:
        Tasteometer();

    public:
        static QNetworkReply* compare( const User& left, const User& right );
    };
}

#endif // TASTEOMETER_H

#include "kdtoolsglobal.h"

#include <QByteArray>

#include <algorithm>

namespace {
    struct Version {
	unsigned char v[3];
    };

    static inline bool operator<( const Version & lhs, const Version & rhs ) {
	return std::lexicographical_compare( lhs.v, lhs.v + 3, rhs.v, rhs.v + 3 );
    }
    static inline bool operator==( const Version & lhs, const Version & rhs ) {
	return std::equal( lhs.v, lhs.v + 3, rhs.v );
    }
    KDTOOLS_MAKE_RELATION_OPERATORS( Version, static inline )
}

static Version kdParseQtVersion( const char * const version ) {
    if ( !version || qstrlen( version ) < 5 || version[1] != '.' || version[3] != '.' || ( version[5] != 0 && version[5] != '.' && version[5] != '-' ) )
	return Version(); // parse error
    const Version result = { { static_cast<unsigned char>(version[0] - '0'), static_cast<unsigned char>(version[2] - '0'), static_cast<unsigned char>(version[4] - '0') } };
    return result;
}

bool _kdCheckQtVersion_impl( int major, int minor, int patchlevel ) {
    static const Version actual = kdParseQtVersion( qVersion() ); // do this only once each run...
    const Version requested = { { static_cast<unsigned char>( major ), static_cast<unsigned char>( minor ), static_cast<unsigned char>( patchlevel ) } };
    return actual >= requested;
}

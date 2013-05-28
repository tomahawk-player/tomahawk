#ifndef LIBTOMAHAWK_PLAYBACKLOG_H
#define LIBTOMAHAWK_PLAYBACKLOG_H

#include "source_ptr.h"

namespace Tomahawk
{

struct PlaybackLog
{
    Tomahawk::source_ptr source;
    unsigned int timestamp;
    unsigned int secsPlayed;
};

};

#endif // LIBTOMAHAWK_PLAYBACKLOG_H

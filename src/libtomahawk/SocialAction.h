#ifndef LIBTOMAHAWK_SOCIALACTION_H
#define LIBTOMAHAWK_SOCIALACTION_H

#include "source_ptr.h"

#include <QVariant>

namespace Tomahawk
{

struct SocialAction
{
    QVariant action;
    QVariant value;
    QVariant timestamp;
    Tomahawk::source_ptr source;
};

};

#endif // LIBTOMAHAWK_SOCIALACTION_H

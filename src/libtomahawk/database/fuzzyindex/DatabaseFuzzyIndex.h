#ifndef TOMAHAWK_DATABASEFUZZYINDEX_H
#define TOMAHAWK_DATABASEFUZZYINDEX_H

#include "FuzzyIndex.h"

namespace Tomahawk {

class DatabaseFuzzyIndex : public FuzzyIndex
{
public:
    explicit DatabaseFuzzyIndex( QObject* parent, bool wipe = false );

    virtual void updateIndex();
};

} // namespace Tomahawk

#endif // TOMAHAWK_DATABASEFUZZYINDEX_H

#ifndef COMPARE_H
#define COMPARE_H

#include <string>
#include "utils/kvEntry.h"

struct ComparePair
{
    bool operator()(const KvEntry &lhs, const KvEntry &rhs) const
    {
        if (lhs.key < rhs.key)
            return true;
        if (rhs.key < lhs.key)
            return false;

        if (lhs.timestamp != rhs.timestamp)
            return lhs.timestamp > rhs.timestamp;

        return lhs.value < rhs.value;
    }
};

#endif

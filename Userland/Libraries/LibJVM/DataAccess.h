/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Assertions.h>
#include <AK/Endian.h>
#include <LibCore/System.h>

namespace JVM {

// Some helpers to facilitate easy data access
// If there's a weird bug with data access, it's probably from these functions, as I'm not great about thinking about bounds checking.
inline u8 get_byte(u8 const* data, size_t& offset, size_t const length)
{
    if (offset >= length)
        VERIFY_NOT_REACHED();
    return *((u8 const*)(data + (offset++)));
}

inline unsigned short get_short(u8 const* data, size_t& loc, size_t const length)
{
    if (loc + 2 > length)
        VERIFY_NOT_REACHED();
    loc += 2;
    return AK::convert_between_host_and_big_endian(*((unsigned short const*)(data + (loc - 2))));
}

inline unsigned int get_int(u8 const* data, size_t& loc, size_t const length)
{
    if (loc + 4 > length)
        VERIFY_NOT_REACHED();
    loc += 4;
    return AK::convert_between_host_and_big_endian(*((unsigned int const*)(data + (loc - 4))));
}

inline unsigned long long get_long(u8 const* data, size_t& loc, size_t const length)
{
    if (loc + 8 > length)
        VERIFY_NOT_REACHED();
    loc += 8;
    return AK::convert_between_host_and_big_endian(*((unsigned long long const*)(data + (loc - 8))));
}

}

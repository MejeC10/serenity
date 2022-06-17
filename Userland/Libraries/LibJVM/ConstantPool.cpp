/*
 * Copyright (c) 2020-2021, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ConstantPool.h"

#include <LibCore/System.h>
#include <LibJVM/Class.h>

namespace JVM {

// FIXME: Actually implement this function.

void CPEntry::dump(Class const* super)
{
    outln("CPEntry dumping not implement yet!");
    super->cp_entry(0); // just here to prevent compiler complaint about unused variables.
}

}

/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/FixedArray.h>
#include <AK/RefPtr.h>
#include <LibJVM/Class.h>

namespace JVM {

class Object {
public:
private:
    FixedArray<u8> m_allocated_memory; // FIXME: Is there a better way to represent allocated memory that doesn't change size?
    RefPtr<Class> m_class;
};

}

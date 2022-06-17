/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Thread.h"

#include <AK/Vector.h>
#include <LibJVM/Class.h>

namespace JVM {

Frame Thread::current_frame()
{
    return m_stack.last();
}

void Thread::push_operand(StackValue op)
{
    m_stack.last().op_stack.append(op);
}

void Thread::inc_pc(long added_pc)
{
    m_pc += added_pc;
}

}

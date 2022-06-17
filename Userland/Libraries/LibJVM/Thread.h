/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullOwnPtr.h>
#include <AK/RefPtr.h>
#include <AK/StdLibExtras.h>
#include <AK/Vector.h>
#include <LibJVM/Class.h>
#include <LibJVM/Forward.h>
#include <LibJVM/Value.h>

namespace JVM {

struct Frame {
    AK::Vector<Value> local_variables;
    AK::Vector<StackValue> op_stack;
    AK::RefPtr<Class> rt_const_pool { nullptr };
    AK::FixedArray<u8> current_method; // Frames are created when a method is invoked and destroyed when one returns, so we can just store the code once.
    // We need this for accessing additional data in the code.
    // FIXME: Should this instead be a reference to a Code Attribute of a loaded class?
    void copy_from(Frame& other)
    {
        local_variables = other.local_variables;
        op_stack = other.op_stack;
        rt_const_pool = other.rt_const_pool;
        auto _ = current_method.try_create(other.current_method.span());
    }
    void move_from(Frame& other)
    {
        local_variables = move(other.local_variables);
        op_stack = move(other.op_stack);
        rt_const_pool.~RefPtr();
        rt_const_pool.swap(other.rt_const_pool);
        current_method.~FixedArray();
        current_method.swap(other.current_method);
    }
    Frame(Frame&& other)
    {
        move_from(other);
    }
    Frame(Frame& other)
    {
        copy_from(other);
    }
};

class Thread {

public:
    Frame current_frame();
    Frame pop_frame();
    void push_frame(Frame frame);
    void remove_frame();
    void replace_frame(Frame frame);
    long pc()
    {
        return m_pc;
    }
    void set_pc(long pc);
    void inc_pc(long added_pc);
    void push_operand(StackValue op);
    void push_local_var(Value var);

private:
    long m_pc;
    AK::Vector<Frame> m_stack;
};

}

/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Assertions.h>
#include <AK/FixedArray.h>
#include <AK/Forward.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/NonnullRefPtr.h>
#include <LibJVM/Forward.h>
#include <LibJVM/Interface.h>
#include <LibJVM/Object.h>

// FIXME: The enums used here should be replaced with the ones from Verification.h.
// FIXME: Figure out how to construct FixedArrays for unions.

namespace JVM {

enum class Type {
    Byte,
    Short,
    Int,
    Long,
    Char,
    Float,
    Double,
    ReturnAddress,
    Boolean,
    Object,
    Array,
    Interface,
    Null,
};

enum class StackType {
    Byte,
    Short,
    Int,
    Long,
    LongHighBytes,
    Char,
    Float,
    Double,
    DoubleHighBytes,
    ReturnAddress,
    Boolean,
    Object,
    Array,
    Interface,
    Null,
};

// FIZME: Implement destructors for these classes (they defaults ones are broken because of unions).

class Value {
public:
    int as_null() const
    {
        VERIFY(m_type == Type::Null);
        return 0;
    }
    explicit Value(Type type)
        : m_type(type)
    {
    }

    explicit Value()
        : m_type(Type::Null)
    {
    }
    explicit Value(int value)
        : m_type(Type::Int)
    {
        m_value.as_int = value;
    }

    Value(Value const& other)
        : m_type(other.m_type)
    {
        /*
        // This is the worst way to construct this, but it's the only way I can think of.
        // The problem we have is that we can't use initialzer lists, which is the only way I know of to init a Fixedarray.
        // We can't use them because m_value is a union.
        memcpy((void*)&m_value, (void*)&other.m_value, sizeof(m_value));
        */
        other.as_null();
    }

    Value(Value&& other)
        : m_type(other.m_type)
    {
        /*
        switch(m_type) {
            case Type::Array: m_value.as_array.swap(other.m_value.as_array); break;
            case Type::Boolean: m_value.as_bool = other.m_value.as_bool; break;
            case Type::Byte: m_value.as_byte = other.m_value.as_byte; break;
            case Type::Char: m_value.as_char = other.m_value.as_char; break;
            case Type::Double : m_value.as_bool = other.m_value.as_bool; break;
            case Type::Boolean: m_value.as_bool = other.m_value.as_bool; break;
            case Type::Boolean: m_value.as_bool = other.m_value.as_bool; break;
            case Type::Boolean: m_value.as_bool = other.m_value.as_bool; break;
        }
        */
        other.as_null(); // Just to prevent compiler complaints
    }
    void init_from_descriptor(AK::Utf8View desc);

    Type type() const { return m_type; }

private:
    Type m_type { Type::Null };
    union Val {
        int as_byte;
        int as_short;
        int as_int;
        long as_long;
        unsigned int as_char;
        float as_float;
        double as_double;
        long as_ret_address;
        bool as_bool;
        FixedArray<Value> as_array;
        Object as_object;
        Interface as_interface;
        int as_null;
        Val()
        {
            as_null = 0;
        }
        ~Val() { }
    } m_value;
};

class StackValue {
public:
    explicit StackValue(StackType type)
        : m_type(type)
    {
    }

    explicit StackValue(int value)
        : m_type(StackType::Int)
    {
        m_value.as_int = value;
    }

    StackValue(StackValue const& other)
        : m_type(other.m_type)
    {
        other.type();
        // memcpy((void*)&m_value, (void*)&other.m_value, sizeof(m_value)); // See comment from same code in Value.
    }

    StackValue(StackValue&& other)
    {
        other.type(); // Just to prevent compiler complaints
    }

    StackType type() const { return m_type; }

private:
    StackType m_type { StackType::Null };
    union Val {
        int as_byte;
        int as_short;
        int as_int;
        int as_long;
        int as_long_high_bytes;
        unsigned int as_char;
        float as_float;
        double as_double;
        long as_ret_address;
        bool as_bool;
        FixedArray<StackValue> as_array; // FIXME: Figure out how to represent this.
        int as_class;
        int as_interface;
        int as_null;
        Val()
        {
            as_null = 0;
        }
        ~Val() { }
    } m_value;
};

}

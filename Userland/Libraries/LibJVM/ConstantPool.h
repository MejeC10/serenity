/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Assertions.h>
#include <AK/FixedArray.h>
#include <AK/OwnPtr.h>
#include <AK/Utf8View.h>
#include <LibCore/System.h>
#include <LibJVM/Class.h>

namespace JVM {

enum class ReferenceKind {
    RefGetField,
    RefGetStatic,
    RefPutField,
    RefPutStatic,
    RefInvokeVirtual,
    RefInvokeStatic,
    RefInvokeSpecial,
    RefNewInvokeSpecial,
    RefInvokeInterface,
};
struct Utf8Info {
    unsigned short length;
    unsigned char const* bytes;
    Utf8Info() { }
    Utf8Info(unsigned short len, unsigned char const* ptr)
    {
        length = len;
        bytes = ptr;
    }
};

struct MethodHandleInfo {
    ReferenceKind ref_kind;
    unsigned short ref_index;
};

enum class ConstantKind {
    // The filler types exist because the tags for ConstantKinds aren't sequential: there are gaps.
    // This enum class has fillers so that tags are correctly mapped.
    // The fillers are unused, but are different than the unusable tag.
    Filler1,
    Utf8,
    Filler2,
    Integer,
    Float,
    Long,
    Double,
    Class,
    String,
    FieldRef,
    MethodRef,
    InterfaceMethodRef,
    NameAndType,
    MethodHandle,
    Filler3,
    Filler4,
    Filler5,
    MethodType,
    Dynamic,
    InvokeDynamic,
    Module,
    Package,
    Unusable, // This is a custom kind introduced to account for the fact that long and double are mandated to occupy 2 entries, even though they only use one.
    // It's a very weird decision, but this implementation follows it for now.
};

// What should we when someone passes an illegal combination of some values and a ConsantKind?
// I don't know enought about c++ to fix this, but I know that what we do currently is not correct.
// Should these be explicitly deleted in favor of custon functions, similarly to AK::FixedArray and other storage types?
// FIXME: Read the previous 2 comments and fix these constructors.

class CPEntry {
public:
    explicit CPEntry(ConstantKind constant_kind, unsigned short value)
        : m_kind(constant_kind)
    {
        if (constant_kind == ConstantKind::Class) {
            m_value.class_info = value;
        }

        else if (constant_kind == ConstantKind::String) {
            m_value.string_info = value;
        }

        else if (constant_kind == ConstantKind::MethodType) {
            m_value.method_type_info = value;
        }

        else if (constant_kind == ConstantKind::Module) {
            m_value.module_info = value;
        }

        else if (constant_kind == ConstantKind::Package) {
            m_value.package_info = value;
        }
    }

    explicit CPEntry(ConstantKind constant_kind, unsigned short value[2])
        : m_kind(constant_kind)
    {
        auto array_assign = [](unsigned short a[2], unsigned short b[2]) { a[0] = b[0]; a[1] = b[1]; };
        if (constant_kind == ConstantKind::FieldRef || constant_kind == ConstantKind::MethodRef || constant_kind == ConstantKind::InterfaceMethodRef) {
            array_assign(m_value.ref_info, value);
        }

        else if (constant_kind == ConstantKind::NameAndType) {
            array_assign(m_value.name_and_type_info, value);
        }

        else if (constant_kind == ConstantKind::Dynamic || constant_kind == ConstantKind::InvokeDynamic) {
            array_assign(m_value.dynamic_info, value);
        }
    }

    explicit CPEntry(int value)
        : m_kind(ConstantKind::Integer)
    {
        m_value.int_info = value;
    }

    explicit CPEntry(float value)
        : m_kind(ConstantKind::Float)
    {
        m_value.float_info = value;
    }

    explicit CPEntry(long long value)
        : m_kind(ConstantKind::Long)
    {
        m_value.long_info = value;
    }

    explicit CPEntry(double value)
        : m_kind(ConstantKind::Double)
    {
        m_value.double_info = value;
    }

    explicit CPEntry(unsigned short length, unsigned char const* value)
        : m_kind(ConstantKind::Utf8)
    {
        m_value.utf8_info = { length, value };
    }

    explicit CPEntry(MethodHandleInfo value)
        : m_kind(ConstantKind::MethodHandle)
    {
        m_value.method_handle_info = value;
    }

    explicit CPEntry(ConstantKind constant_kind) // This is used for the 'Unused' ConstantKind.
        : m_kind(constant_kind)
    {
    }

    CPEntry() { }

    ConstantKind kind() const { return m_kind; }

    unsigned short as_class_info();
    unsigned short* as_ref_info();
    unsigned short as_string_info();
    int as_int_info();
    float as_float_info();
    long long as_long_info();
    double as_double_info();
    unsigned short* as_name_and_type_info();
    Utf8Info as_utf8_info()
    {
        VERIFY(m_kind == ConstantKind::Utf8);
        return m_value.utf8_info;
    }
    MethodHandleInfo as_method_handle_info();
    unsigned short as_method_type_info();
    unsigned short* as_dynamic_info(); // Used for both Dynamic and InvokeDynamic.
    unsigned short as_module_info();
    unsigned short as_package_info();

    void dump(Class const* super);

private:
    ConstantKind m_kind;
    union Info {
        unsigned short class_info;
        unsigned short ref_info[2]; // Used for FieldRef, MethodRef, and InterfaceRef.
        unsigned short string_info;
        int int_info;
        float float_info;
        long long long_info;
        double double_info;
        unsigned short name_and_type_info[2];
        Utf8Info utf8_info;
        MethodHandleInfo method_handle_info;
        unsigned short method_type_info;
        unsigned short dynamic_info[2]; // Used for both Dynamic and InvokeDynamic.
        unsigned short module_info;
        unsigned short package_info;
        Info() { }
    } m_value;
};
}

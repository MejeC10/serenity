/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/FixedArray.h>
#include <AK/NonnullRefPtr.h>
#include <AK/StdLibExtras.h>
#include <AK/Utf8View.h>
#include <LibJVM/Attributes.h>
#include <LibJVM/ConstantPool.h>

namespace JVM {

class Class;
class CPEntry;

struct FieldInfo {
    short access_flags;
    short name_index;
    short descriptor_index;
    FixedArray<AttributeInfo> attributes;
    void dump(Class* const super);
    inline void move_from(FieldInfo& other)
    {
        access_flags = other.access_flags;
        name_index = other.name_index;
        descriptor_index = other.descriptor_index;
        // In order to move the value without copying the entire array, we just copy the array pointers.
        // However, first we have to deallocate our array.
        // That way, once we swap arrays, the value we're moving from has been deallocated, and we don't leak memory.
        attributes.~FixedArray();
        attributes.swap(other.attributes);
    }
    inline void move_from(FieldInfo&& other)
    {
        access_flags = other.access_flags;
        name_index = other.name_index;
        descriptor_index = other.descriptor_index;
        attributes.~FixedArray();
        attributes.swap(other.attributes);
    }
    FieldInfo(FieldInfo const& other)
        : access_flags(other.access_flags)
        , name_index(other.name_index)
        , descriptor_index(other.descriptor_index)
        , attributes(other.attributes.must_clone_but_fixme_should_propagate_errors())
    {
    }
    FieldInfo() { }
    FieldInfo(short flags, short name, short desc, FixedArray<AttributeInfo>&& attribs)
    {
        access_flags = flags;
        name_index = name;
        descriptor_index = desc;
        attributes.~FixedArray();
        attributes.swap(attribs);
    }
};

struct MethodInfo {
    short access_flags;
    short name_index;
    short descriptor_index;
    FixedArray<AttributeInfo> attributes;
    void dump(Class* const super);
    inline void move_from(MethodInfo& other)
    {
        access_flags = other.access_flags;
        name_index = other.name_index;
        descriptor_index = other.descriptor_index;
        // In order to move the value without copying the entire array, we just copy the array pointers.
        // However, first we have to deallocate our array.
        // That way, once we swap arrays, the value we're moving from has been deallocated, and we don't leak memory.
        attributes.~FixedArray();
        attributes.swap(other.attributes);
    }
    inline void move_from(MethodInfo&& other)
    {
        access_flags = other.access_flags;
        name_index = other.name_index;
        descriptor_index = other.descriptor_index;
        attributes.~FixedArray();
        attributes.swap(other.attributes);
    }
    MethodInfo(MethodInfo const& other)
        : access_flags(other.access_flags)
        , name_index(other.name_index)
        , descriptor_index(other.descriptor_index)
        , attributes(other.attributes.must_clone_but_fixme_should_propagate_errors()) // How can this be rewritten with try_clone()?
    {
    }
    MethodInfo() { }
};

class Class : public AK::RefCounted<Class> {
public:
    ErrorOr<bool> load_from_file(AK::StringView path, bool check_file);
    bool verify_const_pool();
    CPEntry cp_entry(short index) const;
    void dump(AK::StringView name);
    void move_from(Class& other)
    {
        m_minor_version = other.m_minor_version;
        m_major_version = other.m_major_version;
        m_access_flags = other.m_access_flags;
        m_this_class_index = other.m_this_class_index;
        m_super_class_index = other.m_super_class_index;
        m_constant_pool.~FixedArray();
        m_interfaces.~FixedArray();
        m_fields.~FixedArray();
        m_methods.~FixedArray();
        m_attributes.~FixedArray();
        m_constant_pool.swap(other.m_constant_pool);
        m_interfaces.swap(other.m_interfaces);
        m_fields.swap(other.m_fields);
        m_methods.swap(other.m_methods);
        m_attributes.swap(other.m_attributes);
    }

    void move_from(Class&& other)
    {
        m_minor_version = other.m_minor_version;
        m_major_version = other.m_major_version;
        m_access_flags = other.m_access_flags;
        m_this_class_index = other.m_this_class_index;
        m_super_class_index = other.m_super_class_index;
        m_constant_pool.~FixedArray();
        m_interfaces.~FixedArray();
        m_fields.~FixedArray();
        m_methods.~FixedArray();
        m_attributes.~FixedArray();
        m_constant_pool.swap(other.m_constant_pool);
        m_interfaces.swap(other.m_interfaces);
        m_fields.swap(other.m_fields);
        m_methods.swap(other.m_methods);
        m_attributes.swap(other.m_attributes);
    }
    Class(Class const& other)
        : m_minor_version(other.m_minor_version)
        , m_major_version(other.m_major_version)
        , m_constant_pool(other.m_constant_pool.must_clone_but_fixme_should_propagate_errors())
        , m_access_flags(other.m_access_flags)
        , m_this_class_index(other.m_this_class_index)
        , m_super_class_index(other.m_super_class_index)
        , m_interfaces(other.m_interfaces.must_clone_but_fixme_should_propagate_errors())
        , m_fields(other.m_fields.must_clone_but_fixme_should_propagate_errors())
        , m_methods(other.m_methods.must_clone_but_fixme_should_propagate_errors())
        , m_attributes(other.m_attributes.must_clone_but_fixme_should_propagate_errors())
    {
    }
    Class& operator=(Class&& other)
    {
        if (this != &other)
            move_from(other);
        return *this;
    }
    Class() { }
    Class(short minor, short major, FixedArray<CPEntry>& pool, short flags, short this_index, short super_index,
        FixedArray<short>& interfaces, FixedArray<FieldInfo>& fields, FixedArray<MethodInfo>& methods, FixedArray<AttributeInfo>& attributes)
        : m_minor_version(minor)
        , m_major_version(major)
        , m_constant_pool(std::move(pool))
        , m_access_flags(flags)
        , m_this_class_index(this_index)
        , m_super_class_index(super_index)
        , m_interfaces(std::move(interfaces))
        , m_fields(std::move(fields))
        , m_methods(std::move(methods))
        , m_attributes(std::move(attributes))
    {
    }

private:
    short m_minor_version { 45 };
    short m_major_version { 0 };
    FixedArray<CPEntry> m_constant_pool;
    short m_access_flags { 0 };
    short m_this_class_index { 0 };
    short m_super_class_index { 0 };
    FixedArray<short> m_interfaces;
    FixedArray<FieldInfo> m_fields;
    FixedArray<MethodInfo> m_methods;
    FixedArray<AttributeInfo> m_attributes;
};

}

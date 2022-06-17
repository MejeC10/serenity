/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/FixedArray.h>

namespace JVM {

struct Requirement {
    unsigned short requires_index;
    unsigned short requires_flags;
    unsigned short requires_version_index;
};

struct Export {
    short exports_index;
    short exports_flags;
    AK::FixedArray<short> exports_to_index;
    void copy_from(Export const& other)
    {
        exports_index = other.exports_index;
        exports_flags = other.exports_flags;
        auto _ = exports_to_index.try_create(other.exports_to_index.span());
    }
    void move_from(Export& other)
    {
        exports_index = other.exports_index;
        exports_flags = other.exports_flags;
        exports_to_index.~FixedArray();
        exports_to_index.swap(other.exports_to_index);
    }
    void move_from(Export&& other)
    {
        exports_index = other.exports_index;
        exports_flags = other.exports_flags;
        exports_to_index.~FixedArray();
        exports_to_index.swap(other.exports_to_index);
    }
    Export(Export const& other)
    {
        copy_from(other);
    }
    Export() { }
};

struct Open {
    short opens_index;
    short opens_flags;
    AK::FixedArray<short> opens_to_index;
    void copy_from(Open const& other)
    {
        opens_index = other.opens_index;
        opens_flags = other.opens_flags;
        auto _ = opens_to_index.try_create(other.opens_to_index.span());
    }
    void move_from(Open& other)
    {
        opens_index = other.opens_index;
        opens_flags = other.opens_flags;
        opens_to_index.~FixedArray();
        opens_to_index.swap(other.opens_to_index);
    }
    void move_from(Open&& other)
    {
        opens_index = other.opens_index;
        opens_flags = other.opens_flags;
        opens_to_index.~FixedArray();
        opens_to_index.swap(other.opens_to_index);
    }
    Open(Open const& other)
    {
        copy_from(other);
    }
    Open() { }
};

struct Provide {
    short provides_index;
    AK::FixedArray<short> provides_with_index;
    void copy_from(Provide const& other)
    {
        provides_index = other.provides_index;
        auto _ = provides_with_index.try_create(other.provides_with_index.span());
    }
    void move_from(Provide& other)
    {
        provides_index = other.provides_index;
        provides_with_index.~FixedArray();
        provides_with_index.swap(other.provides_with_index);
    }
    void move_from(Provide&& other)
    {
        provides_index = other.provides_index;
        provides_with_index.~FixedArray();
        provides_with_index.swap(other.provides_with_index);
    }
    Provide(Provide const& other)
    {
        copy_from(other);
    }
    Provide() { }
};

struct Module {
    short module_name_index;
    short module_flags;
    short module_version_index;
    AK::FixedArray<Requirement> requirements;
    AK::FixedArray<Export> exports;
    AK::FixedArray<Open> opens;
    AK::FixedArray<short> uses;
    AK::FixedArray<Provide> provides;
    void copy_from(Module const& other)
    {
        module_name_index = other.module_name_index;
        module_flags = other.module_flags;
        module_version_index = other.module_version_index;
        auto req = requirements.try_create(other.requirements.span());
        auto exp = exports.try_create(other.exports.span());
        auto open = opens.try_create(other.opens.span());
        auto use = uses.try_create(other.uses.span());
        auto prov = provides.try_create(other.provides.span());
    }
    void move_from(Module& other)
    {
        module_name_index = other.module_name_index;
        module_flags = other.module_flags;
        module_version_index = other.module_version_index;
        requirements.~FixedArray();
        exports.~FixedArray();
        opens.~FixedArray();
        uses.~FixedArray();
        provides.~FixedArray();
        requirements.swap(other.requirements);
        exports.swap(other.exports);
        opens.swap(other.opens);
        uses.swap(other.uses);
        provides.swap(other.provides);
    }
    void move_from(Module&& other)
    {
        module_name_index = other.module_name_index;
        module_flags = other.module_flags;
        module_version_index = other.module_version_index;
        requirements.~FixedArray();
        exports.~FixedArray();
        opens.~FixedArray();
        uses.~FixedArray();
        provides.~FixedArray();
        requirements.swap(other.requirements);
        exports.swap(other.exports);
        opens.swap(other.opens);
        uses.swap(other.uses);
        provides.swap(other.provides);
    }
};

}

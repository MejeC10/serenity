/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "JVM.h"

#include <AK/HashMap.h>
#include <AK/HashTable.h>
#include <AK/Optional.h>
#include <AK/RefPtr.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <AK/Vector.h>
#include <LibJVM/Class.h>
#include <LibJVM/Thread.h>

namespace JVM {

bool JVM::load_from_class_file(StringView path)
{
    outln("in JVM::load_from_class_file!");
    Class c;
    ErrorOr<bool> c_result = c.load_from_file(path, false);
    outln("Parsed class file!");
    if (c_result.is_error() || !c_result.value()) {
        if (c_result.is_error()) {
            outln("Crashing due to error {}", c_result.error());
        } else {
            outln("Crashing due to return false!");
        }
        return false;
    }
    outln("Attempting to set something in hashmap!");
    outln("StringView: {}", path);
    auto res = m_loaded_classes.try_set(path.to_string(), c);
    outln("Set a hashmap value!");
    if (res.is_error()) {
        outln("Error: {}", res.error());
        return false;
    }
    return true;
}

RefPtr<Class> JVM::resolve_class_reference(StringView ref)
{
    auto result = m_loaded_classes.get(ref);
    if (result.has_value()) {
        return result.value();
    } else {
        if (!load_from_class_file(ref)) {
            outln("jvm.cpp 39");
            return make_ref_counted<Class>(m_loaded_classes.get(ref).value()); // If load_from_class_file succeeded, then this call cannot fail.
        }
        outln("Load from Class file failed, crashing...");
        return nullptr;
    }
}

}

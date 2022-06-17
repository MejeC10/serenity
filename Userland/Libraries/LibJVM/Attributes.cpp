/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Attributes.h"

#include <AK/Error.h>
#include <AK/FixedArray.h>
#include <AK/Utf8View.h>
#include <LibCore/System.h>
#include <LibJVM/Class.h>
#include <LibJVM/Forward.h>
#include <LibJVM/Module.h>
#include <LibJVM/OpcodeTable.h>
#include <LibJVM/StackMapFrame.h>
#include <LibJVM/Verification.h>

namespace JVM {

void ElementValue::copy_from(ElementValue const& other)
{
    if ((tag >= 'B' && tag <= 'Z') || tag == 's') { // Ascii codes.
        // Because we check ascii code ranges, we could accidentally parse illegal char codes.
        value.const_value_index = other.value.const_value_index;
    } else if (tag == 'e') {
        value.enum_const_value = other.value.enum_const_value;
    } else if (tag == 'c') {
        value.class_info_index = other.value.class_info_index;
    } else if (tag == '@') {
        Annotation anno;
        anno.copy_from(*other.value.annotation_value.ptr());
        value.annotation_value = make<Annotation>(anno);
    } else if (tag == '[') {
        auto _ = value.array_value->try_create(other.value.array_value->span());
    } else {
        VERIFY_NOT_REACHED();
    }
}
void ElementValue::move_from(ElementValue& other)
{
    if ((tag >= 'B' && tag <= 'Z') || tag == 's') {
        value.const_value_index = other.value.const_value_index;
    } else if (tag == 'e') {
        value.enum_const_value = other.value.enum_const_value;
    } else if (tag == 'c') {
        value.class_info_index = other.value.class_info_index;
    } else if (tag == '@') {
        value.annotation_value.~NonnullOwnPtr();
        value.annotation_value.swap(other.value.annotation_value);
    } else if (tag == '[') {
        value.array_value->~FixedArray();
        AK::swap(value.array_value, other.value.array_value); // We use AK::swap because these aren't FixedArrays, they're FixedArray pointers.
    } else {
        VERIFY_NOT_REACHED();
    }
}
void ElementValue::move_from(ElementValue&& other)
{
    if ((tag >= 'B' && tag <= 'Z') || tag == 's') {
        value.const_value_index = other.value.const_value_index;
    } else if (tag == 'e') {
        value.enum_const_value = other.value.enum_const_value;
    } else if (tag == 'c') {
        value.class_info_index = other.value.class_info_index;
    } else if (tag == '@') {
        value.annotation_value.~NonnullOwnPtr();
        value.annotation_value.swap(other.value.annotation_value);
    } else if (tag == '[') {
        value.array_value->~FixedArray();
        AK::swap(value.array_value, other.value.array_value); // We use std::swap because these aren't FixedArrays, they're FixedArray pointers.
    } else {
        VERIFY_NOT_REACHED();
    }
}

void AttributeInfo::dump(Class* const super)
{
    out("Attribute Type: ");
    switch (m_attribute.m_kind) {
    case AttributeKind::ConstantValue:
        outln("ConstantValue");
        outln("ConstantValue index: {}", m_attribute.m_value.constantvalue_index.constant_value_index);
        outln("ConstantValue value:");
        super->cp_entry(m_attribute.m_value.constantvalue_index.constant_value_index).dump(super);
        break;
    case AttributeKind::Code:
        outln("Code");
        outln("Max Stack: {}", m_attribute.m_value.code->max_stack);
        outln("Max Locals: {}", m_attribute.m_value.code->max_locals);
        outln("Code:");
        for (auto&& c : m_attribute.m_value.code->code) {
            outln("{}", g_opcode_name_table[c]);
        }
        outln();
        outln("Exceptions:");
        for (auto&& e : m_attribute.m_value.code->exception_table) {
            outln("Start PC: {}", e.start_pc);
            outln("End PC: {}", e.end_pc);
            outln("Handler PC: {}", e.handler_pc);
            outln("Catch Type: {}", e.catch_type);
            outln();
        }
        outln("Attributes:");
        for (auto&& a : m_attribute.m_value.code->attributes) {
            a.dump(super);
        }
        outln();
        break;
    /*
    case AttributeKind::StackMapTable:
        outln("StackMapTable");
        outln("Number of entries: {}", m_attribute.m_value.sm_table.frames.size());
        outln("Entries:");
        for (auto&& f : m_attribute.m_value.sm_table.frames) {
            outln("Not Implemented Yet!");
        }
        outln();
        break;
    case AttributeKind::BootstrapMethods:
        outln("BootstrapMethods");
        outln("BootstrapMethods: ");
        for (auto&& m : m_attribute.m_value.bootstrap_methods->bootstrap_methods) {
            outln("Method");
            outln("");
        }
        break;
    */
    default:
        outln("Not Implemented Yet!");
        break;
    }
}

}

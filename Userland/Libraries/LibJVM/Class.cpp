/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Class.h"

#include <AK/Assertions.h>
#include <AK/FixedArray.h>
#include <AK/Span.h>
#include <AK/StdLibExtras.h>
#include <AK/StringView.h>
#include <AK/Utf8View.h>
#include <LibCore/MappedFile.h>
#include <LibCore/System.h>
#include <LibJVM/Attributes.h>
#include <LibJVM/ConstantPool.h>
#include <LibJVM/DataAccess.h>
#include <LibJVM/Module.h>

namespace JVM {
// FIXME: Replace all 'T's with 'unsigned T's.

unsigned short const SUPPORTED_MAJOR_MAX = 61;

int const MAGIC = 0xCAFEBABE;

inline Annotation parse_annotation(u8* data, size_t& loc, size_t length); // Forward delcaration

// Change MUST to TRY for these functions.

inline ElementValue parse_evalue(u8* data, size_t& loc, size_t length)
{
    ElementValue value;
    u8 tag = get_byte(data, loc, length);
    value.tag = tag;
    if ((tag > 65 && tag < 91) || tag == 115) { // Ascii codes.
        value.value.const_value_index = get_short(data, loc, length);
    } else if (tag == 'e') {
        value.value.enum_const_value = { get_short(data, loc, length), get_short(data, loc, length) };
    } else if (tag == 'c') {
        value.value.class_info_index = get_short(data, loc, length);
    } else if (tag == '@') {
        Annotation new_annot = parse_annotation(data, loc, length);
        value.value.annotation_value = make<Annotation>(new_annot);
        // We should confirm that we still hold on to the value of new_annot after leaving this function.
    } else if (tag == '[') {
        short num_vals = get_short(data, loc, length);
        value.value.array_value->~FixedArray();
        auto new_array = MUST(FixedArray<ElementValue>::try_create(num_vals));
        value.value.array_value = &new_array;
        for (int i = 0; i < num_vals; i++) {
            (*value.value.array_value)[i].move_from(parse_evalue(data, loc, length));
        }
    }
    return value;
}

inline Annotation parse_annotation(u8* data, size_t& loc, size_t length)
{
    short type_index = get_short(data, loc, length);
    short num_pairs = get_short(data, loc, length);
    Annotation annotation;
    annotation.type_index = type_index;
    annotation.element_value_pairs.~FixedArray();
    auto evpairs = MUST(FixedArray<ElementValuePair>::try_create((size_t)num_pairs));
    annotation.element_value_pairs.swap(evpairs);
    for (int i = 0; i < num_pairs; i++) {
        short index = get_short(data, loc, length);
        ElementValuePair result;
        result.element_name_index = index;
        result.value.move_from(parse_evalue(data, loc, length));
        annotation.element_value_pairs[i].move_from(result);
    }
    return annotation;
}

// TODO: Actually finish checking all the attributes for each structure!
// Some fields of the class file have been skipped over, but still have attributes that need to be parsed!
// FIXME: Fully change .try_create() to ::try_create().
// FIXME: Some of the parsing for rt_annotations is incorrect because of copy-pasting from different fields.

ErrorOr<bool> Class::load_from_file(AK::StringView path, bool check_file)
{
    outln("In Class:load_from_file!");
    // FIXME: Add more verifications! Also, we should be consistent in when we choose to debug and return false and when we choose to warn and crash.
    auto class_file_result = Core::MappedFile::map(path);
    if (class_file_result.is_error()) {
        dbgln("Error loading file from path!");
        dbgln("Error Code: {}", class_file_result.error());
        dbgln("Path: {}", path);
        return false;
    }
    auto class_file = class_file_result.release_value();
    auto class_data = (u8*)class_file->data();
    size_t size = class_file->size();
    size_t loc = 0;
    int magic = get_int(class_data, loc, size);
    if (magic != MAGIC) {
        dbgln("Error when parsing file: file doesn't contain magic number 0xCAFEBABE as header!\n");
        return false;
    }
    m_minor_version = get_short(class_data, loc, size);
    m_major_version = get_short(class_data, loc, size);
    if (m_major_version > SUPPORTED_MAJOR_MAX) {
        dbgln("Error when parsing file: file version is too advanced! Latest supported major: {}", SUPPORTED_MAJOR_MAX);
        return false;
    }
    unsigned short cp_size = get_short(class_data, loc, size) - 1;
    m_constant_pool.~FixedArray();
    auto pool = TRY(FixedArray<CPEntry>::try_create(cp_size));
    m_constant_pool.swap(pool);
    auto index = 0;
    outln("Starting parsing of constant pool");
    outln("loc: {:X}", loc);
    while (cp_size > 0) {
        ConstantKind kind = static_cast<ConstantKind>(get_byte(class_data, loc, size));
        if (kind == ConstantKind::Class || kind == ConstantKind::String) {
            m_constant_pool[index] = CPEntry(kind, get_short(class_data, loc, size));
        } else if (kind == ConstantKind::FieldRef || kind == ConstantKind::MethodRef || kind == ConstantKind::InterfaceMethodRef) {
            unsigned short ref_values[2];
            ref_values[0] = get_short(class_data, loc, size);
            ref_values[1] = get_short(class_data, loc, size);
            m_constant_pool[index] = CPEntry(kind, ref_values);
        } else if (kind == ConstantKind::Integer || kind == ConstantKind::Float) {
            m_constant_pool[index] = CPEntry(kind, get_int(class_data, loc, size));
        }

        else if (kind == ConstantKind::Long || kind == ConstantKind::Double) {
            long long val = get_long(class_data, loc, size);
            m_constant_pool[index] = CPEntry(kind, val);
            index++;
            cp_size--;
            m_constant_pool[index] = CPEntry(ConstantKind::Unusable); // The oracle spec requires that the index after a long or a double is unused.
        }

        else if (kind == ConstantKind::NameAndType) {
            unsigned short NAT_values[2];
            NAT_values[0] = get_short(class_data, loc, size);
            NAT_values[1] = get_short(class_data, loc, size);
            m_constant_pool[index] = CPEntry(ConstantKind::NameAndType, NAT_values);
        }

        else if (kind == ConstantKind::Utf8) {
            unsigned short length = get_short(class_data, loc, size);
            m_constant_pool[index] = CPEntry(length, class_data + loc);
            loc += length;
        }

        else if (kind == ConstantKind::MethodHandle) {
            ReferenceKind refkind = static_cast<ReferenceKind>(get_byte(class_data, loc, size));
            unsigned short index = get_short(class_data, loc, size);
            MethodHandleInfo MHInfo = { refkind, index };
            m_constant_pool[index] = CPEntry(MHInfo);
        }

        else if (kind == ConstantKind::MethodType) {
            unsigned short desc_index = get_short(class_data, loc, size);
            m_constant_pool[index] = CPEntry(ConstantKind::MethodType, desc_index);
        }

        else if (kind == ConstantKind::Dynamic || kind == ConstantKind::InvokeDynamic) {
            unsigned short bootstrap_method_attr_index = get_short(class_data, loc, size);
            unsigned short name_and_type_index = get_short(class_data, loc, size);
            unsigned short dynamic[2] = { bootstrap_method_attr_index, name_and_type_index };
            m_constant_pool[index] = CPEntry(kind, dynamic);
        }

        else if (kind == ConstantKind::Module) {
            unsigned short name_index = get_short(class_data, loc, size);
            m_constant_pool[index] = CPEntry(ConstantKind::Module, name_index);
        }

        else if (kind == ConstantKind::Package) {
            unsigned short name_index = get_short(class_data, loc, size);
            m_constant_pool[index] = CPEntry(ConstantKind::Package, name_index);
        }

        else {
            warnln("Error when parsing constant table! Found illegal constant tag {}", (int)kind);
            return false;
        }
        index++;
        cp_size--;
    }
    if (check_file)
        verify_const_pool();
    outln("Loc after parsing const pool: {:X}", loc);
    m_access_flags = get_short(class_data, loc, size);
    m_this_class_index = get_short(class_data, loc, size);
    m_super_class_index = get_short(class_data, loc, size);
    short num_interfaces = get_short(class_data, loc, size);
    // FIXME: Use an AK::Span and raw ptr functions here since all the elements have the same size.
    m_interfaces.~FixedArray();
    auto interfaces = TRY(FixedArray<short>::try_create((size_t)num_interfaces));
    m_interfaces.swap(interfaces);
    for (short i = 0; i < num_interfaces; i++) {
        m_interfaces[i] = get_short(class_data, loc, size);
        if (check_file) {
            CPEntry entry = cp_entry(i);
            if (entry.kind() != ConstantKind::Class) {
                warnln("Error when reading interfaces section of .class file: Interface does not point to a class structure!");
                VERIFY_NOT_REACHED();
            }
            // FIXME: Add additional verifications as per the Oracle spec;
        }
    }
    outln("Loc after parsing interfaces pool: {:X}", loc);
    // FIXME: Some attributes should only appear once in a field, and/or are required to appear. We should add confirmations for this.
    short num_fields = get_short(class_data, loc, size);
    m_fields.~FixedArray();
    auto fields = TRY(FixedArray<FieldInfo>::try_create((size_t)num_fields));
    m_fields.swap(fields);
    outln("Loading Fields");
    for (short field_index = 0; field_index < num_fields; field_index++) {
        short acc_flags = get_short(class_data, loc, size);
        short name_ind = get_short(class_data, loc, size);
        short desc_ind = get_short(class_data, loc, size);
        short attrib_count = get_short(class_data, loc, size);
        FieldInfo field = FieldInfo(acc_flags, name_ind, desc_ind, AK::FixedArray<AttributeInfo>());
        field.attributes.~FixedArray();
        auto attribs = TRY(FixedArray<AttributeInfo>::try_create(attrib_count));
        field.attributes.swap(attribs);
        for (short attrib_index = 0; attrib_index < attrib_count; attrib_index++) {
            short name_ind = get_short(class_data, loc, size);
            CPEntry entry = cp_entry(name_ind);
            if (entry.kind() != ConstantKind::Utf8) {
                warnln("Error when reading field section of .class file: Field attribute index is not Utf8!\n");
                VERIFY_NOT_REACHED();
            }
            Utf8Info name_utf8 = entry.as_utf8_info();
            StringView name = StringView(name_utf8.bytes, name_utf8.length);
            unsigned int length = get_int(class_data, loc, size);
            size_t loc_before = loc;
            if (name == "ConstantValue"sv) {
                ConstantValue cv = ConstantValue();
                if (check_file && length != 2) { // This should short-circuit in the case that we aren't verifying the file
                    warnln("Error when reading field section of .class file: Field ConstantValue attribute has length other than 2!\n");
                    VERIFY_NOT_REACHED();
                }
                cv.constant_value_index = get_short(class_data, loc, size);
                field.attributes[attrib_index].move_from(AttributeInfo(cv));
            } else if (name == "Synthetic"sv) {
                field.attributes[attrib_index].move_from(AttributeInfo(AttributeKind::Synthetic));

            } else if (name == "Deprecated"sv) {
                field.attributes[attrib_index].move_from(AttributeInfo(AttributeKind::Deprecated));

            } else if (name == "Signature"sv) {
                short sig_index = get_short(class_data, loc, size);
                Signature sig;
                sig.sig_index = sig_index;
                field.attributes[attrib_index].move_from(AttributeInfo(sig));
            } else if (name == "RuntimeVisibleAnnotations"sv) {
                short num_annotations = get_short(class_data, loc, size);
                RuntimeVisibleAnnotations rt_vis_annotations;
                auto annos = TRY(FixedArray<Annotation>::try_create((size_t)num_annotations));
                rt_vis_annotations.annotations.swap(annos);
                for (int i = 0; i < num_annotations; i++) {
                    rt_vis_annotations.annotations[i] = parse_annotation(class_data, loc, size);
                }
                field.attributes[attrib_index] = AttributeInfo(rt_vis_annotations);

            } else if (name == "RuntimeInvisibleAnnotations"sv) {
                short num_annotations = get_short(class_data, loc, size);
                RuntimeInvisibleAnnotations rt_invis_annotations;
                auto annos = TRY(FixedArray<Annotation>::try_create((size_t)num_annotations));
                rt_invis_annotations.annotations.swap(annos);
                for (int i = 0; i < num_annotations; i++) {
                    rt_invis_annotations.annotations[i] = parse_annotation(class_data, loc, size);
                }
                field.attributes[attrib_index] = AttributeInfo(rt_invis_annotations);

            }

            else if (name == "RuntimeVisibleTypeAnnotations"sv) {
                short num_annotations = get_short(class_data, loc, size);
                RuntimeVisibleTypeAnnotations rt_vis_type_annotations;
                auto annos = TRY(FixedArray<TypeAnnotation>::try_create((size_t)num_annotations));
                rt_vis_type_annotations.annotations.swap(annos);
                for (int i = 0; i < num_annotations; i++) {
                    rt_vis_type_annotations.annotations[i].target_type = get_byte(class_data, loc, size);
                    if (rt_vis_type_annotations.annotations[i].target_type == 0x13) {
                        // The target is empty, so we just pass
                    } else {
                        // Since we're in a field_info, the value for this type can only legally be 0x13 (see table 4.7.20-C).
                        // If it's not that, throw an error
                        warnln("Error: Target type of a RuntimeVisibleTypeAnnotation in a field_info struct is not 0x13!");
                        return false;
                    }
                    u8 path_length = get_byte(class_data, loc, size);
                    auto path_result = rt_vis_type_annotations.annotations[i].target_path.path.try_create((size_t)path_length);
                    for (int i = 0; i < path_length; i++) {
                        rt_vis_type_annotations.annotations[i].target_path.path[i] = { get_byte(class_data, loc, size), get_byte(class_data, loc, size) };
                    }
                    short num_pairs = get_short(class_data, loc, size);
                    rt_vis_type_annotations.annotations[i].element_value_pairs.must_create_but_fixme_should_propagate_errors((size_t)num_pairs);
                    for (int i = 0; i < num_pairs; i++) {
                        rt_vis_type_annotations.annotations[i].element_value_pairs[i].element_name_index = get_short(class_data, loc, size);
                        rt_vis_type_annotations.annotations[i].element_value_pairs[i].value = parse_evalue(class_data, loc, size);
                    }
                }
                field.attributes[attrib_index] = AttributeInfo(rt_vis_type_annotations);

            } else if (name == "RuntimeInvisibleTypeAnnotations"sv) {
                short num_annotations = get_short(class_data, loc, size);
                RuntimeInvisibleTypeAnnotations rt_invis_type_annotations;
                auto annos = TRY(FixedArray<TypeAnnotation>::try_create((size_t)num_annotations));
                rt_invis_type_annotations.annotations.swap(annos);
                for (int i = 0; i < num_annotations; i++) {
                    rt_invis_type_annotations.annotations[i].target_type = get_byte(class_data, loc, size);
                    if (rt_invis_type_annotations.annotations[i].target_type == 0x13) {
                        // The target is empty, so we just pass
                    } else {
                        // Since we're in a field_info, the value for this type can only legally be 0x13 (see table 4.7.20-C).
                        // If it's not that, throw an error
                        warnln("Error: Target type of a RuntimeVisibleTypeAnnotation in a field_info struct is not 0x13!");
                        return false;
                    }
                    u8 path_length = get_byte(class_data, loc, size);
                    rt_invis_type_annotations.annotations[i].target_path.path.must_create_but_fixme_should_propagate_errors((size_t)path_length);
                    for (int i = 0; i < path_length; i++) {
                        rt_invis_type_annotations.annotations[i].target_path.path[i] = { get_byte(class_data, loc, size), get_byte(class_data, loc, size) };
                    }
                    short num_pairs = get_short(class_data, loc, size);
                    rt_invis_type_annotations.annotations[i].element_value_pairs.must_create_but_fixme_should_propagate_errors((size_t)num_pairs);
                    for (int i = 0; i < num_pairs; i++) {
                        rt_invis_type_annotations.annotations[i].element_value_pairs[i].element_name_index = get_short(class_data, loc, size);
                        rt_invis_type_annotations.annotations[i].element_value_pairs[i].value = parse_evalue(class_data, loc, size);
                    }
                }
                field.attributes[attrib_index] = AttributeInfo(rt_invis_type_annotations);

            } else {
                Custom custom;
                custom.name_index = get_short(class_data, loc, size);
                unsigned int cus_len = get_int(class_data, loc, size);
                loc += cus_len; // Skip the data. We can't use it, so it doesn't matter.
                field.attributes[attrib_index] = AttributeInfo(custom);
            }
            if (check_file && loc_before + loc != length) {
                warnln("Size of {} attribute doesn't match length field", name);
                warnln("Size of {} attribute: {}", name, loc - loc_before);
                warnln("Given length: {}", length);
                return false;
            }
        }
        m_fields[field_index].move_from(field);
    }
    outln("loc after loading fields: {:X}", loc);
    outln("Number of fields parsed: {}", m_fields.size());
    short num_methods = get_short(class_data, loc, size);
    m_methods.~FixedArray();
    auto methods = TRY(FixedArray<MethodInfo>::try_create((size_t)num_methods));
    m_methods.swap(methods);
    outln("Loading Methods");
    for (short method_index = 0; method_index < num_methods; method_index++) {
        MethodInfo method;
        method.access_flags = get_short(class_data, loc, size);
        method.name_index = get_short(class_data, loc, size);
        method.descriptor_index = get_short(class_data, loc, size);
        short attributes_count = get_short(class_data, loc, size);
        auto mattribs = TRY(FixedArray<AttributeInfo>::try_create(attributes_count));
        method.attributes.swap(mattribs);
        for (short attrib_index = 0; attrib_index < attributes_count; attrib_index++) {
            short attrib_name_ind = get_short(class_data, loc, size);
            CPEntry name_entry = cp_entry(attrib_name_ind);
            Utf8Info name_utf8 = name_entry.as_utf8_info();
            StringView name = StringView(name_utf8.bytes, name_utf8.length);
            unsigned int length = get_int(class_data, loc, size);
            size_t loc_before = loc; // Saving for later checks.
            if (name == "Exceptions"sv) {
                short num_exceptions = get_short(class_data, loc, size);
                ExceptionTable exception;
                exception.exception_index_table.must_create_but_fixme_should_propagate_errors((size_t)num_exceptions);
                for (short exception_index = 0; exception_index < num_exceptions; exception_index++) {
                    exception.exception_index_table[exception_index] = get_short(class_data, loc, size);
                }
                method.attributes[attrib_index] = AttributeInfo(exception);
            } else if (name == "RuntimeVisibleParameterAnnotations"sv) {
                u8 num_params = get_byte(class_data, loc, size);
                RuntimeVisibleParameterAnnotations param_annos;
                param_annos.parameter_annotations.must_create_but_fixme_should_propagate_errors(num_params);
                for (u8 param_index = 0; param_index < num_params; param_index++) {
                    short num_annos = get_short(class_data, loc, size);
                    param_annos.parameter_annotations[param_index].annotations.must_create_but_fixme_should_propagate_errors(num_annos);
                    for (short i = 0; i < num_annos; i++) {
                        param_annos.parameter_annotations[param_index].annotations[i] = parse_annotation(class_data, loc, size);
                    }
                }
                method.attributes[attrib_index] = AttributeInfo(param_annos);
            } else if (name == "RuntimeInvisibleParameterAnnotations"sv) {
                u8 num_params = get_byte(class_data, loc, size);
                RuntimeInvisibleParameterAnnotations param_annos;
                param_annos.parameter_annotations.must_create_but_fixme_should_propagate_errors(num_params);
                for (u8 param_index = 0; param_index < num_params; param_index++) {
                    short num_annos = get_short(class_data, loc, size);
                    param_annos.parameter_annotations[param_index].annotations.must_create_but_fixme_should_propagate_errors(num_annos);
                    for (short i = 0; i < num_annos; i++) {
                        param_annos.parameter_annotations[param_index].annotations[i] = parse_annotation(class_data, loc, size);
                    }
                }
                method.attributes[attrib_index] = AttributeInfo(param_annos);
            } else if (name == "AnnotationDefault") {
                method.attributes[attrib_index] = AttributeInfo(AnnotationDefault(parse_evalue(class_data, loc, size)));
            } else if (name == "MethodParameters") {
                u8 params_count = get_byte(class_data, loc, size);
                MethodParameters method_params;
                method_params.parameters.must_create_but_fixme_should_propagate_errors(params_count);
                for (int i = 0; i < params_count; i++) {
                    method_params.parameters[i] = { get_short(class_data, loc, size), get_short(class_data, loc, size) };
                }
                method.attributes[attrib_index] = AttributeInfo(method_params);
            } else if (name == "Code"sv) {
                Code code;
                outln("Loc of code: {:X}", loc);
                code.max_stack = get_short(class_data, loc, size);
                code.max_locals = get_short(class_data, loc, size);
                code.code.~FixedArray();
                auto c = TRY(FixedArray<u8>::try_create(get_int(class_data, loc, size)));
                code.code.swap(c);
                outln("Size of code: {}", code.code.size());
                for (size_t i = 0; i < code.code.size(); i++) {
                    code.code[i] = get_byte(class_data, loc, size);
                }
                code.exception_table.~FixedArray();
                auto etable = TRY(FixedArray<Exception>::try_create(get_short(class_data, loc, size)));
                code.exception_table.swap(etable);
                for (size_t i = 0; i < code.exception_table.size(); i++) {
                    code.exception_table[i] = { get_short(class_data, loc, size), get_short(class_data, loc, size),
                        get_short(class_data, loc, size), get_short(class_data, loc, size) };
                }
                code.attributes.~FixedArray();
                auto attribs = TRY(FixedArray<AttributeInfo>::try_create(get_short(class_data, loc, size)));
                code.attributes.swap(attribs);
                outln("Loc before parsing code attributes: {:X}", loc);
                outln("Number of code attributes: {}", code.attributes.size());
                for (size_t i = 0; i < code.attributes.size(); i++) {
                    CPEntry c_name_entry = cp_entry(get_short(class_data, loc, size));
                    Utf8Info c_name_utf8 = c_name_entry.as_utf8_info();
                    StringView c_name = StringView(c_name_utf8.bytes, c_name_utf8.length);
                    unsigned int code_length = get_int(class_data, loc, size);
                    size_t code_loc_before = loc;
                    if (c_name == "LineNumberTable"sv) {
                        LineNumberTable table;
                        auto ln_table = TRY(FixedArray<LineNumber>::try_create(get_short(class_data, loc, size)));
                        table.line_number_table.swap(ln_table);
                        outln("Size of ln table: {}", table.line_number_table.size());
                        outln("loc of ln_table values: {:X}", loc);
                        for (size_t j = 0; j < table.line_number_table.size(); j++) {
                            table.line_number_table[j] = { get_short(class_data, loc, size), get_short(class_data, loc, size) };
                        }
                        code.attributes[i] = AttributeInfo(table);
                    } else if (c_name == "LocalVariableTable"sv) {
                        LocalVariableTable table;
                        table.local_variable_table.must_create_but_fixme_should_propagate_errors(get_short(class_data, loc, size));
                        for (size_t j = 0; j < table.local_variable_table.size(); j++) {
                            table.local_variable_table[j] = { get_short(class_data, loc, size), get_short(class_data, loc, size),
                                get_short(class_data, loc, size), get_short(class_data, loc, size), get_short(class_data, loc, size) };
                        }
                        code.attributes[i] = AttributeInfo(table);
                    } else if (c_name == "LocalVariableTypeTable"sv) {
                        LocalVariableTypeTable table;
                        table.local_variable_type_table.must_create_but_fixme_should_propagate_errors(get_short(class_data, loc, size));
                        for (size_t j = 0; j < table.local_variable_type_table.size(); j++) {
                            table.local_variable_type_table[j] = { get_short(class_data, loc, size), get_short(class_data, loc, size),
                                get_short(class_data, loc, size), get_short(class_data, loc, size), get_short(class_data, loc, size) };
                        }
                        code.attributes[i] = AttributeInfo(table);
                    } else if (c_name == "StackMapTable"sv) {
                        StackMapTable table;
                        table.frames.must_create_but_fixme_should_propagate_errors(get_short(class_data, loc, size));
                        for (size_t j = 0; j < table.frames.size(); j++) {
                            table.frames[j]->move_from(StackMapFrame(class_data, loc, size));
                        }
                        code.attributes[i] = AttributeInfo(table);
                    } else if (c_name == "RuntimeVisibleParameterAnnotations"sv) {
                        u8 num_params = get_byte(class_data, loc, size);
                        RuntimeVisibleParameterAnnotations param_annos;
                        param_annos.parameter_annotations.must_create_but_fixme_should_propagate_errors(num_params);
                        for (u8 param_index = 0; param_index < num_params; param_index++) {
                            short num_annos = get_short(class_data, loc, size);
                            param_annos.parameter_annotations[param_index].annotations.must_create_but_fixme_should_propagate_errors(num_annos);
                            for (short i = 0; i < num_annos; i++) {
                                param_annos.parameter_annotations[param_index].annotations[i] = parse_annotation(class_data, loc, size);
                            }
                        }
                        code.attributes[attrib_index] = AttributeInfo(param_annos);
                    } else if (c_name == "RuntimeInvisibleParameterAnnotations"sv) {
                        u8 num_params = get_byte(class_data, loc, size);
                        RuntimeInvisibleParameterAnnotations param_annos;
                        param_annos.parameter_annotations.must_create_but_fixme_should_propagate_errors(num_params);
                        for (u8 param_index = 0; param_index < num_params; param_index++) {
                            short num_annos = get_short(class_data, loc, size);
                            param_annos.parameter_annotations[param_index].annotations.must_create_but_fixme_should_propagate_errors(num_annos);
                            for (short i = 0; i < num_annos; i++) {
                                param_annos.parameter_annotations[param_index].annotations[i] = parse_annotation(class_data, loc, size);
                            }
                        }
                        code.attributes[attrib_index] = AttributeInfo(param_annos);
                    }
                    if (check_file && code_loc_before + loc != code_length) {
                        warnln("Size of {} attribute doesn't match length field", c_name);
                        warnln("Size of {} attribute: {}", c_name, loc - code_loc_before);
                        warnln("Given length: {}", code_length);
                        return false;
                    }
                    method.attributes[attrib_index] = AttributeInfo(code);
                }
            } else if (name == "Synthetic"sv) {
                method.attributes[attrib_index] = AttributeInfo(AttributeKind::Synthetic);
            } else if (name == "Deprecated"sv) {
                method.attributes[attrib_index] = AttributeInfo(AttributeKind::Deprecated);
            } else if (name == "Signature"sv) {
                short sig_index = get_short(class_data, loc, size);
                Signature sig;
                sig.sig_index = sig_index;
                method.attributes[attrib_index] = AttributeInfo(sig);
            } else if (name == "RuntimeVisibleAnnotations"sv) {
                short num_annotations = get_short(class_data, loc, size);
                RuntimeVisibleAnnotations rt_vis_annotations;
                auto annos = TRY(FixedArray<Annotation>::try_create(num_annotations));
                rt_vis_annotations.annotations.swap(annos);
                for (int i = 0; i < num_annotations; i++) {
                    rt_vis_annotations.annotations[i] = parse_annotation(class_data, loc, size);
                }
                method.attributes[attrib_index] = AttributeInfo(rt_vis_annotations);

            } else if (name == "RuntimeInvisibleAnnotations"sv) {
                short num_annotations = get_short(class_data, loc, size);
                RuntimeInvisibleAnnotations rt_invis_annotations;
                auto annos = TRY(FixedArray<Annotation>::try_create((size_t)num_annotations));
                rt_invis_annotations.annotations.swap(annos);
                for (int i = 0; i < num_annotations; i++) {
                    rt_invis_annotations.annotations[i] = parse_annotation(class_data, loc, size);
                }
                method.attributes[attrib_index] = AttributeInfo(rt_invis_annotations);

            } else if (name == "RuntimeVisibleTypeAnnotations"sv) {
                short num_annotations = get_short(class_data, loc, size);
                RuntimeVisibleTypeAnnotations rt_vis_type_annotations;
                rt_vis_type_annotations.annotations.must_create_but_fixme_should_propagate_errors((size_t)num_annotations);
                for (int i = 0; i < num_annotations; i++) {
                    rt_vis_type_annotations.annotations[i].target_type = get_byte(class_data, loc, size);
                    if (rt_vis_type_annotations.annotations[i].target_type == 0x13) {
                        // The target is empty, so we just pass
                    } else {
                        // Since we're in a field_info, the value for this type can only legally be 0x13 (see table 4.7.20-C).
                        // If it's not that, throw an error
                        warnln("Error: Target type of a RuntimeVisibleTypeAnnotation in a field_info struct is not 0x13!");
                        return false;
                    }
                    u8 path_length = get_byte(class_data, loc, size);
                    auto paths = TRY(FixedArray<PathEntry>::try_create((size_t)path_length));
                    rt_vis_type_annotations.annotations[i].target_path.path.swap(paths);
                    for (int i = 0; i < path_length; i++) {
                        rt_vis_type_annotations.annotations[i].target_path.path[i] = { get_byte(class_data, loc, size), get_byte(class_data, loc, size) };
                    }
                    short num_pairs = get_short(class_data, loc, size);
                    rt_vis_type_annotations.annotations[i].element_value_pairs.must_create_but_fixme_should_propagate_errors((size_t)num_pairs);
                    for (int i = 0; i < num_pairs; i++) {
                        rt_vis_type_annotations.annotations[i].element_value_pairs[i].element_name_index = get_short(class_data, loc, size);
                        rt_vis_type_annotations.annotations[i].element_value_pairs[i].value = parse_evalue(class_data, loc, size);
                    }
                }
                method.attributes[attrib_index] = AttributeInfo(rt_vis_type_annotations);

            } else if (name == "RuntimeInvisibleTypeAnnotations"sv) {
                short num_annotations = get_short(class_data, loc, size);
                RuntimeInvisibleTypeAnnotations rt_invis_type_annotations;
                rt_invis_type_annotations.annotations.must_create_but_fixme_should_propagate_errors((size_t)num_annotations);
                for (int i = 0; i < num_annotations; i++) {
                    rt_invis_type_annotations.annotations[i].target_type = get_byte(class_data, loc, size);
                    if (rt_invis_type_annotations.annotations[i].target_type == 0x13) {
                        // The target is empty, so we just pass
                    } else {
                        // Since we're in a field_info, the value for this type can only legally be 0x13 (see table 4.7.20-C).
                        // If it's not that, throw an error
                        warnln("Error: Target type of a RuntimeVisibleTypeAnnotation in a field_info struct is not 0x13!");
                        return false;
                    }
                    u8 path_length = get_byte(class_data, loc, size);
                    auto paths = TRY(FixedArray<PathEntry>::try_create((size_t)path_length));
                    rt_invis_type_annotations.annotations[i].target_path.path.swap(paths);
                    for (int i = 0; i < path_length; i++) {
                        rt_invis_type_annotations.annotations[i].target_path.path[i] = { get_byte(class_data, loc, size), get_byte(class_data, loc, size) };
                    }
                    short num_pairs = get_short(class_data, loc, size);
                    rt_invis_type_annotations.annotations[i].element_value_pairs.must_create_but_fixme_should_propagate_errors((size_t)num_pairs);
                    for (int i = 0; i < num_pairs; i++) {
                        rt_invis_type_annotations.annotations[i].element_value_pairs[i].element_name_index = get_short(class_data, loc, size);
                        rt_invis_type_annotations.annotations[i].element_value_pairs[i].value = parse_evalue(class_data, loc, size);
                    }
                }
                method.attributes[attrib_index] = AttributeInfo(rt_invis_type_annotations);

            } else {
                Custom custom;
                custom.name_index = attrib_name_ind;
                loc += length; // Skip the data. We can't use it, so it doesn't matter.
                method.attributes[attrib_index] = AttributeInfo(custom);
            }
            if (check_file && loc_before + loc != length) {
                warnln("Size of {} attribute doesn't match length field", name);
                warnln("Size of {} attribute: {}", name, loc - loc_before);
                warnln("Given length: {}", length);
                return false;
            }
        }
        m_methods[method_index].move_from(method);
    }
    outln("loc after loading methods: {:X}", loc);
    outln("Number of methods parsed: {}", m_methods.size());
    short num_attributes = get_short(class_data, loc, size);
    m_attributes.~FixedArray();
    auto attributes = TRY(FixedArray<AttributeInfo>::try_create((size_t)num_attributes));
    m_attributes.swap(attributes);
    outln("Loading Attributes");
    outln("Number of attributes to be parsed: {}", m_attributes.size());
    for (unsigned short attrib_index = 0; attrib_index < num_attributes; attrib_index++) {
        unsigned short attrib_name_ind = get_short(class_data, loc, size);
        CPEntry name_entry = cp_entry(attrib_name_ind);
        Utf8Info name_info = name_entry.as_utf8_info();
        StringView name = StringView(name_info.bytes, name_info.length);
        unsigned int length = get_int(class_data, loc, size);
        size_t loc_before = loc; // Save for later length check.
        if (name == "SourceFile"sv) {
            outln("loc of sourcefile before value: {:X}", loc);
            m_attributes[attrib_index] = AttributeInfo((SourceFile) { get_short(class_data, loc, size) });
        } else if (name == "InnerClasses"sv) {
            unsigned short num_classes = get_short(class_data, loc, size);
            InnerClassTable classes;
            auto class_table = TRY(FixedArray<InnerClass>::try_create(num_classes));
            classes.classes.swap(class_table);
            for (unsigned short i = 0; i < num_classes; i++) {
                classes.classes[i] = (InnerClass) { get_short(class_data, loc, size), get_short(class_data, loc, size),
                    get_short(class_data, loc, size), get_short(class_data, loc, size) };
            }
            m_attributes[attrib_index] = AttributeInfo(classes);
        } else if (name == "EnclosingMethod"sv) {
            m_attributes[attrib_index] = AttributeInfo((EnclosingMethod) { get_short(class_data, loc, size), get_short(class_data, loc, size) });
        } else if (name == "SourceDebugExtension"sv) {
            m_attributes[attrib_index] = AttributeInfo((SourceDebugExtension) { AK::Utf8View(AK::StringView(class_data, length)) });
        } else if (name == "BootstrapMethods"sv) {
            unsigned short num_bootstraps = get_short(class_data, loc, size);
            BootstrapMethods methods;
            auto bmethods = TRY(FixedArray<BootstrapMethod>::try_create(num_bootstraps));
            methods.bootstrap_methods.swap(bmethods);
            for (unsigned short i = 0; i < num_bootstraps; i++) {
                BootstrapMethod method;
                unsigned short method_ref = get_short(class_data, loc, size);
                unsigned short num_args = get_short(class_data, loc, size);
                method.bootstrap_method_ref = method_ref;
                auto args = TRY(FixedArray<short>::try_create(num_args));
                method.bootstrap_arguments.swap(args);
                for (unsigned short j = 0; j < num_args; j++) {
                    method.bootstrap_arguments[j] = get_short(class_data, loc, size);
                }
                methods.bootstrap_methods[i].move_from(method);
            }
            m_attributes[attrib_index] = AttributeInfo(methods);
        } else if (name == "Module"sv) {
            Module module;
            module.module_name_index = get_short(class_data, loc, size);
            module.module_flags = get_short(class_data, loc, size);
            module.module_version_index = get_short(class_data, loc, size);
            unsigned short num_requires = get_short(class_data, loc, size);
            auto reqs = TRY(FixedArray<Requirement>::try_create(num_requires));
            module.requirements.swap(reqs);
            for (unsigned short i = 0; i < num_requires; i++) {
                module.requirements[i] = (Requirement) { get_short(class_data, loc, size),
                    get_short(class_data, loc, size), get_short(class_data, loc, size) };
            }
            unsigned short num_exports = get_short(class_data, loc, size);
            auto exps = TRY(FixedArray<Export>::try_create(num_exports));
            module.exports.swap(exps);
            for (unsigned short i = 0; i < num_exports; i++) {
                Export exp;
                exp.exports_index = get_short(class_data, loc, size);
                exp.exports_flags = get_short(class_data, loc, size);
                unsigned short exports_to_count = get_short(class_data, loc, size);
                auto etoindexs = TRY(FixedArray<short>::try_create(exports_to_count));
                exp.exports_to_index.swap(etoindexs);
                for (unsigned short j = 0; j < exports_to_count; j++)
                    exp.exports_to_index[j] = get_short(class_data, loc, size);
                module.exports[i].move_from(exp);
            }
            unsigned short num_opens = get_short(class_data, loc, size);
            auto opens = TRY(FixedArray<Open>::try_create(num_opens));
            module.opens.swap(opens);
            for (unsigned short i = 0; i < num_opens; i++) {
                Open open;
                open.opens_index = get_short(class_data, loc, size);
                open.opens_flags = get_short(class_data, loc, size);
                unsigned short opens_to_count = get_short(class_data, loc, size);
                auto otoindex = TRY(FixedArray<short>::try_create(opens_to_count));
                open.opens_to_index.swap(otoindex);
                for (unsigned short j = 0; j < opens_to_count; j++)
                    open.opens_to_index[j] = get_short(class_data, loc, size);
                module.opens[i].move_from(open);
            }
            unsigned short num_uses = get_short(class_data, loc, size);
            auto uses = TRY(FixedArray<short>::try_create(num_uses));
            module.uses.swap(uses);
            for (unsigned short i = 0; i < num_uses; i++)
                module.uses[i] = get_short(class_data, loc, size);
            unsigned short num_provides = get_short(class_data, loc, size);
            auto prov = module.provides.try_create((num_provides));
            for (unsigned short i = 0; i < num_provides; i++) {
                Provide provide;
                provide.provides_index = get_short(class_data, loc, size);
                unsigned short provides_with_count = get_short(class_data, loc, size);
                auto _ = provide.provides_with_index.try_create(provides_with_count);
                for (unsigned short j = 0; j < provides_with_count; j++)
                    provide.provides_with_index[j] = get_short(class_data, loc, size);
                module.provides[i].move_from(provide);
            }
            m_attributes[attrib_index] = AttributeInfo(module);

        } else if (name == "ModulePackages"sv) {
            ModulePackages packages;
            auto _ = packages.package_index.try_create(get_short(class_data, loc, size));
            for (unsigned short i = 0; i < packages.package_index.size(); i++)
                packages.package_index[i] = get_short(class_data, loc, size);
            m_attributes[attrib_index] = AttributeInfo(packages);
        } else if (name == "ModuleMainClass"sv) {
            m_attributes[attrib_index] = AttributeInfo((ModuleMainClass) { get_short(class_data, loc, size) });
        } else if (name == "NestHost"sv) {
            m_attributes[attrib_index] = AttributeInfo((NestHost) { get_short(class_data, loc, size) });
        } else if (name == "NestMembers"sv) {
            unsigned short num_classes = get_short(class_data, loc, size);
            NestMembers members;
            auto _ = members.classes.try_create(num_classes);
            for (unsigned short i = 0; i < num_classes; i++)
                members.classes[i] = get_short(class_data, loc, size);
            m_attributes[attrib_index] = AttributeInfo(members);
        } else if (name == "Record"sv) {
            unsigned short components_count = get_short(class_data, loc, size);
            Record record;
            auto rcomponents = record.components.try_create(components_count);
            for (unsigned short component_ind = 0; component_ind < components_count; component_ind++) {
                RecordComponent component;
                component.name_index = get_short(class_data, loc, size);
                component.desc_index = get_short(class_data, loc, size);
                auto _ = component.attributes.try_create(get_short(class_data, loc, size));
                for (size_t i = 0; i < component.attributes.size(); i++) {
                    unsigned short rc_attrib_name_index = get_short(class_data, loc, size);
                    unsigned rc_attrib_len = get_short(class_data, loc, size);
                    size_t rc_loc_before = loc;
                    CPEntry rc_name_entry = cp_entry(rc_attrib_name_index);
                    Utf8Info rc_name_info = rc_name_entry.as_utf8_info();
                    StringView rc_name = StringView(rc_name_info.bytes, rc_name_info.length);
                    if (rc_name == "Signature"sv) {
                        component.attributes[i] = AttributeInfo((Signature)get_short(class_data, loc, size));
                    } else if (rc_name == "RuntimeVisibleParameterAnnotations"sv) {
                        u8 num_params = get_byte(class_data, loc, size);
                        RuntimeVisibleParameterAnnotations param_annos;
                        param_annos.parameter_annotations.must_create_but_fixme_should_propagate_errors(num_params);
                        for (u8 param_index = 0; param_index < num_params; param_index++) {
                            short num_annos = get_short(class_data, loc, size);
                            param_annos.parameter_annotations[param_index].annotations.must_create_but_fixme_should_propagate_errors(num_annos);
                            for (short i = 0; i < num_annos; i++) {
                                param_annos.parameter_annotations[param_index].annotations[i] = parse_annotation(class_data, loc, size);
                            }
                        }
                        component.attributes[i] = AttributeInfo(param_annos);
                    } else if (rc_name == "RuntimeInvisibleParameterAnnotations"sv) {
                        u8 num_params = get_byte(class_data, loc, size);
                        RuntimeInvisibleParameterAnnotations param_annos;
                        param_annos.parameter_annotations.must_create_but_fixme_should_propagate_errors(num_params);
                        for (u8 param_index = 0; param_index < num_params; param_index++) {
                            short num_annos = get_short(class_data, loc, size);
                            param_annos.parameter_annotations[param_index].annotations.must_create_but_fixme_should_propagate_errors(num_annos);
                            for (short i = 0; i < num_annos; i++) {
                                param_annos.parameter_annotations[param_index].annotations[i] = parse_annotation(class_data, loc, size);
                            }
                        }
                        component.attributes[i] = AttributeInfo(param_annos);
                    } else {
                        Custom custom;
                        custom.name_index = rc_attrib_name_index;
                        unsigned int cus_len = rc_attrib_len;
                        loc += cus_len; // Skip the data. We can't use it, so it doesn't matter.
                        component.attributes[i] = AttributeInfo(custom);
                    }
                    if (check_file && rc_loc_before + loc != rc_attrib_len) {
                        warnln("Size of {} attribute doesn't match length field", name);
                        warnln("Size of {} attribute: {}", name, loc - rc_loc_before);
                        warnln("Given length: {}", rc_attrib_len);
                        return false;
                    }
                }
            }
        } else if (name == "PermittedSubclasses"sv) {
            PermittedSubclasses subclasses;
            auto _ = subclasses.classes.try_create(get_short(class_data, loc, size));
            for (size_t i = 0; i < subclasses.classes.size(); i++) {
                subclasses.classes[i] = get_short(class_data, loc, size);
            }
            m_attributes[attrib_index] = AttributeInfo(subclasses);
        } else if (name == "Synthetic"sv) {
            m_attributes[attrib_index] = AttributeInfo(AttributeKind::Synthetic);
        } else if (name == "Deprecated"sv) {
            m_attributes[attrib_index] = AttributeInfo(AttributeKind::Deprecated);
        } else if (name == "Signature"sv) {
            short sig_index = get_short(class_data, loc, size);
            Signature sig;
            sig.sig_index = sig_index;
            m_attributes[attrib_index] = AttributeInfo(sig);
        } else if (name == "RuntimeVisibleAnnotations"sv) {
            short num_annotations = get_short(class_data, loc, size);
            RuntimeVisibleAnnotations rt_vis_annotations;
            auto _ = rt_vis_annotations.annotations.try_create((size_t)num_annotations);
            for (int i = 0; i < num_annotations; i++) {
                rt_vis_annotations.annotations[i] = parse_annotation(class_data, loc, size);
            }
            m_attributes[attrib_index] = AttributeInfo(rt_vis_annotations);

        } else if (name == "RuntimeInvisibleAnnotations"sv) {
            short num_annotations = get_short(class_data, loc, size);
            RuntimeInvisibleAnnotations rt_invis_annotations;
            rt_invis_annotations.annotations.must_create_but_fixme_should_propagate_errors((size_t)num_annotations);
            for (int i = 0; i < num_annotations; i++) {
                rt_invis_annotations.annotations[i] = parse_annotation(class_data, loc, size);
            }
            m_attributes[attrib_index] = AttributeInfo(rt_invis_annotations);

        } else if (name == "RuntimeVisibleTypeAnnotations"sv) {
            short num_annotations = get_short(class_data, loc, size);
            RuntimeVisibleTypeAnnotations rt_vis_type_annotations;
            rt_vis_type_annotations.annotations.must_create_but_fixme_should_propagate_errors((size_t)num_annotations);
            for (int i = 0; i < num_annotations; i++) {
                rt_vis_type_annotations.annotations[i].target_type = get_byte(class_data, loc, size);
                if (rt_vis_type_annotations.annotations[i].target_type == 0x13) {
                    // The target is empty, so we just pass
                } else {
                    // Since we're in a field_info, the value for this type can only legally be 0x13 (see table 4.7.20-C).
                    // If it's not that, throw an error
                    warnln("Error: Target type of a RuntimeVisibleTypeAnnotation in a field_info struct is not 0x13!");
                    return false;
                }
                u8 path_length = get_byte(class_data, loc, size);
                auto _ = rt_vis_type_annotations.annotations[i].target_path.path.try_create((size_t)path_length);
                for (int i = 0; i < path_length; i++) {
                    rt_vis_type_annotations.annotations[i].target_path.path[i] = PathEntry(get_byte(class_data, loc, size), get_byte(class_data, loc, size));
                }
                short num_pairs = get_short(class_data, loc, size);
                rt_vis_type_annotations.annotations[i].element_value_pairs.must_create_but_fixme_should_propagate_errors((size_t)num_pairs);
                for (int i = 0; i < num_pairs; i++) {
                    rt_vis_type_annotations.annotations[i].element_value_pairs[i].element_name_index = get_short(class_data, loc, size);
                    rt_vis_type_annotations.annotations[i].element_value_pairs[i].value = parse_evalue(class_data, loc, size);
                }
            }
            m_attributes[attrib_index] = AttributeInfo(rt_vis_type_annotations);

        } else if (name == "RuntimeInvisibleTypeAnnotations"sv) {
            short num_annotations = get_short(class_data, loc, size);
            RuntimeInvisibleTypeAnnotations rt_invis_type_annotations;
            rt_invis_type_annotations.annotations.must_create_but_fixme_should_propagate_errors((size_t)num_annotations);
            for (int i = 0; i < num_annotations; i++) {
                rt_invis_type_annotations.annotations[i].target_type = get_byte(class_data, loc, size);
                if (rt_invis_type_annotations.annotations[i].target_type == 0x13) {
                    // The target is empty, so we just pass
                } else {
                    // Since we're in a field_info, the value for this type can only legally be 0x13 (see table 4.7.20-C).
                    // If it's not that, throw an error
                    warnln("Error: Target type of a RuntimeVisibleTypeAnnotation in a field_info struct is not 0x13!");
                    return false;
                }
                u8 path_length = get_byte(class_data, loc, size);
                rt_invis_type_annotations.annotations[i].target_path.path.must_create_but_fixme_should_propagate_errors((size_t)path_length);
                for (int i = 0; i < path_length; i++) {
                    rt_invis_type_annotations.annotations[i].target_path.path[i] = { get_byte(class_data, loc, size), get_byte(class_data, loc, size) };
                }
                short num_pairs = get_short(class_data, loc, size);
                rt_invis_type_annotations.annotations[i].element_value_pairs.must_create_but_fixme_should_propagate_errors((size_t)num_pairs);
                for (int i = 0; i < num_pairs; i++) {
                    rt_invis_type_annotations.annotations[i].element_value_pairs[i].element_name_index = get_short(class_data, loc, size);
                    rt_invis_type_annotations.annotations[i].element_value_pairs[i].value = parse_evalue(class_data, loc, size);
                }
            }
            m_attributes[attrib_index] = AttributeInfo(rt_invis_type_annotations);

        } else {
            Custom custom;
            custom.name_index = attrib_name_ind;
            unsigned int cus_len = length;
            loc += cus_len; // Skip the data. We can't use it, so it doesn't matter.
            m_attributes[attrib_index] = AttributeInfo(custom);
        }
        if (check_file && loc_before + loc != length) {
            warnln("Size of {} attribute doesn't match length field", name);
            warnln("Size of {} attribute: {}", name, loc - loc_before);
            warnln("Given length: {}", length);
            return false;
        }
    }
    outln("Finished parsing class file!");
    return true;
}

void Class::dump(AK::StringView name)
{
    outln("Class Name: {}", name);
    outln("Class Version: {}.{}", m_major_version, m_minor_version);
    outln("Number of Constant Pool Entries: {}", m_constant_pool.size());
    outln("Constant Pool Contents:");
    for (auto&& entry : m_constant_pool) {
        entry.dump(this);
    }
    outln();
    outln("Access flags: {}", m_access_flags);
    outln("This Class:");
    cp_entry(m_this_class_index).dump(this);
    outln();
    outln("Super Class:");
    cp_entry(m_super_class_index).dump(this);
    outln();
    outln("Number of Interfaces: {}", m_constant_pool.size());
    outln("Interfaces:");
    for (auto&& interface : m_interfaces) {
        cp_entry(interface).dump(this);
    }
    outln();
    outln("Number of Fields: {}", m_fields.size());
    outln("Fields:");
    for (auto&& field : m_fields) {
        field.dump(this);
    }
    outln();
    outln("Number of Methods: {}", m_methods.size());
    outln("Methods:");
    for (auto&& method : m_methods) {
        method.dump(this);
    }
    outln();
    outln("Number of Attributes: {}", m_attributes.size());
    outln("Attributes:");
    for (auto&& attribute : m_attributes) {
        attribute.dump(this);
    }
}

void FieldInfo::dump(Class* const super)
{
    outln("Access Flags: {}", access_flags);
    outln("Name index: {}", name_index);
    out("Name: ");
    super->cp_entry(name_index).dump(super);
    outln("Desciptor Index: {}", descriptor_index);
    out("Desciptor: ");
    super->cp_entry(descriptor_index).dump(super);
    outln("Number of attributes: {}", attributes.size());
    for (auto&& attribute : attributes) {
        attribute.dump(super);
    }
}

void MethodInfo::dump(Class* const super)
{
    outln("Access Flags: {}", access_flags);
    outln("Name index: {}", name_index);
    out("Name: ");
    super->cp_entry(name_index).dump(super);
    outln("Desciptor Index: {}", descriptor_index);
    out("Desciptor: ");
    super->cp_entry(descriptor_index).dump(super);
    outln("Number of attributes: {}", attributes.size());
    for (auto&& attribute : attributes) {
        attribute.dump(super);
    }
}

// FIXME: Actually implement this function
bool Class::verify_const_pool()
{
    return true;
}

CPEntry Class::cp_entry(short index) const
{
    VERIFY(!((size_t)index >= m_constant_pool.size() || index == 0));
    return m_constant_pool[index - 1]; // The Constant Pool is indexed starting from 1. (Because 0 is an implicit reference to the current class instance, like 'this' in C++.
}

}

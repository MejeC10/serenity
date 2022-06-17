/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/FixedArray.h>
#include <AK/StdLibExtras.h>
#include <AK/Utf8View.h>
#include <LibJVM/Forward.h>
#include <LibJVM/Module.h>
#include <LibJVM/StackMapFrame.h>
#include <LibJVM/Verification.h>

// FIXME: Remove useless structs and merge structs that are only used in another struct into nameless structs.
// FIXME: Replace all 'short's with 'unsigned short's.
// FIXME: Actually handle errors! (specifically those from try_create())

namespace JVM {

struct ConstantValue {
    unsigned short constant_value_index;
    ConstantValue(short index)
    {
        constant_value_index = index;
    }
    ConstantValue() { }
};

struct Exception {
    unsigned short start_pc;
    unsigned short end_pc;
    unsigned short handler_pc;
    unsigned short catch_type;
};

struct ExceptionTable {
    AK::FixedArray<short> exception_index_table;
    void copy_from(ExceptionTable const& other)
    {
        auto _ = exception_index_table.try_create(other.exception_index_table.span());
    }
    void move_from(ExceptionTable& other)
    {
        exception_index_table.~FixedArray();
        exception_index_table.swap(other.exception_index_table);
    }
    void move_from(ExceptionTable&& other)
    {
        exception_index_table.~FixedArray();
        exception_index_table.swap(other.exception_index_table);
    }
    ExceptionTable() { }
};

struct Code {
    unsigned short max_stack;
    unsigned short max_locals;
    AK::FixedArray<u8> code;
    AK::FixedArray<Exception> exception_table;
    AK::FixedArray<AttributeInfo> attributes;
    void copy_from(Code const& other)
    {
        max_stack = other.max_stack;
        max_locals = other.max_locals;
        auto _ = code.try_create(other.code.span());
        auto etable = exception_table.try_create(other.exception_table.span());
        auto atable = attributes.try_create(other.attributes.span());
    }
    void move_from(Code& other)
    {
        max_stack = other.max_stack;
        max_locals = other.max_locals;
        code.~FixedArray();
        exception_table.~FixedArray();
        attributes.~FixedArray();
        code.swap(other.code);
        exception_table.swap(other.exception_table);
        attributes.swap(other.attributes);
    }
    void move_from(Code&& other)
    {
        max_stack = other.max_stack;
        max_locals = other.max_locals;
        code.~FixedArray();
        exception_table.~FixedArray();
        attributes.~FixedArray();
        code.swap(other.code);
        exception_table.swap(other.exception_table);
        attributes.swap(other.attributes);
    }
    Code() { }
};

struct InnerClass {
    unsigned short inner_class_info_index;
    unsigned short outer_class_info_index;
    unsigned short inner_name_index;
    unsigned short inner_class_access_flags;
};

struct InnerClassTable {
    AK::FixedArray<InnerClass> classes;
    void copy_from(InnerClassTable const& other)
    {
        auto _ = classes.try_create(other.classes.span());
    }
    void move_from(InnerClassTable& other)
    {
        auto _ = classes.try_create(other.classes.span());
    }
    void move_from(InnerClassTable&& other)
    {
        auto _ = classes.try_create(other.classes.span());
    }
};

struct EnclosingMethod {
    unsigned short class_index;
    unsigned short method_index;
};

struct Synthetic {
};

struct Signature {
    unsigned short sig_index;
};

struct SourceFile {
    unsigned short sourcefile_index;
};

struct SourceDebugExtension {
    AK::Utf8View debug_extension;
};

struct LineNumber {
    unsigned short start_pc;
    unsigned short line_number;
};

struct LineNumberTable {
    AK::FixedArray<LineNumber> line_number_table;
    void copy_from(LineNumberTable const& other)
    {
        auto _ = line_number_table.try_create(other.line_number_table.span());
    }
    void move_from(LineNumberTable& other)
    {
        line_number_table.~FixedArray();
        line_number_table.swap(other.line_number_table);
    }
    void move_from(LineNumberTable&& other)
    {
        line_number_table.~FixedArray();
        line_number_table.swap(other.line_number_table);
    }
    LineNumberTable() { }
};

struct LocalVariable {
    unsigned short start_pc;
    unsigned short length;
    unsigned short name_index;
    unsigned short descriptor_index;
    unsigned short index;
};

struct LocalVariableTable {
    AK::FixedArray<LocalVariable> local_variable_table;
    void copy_from(LocalVariableTable const& other)
    {
        auto _ = local_variable_table.try_create(other.local_variable_table.span());
    }
    void move_from(LocalVariableTable& other)
    {
        local_variable_table.~FixedArray();
        local_variable_table.swap(other.local_variable_table);
    }
    void move_from(LocalVariableTable&& other)
    {
        local_variable_table.~FixedArray();
        local_variable_table.swap(other.local_variable_table);
    }
    LocalVariableTable() { }
};

struct LocalVariableType {
    unsigned short start_pc;
    unsigned short length;
    unsigned short name_index;
    unsigned short signature_index;
    unsigned short index;
};

struct LocalVariableTypeTable {
    AK::FixedArray<LocalVariableType> local_variable_type_table;
    LocalVariableTypeTable() { }
    void copy_from(LocalVariableTypeTable const& other)
    {
        auto _ = local_variable_type_table.try_create(other.local_variable_type_table.span());
    }
    void move_from(LocalVariableTypeTable& other)
    {
        local_variable_type_table.~FixedArray();
        local_variable_type_table.swap(other.local_variable_type_table);
    }
    void move_from(LocalVariableTypeTable&& other)
    {
        local_variable_type_table.~FixedArray();
        local_variable_type_table.swap(other.local_variable_type_table);
    }
};

struct Deprecated {
};

struct EnumConstValue {
    unsigned short type_name_index;
    unsigned short const_name_index;
};

struct ElementValue {
    char tag;
    union Val {
        unsigned short const_value_index;
        EnumConstValue enum_const_value;
        unsigned short class_info_index;
        AK::NonnullOwnPtr<JVM::Annotation> annotation_value; // This has to be a pointer to resolve circular dependencies
        // Technically, this could be circular, but in reality it will end when an ElementValue doesn't use the annotation_value.
        AK::FixedArray<ElementValue>* array_value;
        Val()
        {
            const_value_index = 0;
        }
        ~Val()
        {
            annotation_value.~NonnullOwnPtr();
        }

    } value;
    ElementValue()
        : tag(0)
    {
        value.const_value_index = 0;
    }

    // These functions are forward declared because they reference Annotation functions, which are forward declared.

    void copy_from(ElementValue const& other);
    void move_from(ElementValue& other);
    void move_from(ElementValue&& other);

    ElementValue(ElementValue const& other)
    {
        copy_from(other);
    }
    ElementValue& operator=(ElementValue const& other)
    {
        if (this != &other) {
            copy_from(other);
        }
        return *this;
    }
};

struct ElementValuePair {
    unsigned short element_name_index;
    ElementValue value;
    void copy_from(ElementValuePair const& other)
    {
        element_name_index = other.element_name_index;
        value.copy_from(other.value);
    }
    void move_from(ElementValuePair& other)
    {
        element_name_index = other.element_name_index;
        value.move_from(other.value);
    }
    void move_from(ElementValuePair&& other)
    {
        element_name_index = other.element_name_index;
        value.move_from(other.value);
    }
    ElementValuePair(ElementValuePair const& other)
    {
        copy_from(other);
    }
    ElementValuePair() { }
};

struct Annotation {
    unsigned short type_index;
    AK::FixedArray<ElementValuePair> element_value_pairs;
    void copy_from(Annotation const& other)
    {
        type_index = other.type_index;
        auto _ = element_value_pairs.try_create(other.element_value_pairs.span());
    }
    void move_from(Annotation& other)
    {
        type_index = other.type_index;
        element_value_pairs.~FixedArray();
        element_value_pairs.swap(other.element_value_pairs);
    }
    void move_from(Annotation&& other)
    {
        type_index = other.type_index;
        element_value_pairs.~FixedArray();
        element_value_pairs.swap(other.element_value_pairs);
    }
    Annotation(Annotation const& other)
    {
        copy_from(other);
    }
    Annotation& operator=(Annotation const& other)
    {
        if (this != &other)
            copy_from(other);
        return *this;
    }
    Annotation& operator=(Annotation&& other)
    {
        if (this != &other)
            move_from(other);
        return *this;
    }
    Annotation() { }
};

struct RuntimeVisibleAnnotations {
    AK::FixedArray<Annotation> annotations;
    RuntimeVisibleAnnotations() { }
    void copy_from(RuntimeVisibleAnnotations const& annos)
    {
        auto _ = annotations.try_create(annos.annotations.span());
    }
    void move_from(RuntimeVisibleAnnotations& annos)
    {
        annotations.~FixedArray();
        annotations.swap(annos.annotations);
    }
    void move_from(RuntimeVisibleAnnotations&& annos)
    {
        annotations.~FixedArray();
        annotations.swap(annos.annotations);
    }
};

struct RuntimeInvisibleAnnotations {
    AK::FixedArray<Annotation> annotations;
    RuntimeInvisibleAnnotations() { }
    void copy_from(RuntimeInvisibleAnnotations const& annos)
    {
        auto _ = annotations.try_create(annos.annotations.span());
    }
    void move_from(RuntimeInvisibleAnnotations& annos)
    {
        annotations.~FixedArray();
        annotations.swap(annos.annotations);
    }
    void move_from(RuntimeInvisibleAnnotations&& annos)
    {
        annotations.~FixedArray();
        annotations.swap(annos.annotations);
    }
};

struct ParameterAnnotations {
    AK::FixedArray<Annotation> annotations;
    void copy_from(ParameterAnnotations const& annos)
    {
        auto _ = annotations.try_create(annos.annotations.span());
    }
    ParameterAnnotations(ParameterAnnotations const& annos)
    {
        copy_from(annos);
    }
    ParameterAnnotations() { }
};

struct RuntimeVisibleParameterAnnotations {
    AK::FixedArray<ParameterAnnotations> parameter_annotations;
    RuntimeVisibleParameterAnnotations() { }
    void copy_from(RuntimeVisibleParameterAnnotations const& annos)
    {
        auto _ = parameter_annotations.try_create(annos.parameter_annotations.span());
    }
    void move_from(RuntimeVisibleParameterAnnotations& annos)
    {
        parameter_annotations.~FixedArray();
        parameter_annotations.swap(annos.parameter_annotations);
    }
    void move_from(RuntimeVisibleParameterAnnotations&& annos)
    {
        parameter_annotations.~FixedArray();
        parameter_annotations.swap(annos.parameter_annotations);
    }
};

struct RuntimeInvisibleParameterAnnotations {
    AK::FixedArray<ParameterAnnotations> parameter_annotations;
    RuntimeInvisibleParameterAnnotations() { }
    void copy_from(RuntimeInvisibleParameterAnnotations const& annos)
    {
        auto _ = parameter_annotations.try_create(annos.parameter_annotations.span());
    }
    void move_from(RuntimeInvisibleParameterAnnotations& annos)
    {
        parameter_annotations.~FixedArray();
        parameter_annotations.swap(annos.parameter_annotations);
    }
    void move_from(RuntimeInvisibleParameterAnnotations&& annos)
    {
        parameter_annotations.~FixedArray();
        parameter_annotations.swap(annos.parameter_annotations);
    }
};

struct PathEntry {
    u8 type_path_kind;
    u8 type_argument_index;
    PathEntry(u8 kind, u8 index)
    {
        type_path_kind = kind;
        type_argument_index = index;
    }
    PathEntry() { }
};

struct TypePath {
    AK::FixedArray<PathEntry> path;
    void copy_from(TypePath const& other)
    {
        auto _ = path.try_create(other.path.span());
    }
    void move_from(TypePath& other)
    {
        path.~FixedArray();
        path.swap(other.path);
    }
    void move_from(TypePath&& other)
    {
        path.~FixedArray();
        path.swap(other.path);
    }
    TypePath& operator=(TypePath const& other)
    {
        if (this != &other)
            copy_from(other);
        return *this;
    }

    TypePath() { }
};

struct Localvar_Target {
    unsigned short start_pc;
    unsigned short length;
    unsigned short index;
};

struct TypeAnnotation {
    u8 target_type;
    union Target {
        u8 type_parameter_target;
        unsigned short supertype_target;
        struct {
            u8 type_parameter_index;
            u8 bound_index;
        } type_parameter_bound_target;
        u8 formal_parameter_target;
        unsigned short throws_target;
        AK::FixedArray<Localvar_Target> local_var_target;
        unsigned short catch_target;
        unsigned short offset_target;
        struct {
            unsigned short offset;
            u8 type_argument_index;
        } type_argument_target;
        Target()
        {
            type_parameter_target = 0;
        }
        ~Target()
        {
            local_var_target.~FixedArray();
        }
    } target_info;
    TypePath target_path;
    unsigned short type_index;
    AK::FixedArray<ElementValuePair> element_value_pairs;

    TypeAnnotation()
        : target_type(0)
    {
        target_info.type_parameter_target = 0;
    }
    void copy_from(TypeAnnotation const& other)
    {
        target_type = other.target_type;
        target_path = other.target_path;
        type_index = other.type_index;
        auto _ = element_value_pairs.try_create(other.element_value_pairs.span());
        switch (target_type) {
        case (0x0):
        case (0x1):
            target_info.type_parameter_target = other.target_info.type_parameter_target;
            break;
        case (0x10):
            target_info.supertype_target = other.target_info.supertype_target;
            break;
        case (0x11):
        case (0x12):
            target_info.type_parameter_bound_target = other.target_info.type_parameter_bound_target;
            break;
        case (0x13):
        case (0x14):
        case (0x15):
            // Empty target, so no copy.
            break;
        case (0x16):
            target_info.formal_parameter_target = other.target_info.formal_parameter_target;
            break;
        case (0x17):
            target_info.type_parameter_bound_target = other.target_info.type_parameter_bound_target;
            break;
        case (0x40):
        case (0x41): {
            auto _ = target_info.local_var_target.try_create(other.target_info.local_var_target.span());
        } break;
        case (0x42):
            target_info.catch_target = other.target_info.catch_target;
            break;
        case (0x43):
        case (0x44):
        case (0x45):
        case (0x46):
            target_info.type_parameter_bound_target = other.target_info.type_parameter_bound_target;
            break;
        case (0x47):
        case (0x48):
        case (0x49):
        case (0x4A):
        case (0x4B):
            target_info.type_parameter_bound_target = other.target_info.type_parameter_bound_target;
            break;
        default:
            VERIFY_NOT_REACHED();
        }
    }
    TypeAnnotation(TypeAnnotation const& other)
    {
        copy_from(other);
    }
};

struct RuntimeVisibleTypeAnnotations {
    AK::FixedArray<TypeAnnotation> annotations;
    RuntimeVisibleTypeAnnotations() { }
    void copy_from(RuntimeVisibleTypeAnnotations const& annos)
    {
        auto _ = annotations.try_create(annos.annotations.span());
    }
    void move_from(RuntimeVisibleTypeAnnotations& annos)
    {
        annotations.~FixedArray();
        annotations.swap(annos.annotations);
    }
    void move_from(RuntimeVisibleTypeAnnotations&& annos)
    {
        annotations.~FixedArray();
        annotations.swap(annos.annotations);
    }
};

struct RuntimeInvisibleTypeAnnotations {
    AK::FixedArray<TypeAnnotation> annotations;
    RuntimeInvisibleTypeAnnotations() { }
    void copy_from(RuntimeInvisibleTypeAnnotations const& annos)
    {
        auto _ = annotations.try_create(annos.annotations.span());
    }
    void move_from(RuntimeInvisibleTypeAnnotations& annos)
    {
        annotations.~FixedArray();
        annotations.swap(annos.annotations);
    }
    void move_from(RuntimeInvisibleTypeAnnotations&& annos)
    {
        annotations.~FixedArray();
        annotations.swap(annos.annotations);
    }
};

struct AnnotationDefault {
    ElementValue default_value;
    void copy_from(AnnotationDefault const& other)
    {
        default_value.copy_from(other.default_value);
    }
    void move_from(AnnotationDefault& other)
    {
        default_value.move_from(other.default_value);
    }
    void move_from(AnnotationDefault&& other)
    {
        default_value.move_from(other.default_value);
    }
};

struct BootstrapMethod {
    unsigned short bootstrap_method_ref;
    AK::FixedArray<short> bootstrap_arguments;
    void copy_from(BootstrapMethod const& other)
    {
        bootstrap_method_ref = other.bootstrap_method_ref;
        auto _ = bootstrap_arguments.try_create(other.bootstrap_arguments.span());
    }
    void move_from(BootstrapMethod& other)
    {
        bootstrap_method_ref = other.bootstrap_method_ref;
        bootstrap_arguments.~FixedArray();
        bootstrap_arguments.swap(other.bootstrap_arguments);
    }
    void move_from(BootstrapMethod&& other)
    {
        bootstrap_method_ref = other.bootstrap_method_ref;
        bootstrap_arguments.~FixedArray();
        bootstrap_arguments.swap(other.bootstrap_arguments);
    }
    BootstrapMethod(BootstrapMethod const& other)
    {
        copy_from(other);
    }
    BootstrapMethod() { }
};

struct BootstrapMethods {
    AK::FixedArray<BootstrapMethod> bootstrap_methods;
    void copy_from(BootstrapMethods const& other_methods)
    {
        auto _ = bootstrap_methods.try_create(other_methods.bootstrap_methods.span());
    }
    void move_from(BootstrapMethods& other_methods)
    {
        bootstrap_methods.~FixedArray();
        bootstrap_methods.swap(other_methods.bootstrap_methods);
    }
    void move_from(BootstrapMethods&& other_methods)
    {
        bootstrap_methods.~FixedArray();
        bootstrap_methods.swap(other_methods.bootstrap_methods);
    }
    BootstrapMethods(BootstrapMethods const& other)
    {
        copy_from(other);
    }
    BootstrapMethods& operator=(BootstrapMethods const& other)
    {
        if (this != &other)
            copy_from(other);
        return *this;
    }
    BootstrapMethods() { }
};

struct MethodParameter {
    unsigned short name_index;
    unsigned short access_flags;
};

struct MethodParameters {
    AK::FixedArray<MethodParameter> parameters;
    void copy_from(MethodParameters const& annos)
    {
        auto _ = parameters.try_create(annos.parameters.span());
    }
    void move_from(MethodParameters& annos)
    {
        parameters.~FixedArray();
        parameters.swap(annos.parameters);
    }
    void move_from(MethodParameters&& annos)
    {
        parameters.~FixedArray();
        parameters.swap(annos.parameters);
    }
    MethodParameters() { }
    MethodParameters(MethodParameters const& other)
    {
        copy_from(other);
    }
    MethodParameters& operator=(MethodParameters const& other)
    {
        if (this != &other)
            copy_from(other);
        return *this;
    }
    MethodParameters& operator=(MethodParameters&& other)
    {
        if (this != &other)
            move_from(other);
        return *this;
    }
};

struct ModulePackages {
    AK::FixedArray<short> package_index;
    void copy_from(ModulePackages const& other)
    {
        auto _ = package_index.try_create(other.package_index.span());
    }
    void move_from(ModulePackages& other)
    {
        package_index.~FixedArray();
        package_index.swap(other.package_index);
    }
    void move_from(ModulePackages&& other)
    {
        package_index.~FixedArray();
        package_index.swap(other.package_index);
    }
};

struct ModuleMainClass {
    unsigned short main_class_index;
};

struct NestHost {
    unsigned short host_class_index;
};

struct NestMembers {
    AK::FixedArray<short> classes;
    void copy_from(NestMembers const& other)
    {
        auto _ = classes.try_create(other.classes.span());
    }
    void move_from(NestMembers& other)
    {
        classes.~FixedArray();
        classes.swap(other.classes);
    }
    void move_from(NestMembers&& other)
    {
        classes.~FixedArray();
        classes.swap(other.classes);
    }
};

struct RecordComponent {
    unsigned short name_index;
    unsigned short desc_index;
    AK::FixedArray<AttributeInfo> attributes;
    void copy_from(RecordComponent const& other)
    {
        name_index = other.name_index;
        desc_index = other.desc_index;
        auto _ = attributes.try_create(other.attributes.span());
    }
    void move_from(RecordComponent& other)
    {
        name_index = other.name_index;
        desc_index = other.desc_index;
        attributes.~FixedArray();
        attributes.swap(other.attributes);
    }
    void move_from(RecordComponent&& other)
    {
        name_index = other.name_index;
        desc_index = other.desc_index;
        attributes.~FixedArray();
        attributes.swap(other.attributes);
    }
    RecordComponent(RecordComponent const& other)
    {
        copy_from(other);
    }
    RecordComponent() { }
};

struct Record {
    AK::FixedArray<RecordComponent> components;
    void copy_from(Record const& other)
    {
        auto _ = components.try_create(other.components.span());
    }
    void move_from(Record& other)
    {
        components.~FixedArray();
        components.swap(other.components);
    }
    void move_from(Record&& other)
    {
        components.~FixedArray();
        components.swap(other.components);
    }
};

struct PermittedSubclasses {
    AK::FixedArray<short> classes;
    void copy_from(PermittedSubclasses const& other)
    {
        auto _ = classes.try_create(other.classes.span());
    }
    void move_from(PermittedSubclasses& other)
    {
        classes.~FixedArray();
        classes.swap(other.classes);
    }
    void move_from(PermittedSubclasses&& other)
    {
        classes.~FixedArray();
        classes.swap(other.classes);
    }
};

struct Custom {
    unsigned short name_index;
};

enum class AttributeKind {
    ConstantValue,
    Code,
    StackMapTable,
    BootstrapMethods,
    NestHost,
    NestMembers,
    PermittedSubclasses,
    Exceptions,
    InnerClasses,
    EnclosingMethod,
    Synthetic,
    Signature,
    Record,
    SourceFile,             // optional
    LineNumberTable,        // optional
    LocalVariableTable,     // optional
    LocalVariableTypeTable, // optional
    SourceDebugExtention,   // optional
    Deprecated,             // optional
    RuntimeVisibleAnnotations,
    RuntimeInvisibleAnnotations,
    RuntimeVisibleParameterAnnotations,
    RuntimeInvisibleParameterAnnotations,
    RuntimeVisibleTypeAnnotations,
    RuntimeInvisibleTypeAnnotations,
    AnnotationDefault,
    MethodParameters,
    Module,
    ModulePackages,
    ModuleMainClass,
    Custom, // This isn't an attribute predefined by the spec, but instead a representation of a custom attribute that some compilers may emit as part of the .class file.
    // As of the writing of this comment, I have no plans to implement this, and this is simply there to catch custom attributes and ignore them.
};

// A lot ofthe union values are actually pointers to values, because the pointer is much smaller than some of the structures.
// However, I don't actually know if these values will live or if they will be deallocated.
class AttributeInfo {

public:
    AttributeInfo(AnnotationDefault def)
    {
        m_attribute.m_kind = AttributeKind::AnnotationDefault;
        m_attribute.m_value.annotation_default = &def;
    }
    AttributeInfo(ConstantValue cv)
    {
        m_attribute.m_value.constantvalue_index = cv;
        m_attribute.m_kind = AttributeKind::ConstantValue;
    }
    AttributeInfo(Signature sig)
    {
        m_attribute.m_kind = AttributeKind::Signature;
        m_attribute.m_value.signature = sig;
    }
    AttributeInfo(Custom cus)
    {
        m_attribute.m_kind = AttributeKind::Custom;
        m_attribute.m_value.custom = cus;
    }
    AttributeInfo(RuntimeVisibleAnnotations& annos)
    {
        m_attribute.m_kind = AttributeKind::RuntimeVisibleAnnotations;
        m_attribute.m_value.runtime_visible_annotations = &annos;
    }
    AttributeInfo(RuntimeInvisibleAnnotations& annos)
    {
        m_attribute.m_kind = AttributeKind::RuntimeInvisibleAnnotations;
        m_attribute.m_value.runtime_invisible_annotations = &annos;
    }
    AttributeInfo(RuntimeVisibleTypeAnnotations& annos)
    {
        m_attribute.m_kind = AttributeKind::RuntimeVisibleTypeAnnotations;
        m_attribute.m_value.runtime_visible_type_annotations = &annos;
    }
    AttributeInfo(RuntimeInvisibleTypeAnnotations& annos)
    {
        m_attribute.m_kind = AttributeKind::RuntimeInvisibleTypeAnnotations;
        m_attribute.m_value.runtime_invisible_type_annotations = &annos;
    }
    AttributeInfo(RuntimeVisibleParameterAnnotations& annos)
    {
        m_attribute.m_kind = AttributeKind::RuntimeVisibleParameterAnnotations;
        m_attribute.m_value.runtime_visible_parameter_annotations = &annos;
    }
    AttributeInfo(RuntimeInvisibleParameterAnnotations& annos)
    {
        m_attribute.m_kind = AttributeKind::RuntimeInvisibleParameterAnnotations;
        m_attribute.m_value.runtime_invisible_parameter_annotations = &annos;
    }
    AttributeInfo(MethodParameters& params)
    {
        m_attribute.m_kind = AttributeKind::MethodParameters;
        m_attribute.m_value.method_parameters = &params;
    }
    AttributeInfo(ExceptionTable& table)
    {
        m_attribute.m_kind = AttributeKind::Exceptions;
        m_attribute.m_value.exceptions = &table;
    }
    AttributeInfo(LocalVariableTypeTable& table)
    {
        m_attribute.m_kind = AttributeKind::LocalVariableTypeTable;
        m_attribute.m_value.local_variable_type_table = &table;
    }
    AttributeInfo(LocalVariableTable& table)
    {
        m_attribute.m_kind = AttributeKind::LocalVariableTable;
        m_attribute.m_value.local_variable_table = &table;
    }
    AttributeInfo(LineNumberTable& table)
    {
        m_attribute.m_kind = AttributeKind::LineNumberTable;
        m_attribute.m_value.line_number_table = &table;
    }
    AttributeInfo(StackMapTable table)
    {
        m_attribute.m_kind = AttributeKind::StackMapTable;
        m_attribute.m_value.sm_table.move_from(table);
    }
    AttributeInfo(Code& code)
    {
        m_attribute.m_kind = AttributeKind::Code;
        m_attribute.m_value.code = &code;
    }
    AttributeInfo(EnclosingMethod method)
    {
        m_attribute.m_kind = AttributeKind::EnclosingMethod;
        m_attribute.m_value.enclosing_method = method;
    }
    AttributeInfo(InnerClassTable& table)
    {
        m_attribute.m_kind = AttributeKind::InnerClasses;
        m_attribute.m_value.inner_classes = &table;
    }
    AttributeInfo(BootstrapMethods& methods)
    {
        m_attribute.m_kind = AttributeKind::BootstrapMethods;
        m_attribute.m_value.bootstrap_methods = &methods;
    }
    AttributeInfo(Module& module)
    {
        m_attribute.m_kind = AttributeKind::Module;
        m_attribute.m_value.module = &module;
    }
    AttributeInfo(ModulePackages& packages)
    {
        m_attribute.m_kind = AttributeKind::ModulePackages;
        m_attribute.m_value.module_packages = &packages;
    }
    AttributeInfo(ModuleMainClass main_class)
    {
        m_attribute.m_kind = AttributeKind::ModuleMainClass;
        m_attribute.m_value.module_main_class = main_class;
    }
    AttributeInfo(NestHost host)
    {
        m_attribute.m_kind = AttributeKind::NestHost;
        m_attribute.m_value.nest_host = host;
    }
    AttributeInfo(NestMembers& members)
    {
        m_attribute.m_kind = AttributeKind::NestMembers;
        m_attribute.m_value.nest_members = &members;
    }
    AttributeInfo(PermittedSubclasses& subclasses)
    {
        m_attribute.m_kind = AttributeKind::PermittedSubclasses;
        m_attribute.m_value.permitted_subclasses = &subclasses;
    }
    AttributeInfo(SourceFile source)
    {
        m_attribute.m_kind = AttributeKind::SourceFile;
        m_attribute.m_value.source_file = source;
    }
    AttributeInfo(SourceDebugExtension debug_ext)
    {
        m_attribute.m_kind = AttributeKind::SourceDebugExtention;
        m_attribute.m_value.debug_extension = debug_ext;
    }
    AttributeInfo(AttributeKind kind)
    {
        m_attribute.m_kind = kind;
    }
    AttributeInfo() { }

    void dump(Class* const super);

    void copy_from(AttributeInfo const& other)
    {
        m_attribute.copy_from(other.m_attribute);
    }
    void move_from(AttributeInfo& other)
    {
        m_attribute.move_from(other.m_attribute);
    }
    void move_from(AttributeInfo&& other)
    {
        m_attribute.move_from(other.m_attribute);
    }
    AttributeInfo& operator=(AttributeInfo const& other)
    {
        if (this != &other) {
            copy_from(other);
        }
        return *this;
    }
    AttributeInfo& operator=(AttributeInfo&& other)
    {
        if (this != &other) {
            move_from(other);
        }
        return *this;
    }
    AttributeInfo(AttributeInfo const& other)
    {
        copy_from(other);
    }

private:
    struct Attribute {
        AttributeKind m_kind;
        union AttributeValue {
            // Should these all be structs, or should some of them be primitives (if that's what they are)?
            // Also, should tables be wrapper structs around AK::FixedArray, or should they just directly be that?
            // FIXME: Make more of these pointers to save on struct space. (Or don't do that if that's bad design).
            // FIXME: Another problem here is that a lot of these are unecessarily pointers because the compiler complains about no default constructor otherwise. Fix this! (I don't know how).
            // FIXME: Change pointer-wrapped classes to instead be wrapped in AK::NonnullOwnPtrs.
            ConstantValue constantvalue_index;
            Code* code;
            StackMapTable sm_table;
            ExceptionTable* exceptions;
            InnerClassTable* inner_classes;
            EnclosingMethod enclosing_method;
            Synthetic synthetic;
            Signature signature;
            SourceFile source_file;
            SourceDebugExtension debug_extension;
            LineNumberTable* line_number_table;
            LocalVariableTable* local_variable_table;
            LocalVariableTypeTable* local_variable_type_table;
            Deprecated deprecated;
            RuntimeVisibleAnnotations* runtime_visible_annotations;
            RuntimeInvisibleAnnotations* runtime_invisible_annotations;
            RuntimeVisibleParameterAnnotations* runtime_visible_parameter_annotations;
            RuntimeInvisibleParameterAnnotations* runtime_invisible_parameter_annotations;
            RuntimeVisibleTypeAnnotations* runtime_visible_type_annotations;
            RuntimeInvisibleTypeAnnotations* runtime_invisible_type_annotations;
            AnnotationDefault* annotation_default;
            BootstrapMethods* bootstrap_methods;
            MethodParameters* method_parameters;
            Module* module; // This is a pointer because the strucutre is very large and would take up too much space
            ModulePackages* module_packages;
            ModuleMainClass module_main_class;
            NestHost nest_host;
            NestMembers* nest_members;
            Record* record;
            PermittedSubclasses* permitted_subclasses;
            Custom custom;
            // These functions are only here to prevent the compiler from getting mad at me and shouldn't be used.
            AttributeValue() { }
            ~AttributeValue() { }
        } m_value;
        Attribute() { }
        ~Attribute()
        {
            // Only explicitly deallocate variants that allocate on the heap (i.e. contain a FixedArray).
            if (m_kind == AttributeKind::Code)
                m_value.code->~Code();
            else if (m_kind == AttributeKind::StackMapTable)
                m_value.sm_table.~StackMapTable();
            else if (m_kind == AttributeKind::Exceptions)
                m_value.exceptions->~ExceptionTable();
            else if (m_kind == AttributeKind::InnerClasses)
                m_value.inner_classes->~InnerClassTable();
            else if (m_kind == AttributeKind::SourceDebugExtention)
                m_value.debug_extension.~SourceDebugExtension();
            else if (m_kind == AttributeKind::LineNumberTable)
                m_value.line_number_table->~LineNumberTable();
            else if (m_kind == AttributeKind::LocalVariableTable)
                m_value.local_variable_table->~LocalVariableTable();
            else if (m_kind == AttributeKind::LocalVariableTypeTable)
                m_value.local_variable_type_table->~LocalVariableTypeTable();
            else if (m_kind == AttributeKind::RuntimeVisibleAnnotations)
                m_value.runtime_visible_annotations->~RuntimeVisibleAnnotations();
            else if (m_kind == AttributeKind::RuntimeInvisibleAnnotations)
                m_value.runtime_invisible_annotations->~RuntimeInvisibleAnnotations();
            else if (m_kind == AttributeKind::RuntimeVisibleParameterAnnotations)
                m_value.runtime_visible_parameter_annotations->~RuntimeVisibleParameterAnnotations();
            else if (m_kind == AttributeKind::RuntimeInvisibleParameterAnnotations)
                m_value.runtime_invisible_parameter_annotations->~RuntimeInvisibleParameterAnnotations();
            else if (m_kind == AttributeKind::RuntimeVisibleTypeAnnotations)
                m_value.runtime_visible_type_annotations->~RuntimeVisibleTypeAnnotations();
            else if (m_kind == AttributeKind::RuntimeInvisibleTypeAnnotations)
                m_value.runtime_invisible_type_annotations->~RuntimeInvisibleTypeAnnotations();
            else if (m_kind == AttributeKind::AnnotationDefault)
                m_value.annotation_default->~AnnotationDefault();
            else if (m_kind == AttributeKind::BootstrapMethods)
                m_value.bootstrap_methods->~BootstrapMethods();
            else if (m_kind == AttributeKind::MethodParameters)
                m_value.method_parameters->~MethodParameters();
            else if (m_kind == AttributeKind::Module)
                m_value.module->~Module();
            else if (m_kind == AttributeKind::ModulePackages)
                m_value.module_packages->~ModulePackages();
            else if (m_kind == AttributeKind::NestMembers)
                m_value.nest_members->~NestMembers();
            else if (m_kind == AttributeKind::Record)
                m_value.record->~Record();
            else if (m_kind == AttributeKind::PermittedSubclasses)
                m_value.permitted_subclasses->~PermittedSubclasses();
            else if (m_kind == AttributeKind::Custom)
                m_value.custom.~Custom();
        }
        void copy_from(Attribute const& other)
        {
            switch (m_kind) {
            case AttributeKind::ConstantValue:
                m_value.constantvalue_index = other.m_value.constantvalue_index;
                break;
            case AttributeKind::Code:
                m_value.code->copy_from(*other.m_value.code);
                break;
            case AttributeKind::StackMapTable:
                m_value.sm_table.copy_from(other.m_value.sm_table);
                break;
            case AttributeKind::Exceptions:
                m_value.exceptions->copy_from(*other.m_value.exceptions);
                break;
            case AttributeKind::InnerClasses:
                m_value.inner_classes->copy_from(*other.m_value.inner_classes);
                break;
            case AttributeKind::EnclosingMethod:
                m_value.enclosing_method = other.m_value.enclosing_method;
                break;
            case AttributeKind::Synthetic:
                m_value.synthetic = other.m_value.synthetic;
                break;
            case AttributeKind::Signature:
                m_value.signature = other.m_value.signature;
                break;
            case AttributeKind::SourceFile:
                m_value.source_file = other.m_value.source_file;
                break;
            case AttributeKind::SourceDebugExtention:
                m_value.debug_extension = other.m_value.debug_extension;
                break;
            case AttributeKind::LineNumberTable:
                m_value.line_number_table->copy_from(*other.m_value.line_number_table);
                break;
            case AttributeKind::LocalVariableTable:
                m_value.local_variable_table->copy_from(*other.m_value.local_variable_table);
                break;
            case AttributeKind::LocalVariableTypeTable:
                m_value.local_variable_type_table->copy_from(*other.m_value.local_variable_type_table);
                break;
            case AttributeKind::Deprecated:
                m_value.deprecated = other.m_value.deprecated;
                break;
            case AttributeKind::RuntimeVisibleAnnotations:
                m_value.runtime_visible_annotations->copy_from(*other.m_value.runtime_visible_annotations);
                break;
            case AttributeKind::RuntimeInvisibleAnnotations:
                m_value.runtime_invisible_annotations->copy_from(*other.m_value.runtime_invisible_annotations);
                break;
            case AttributeKind::RuntimeVisibleParameterAnnotations:
                m_value.runtime_visible_parameter_annotations->copy_from(*other.m_value.runtime_visible_parameter_annotations);
                break;
            case AttributeKind::RuntimeInvisibleParameterAnnotations:
                m_value.runtime_invisible_parameter_annotations->copy_from(*other.m_value.runtime_invisible_parameter_annotations);
                break;
            case AttributeKind::RuntimeVisibleTypeAnnotations:
                m_value.runtime_visible_type_annotations->copy_from(*other.m_value.runtime_visible_type_annotations);
                break;
            case AttributeKind::RuntimeInvisibleTypeAnnotations:
                m_value.runtime_invisible_type_annotations->copy_from(*other.m_value.runtime_invisible_type_annotations);
                break;
            case AttributeKind::AnnotationDefault:
                m_value.annotation_default->copy_from(*other.m_value.annotation_default);
                break;
            case AttributeKind::BootstrapMethods:
                m_value.bootstrap_methods->copy_from(*other.m_value.bootstrap_methods);
                break;
            case AttributeKind::MethodParameters:
                m_value.method_parameters->copy_from(*other.m_value.method_parameters);
                break;
            case AttributeKind::Module:
                m_value.module->copy_from(*other.m_value.module);
                break;
            case AttributeKind::ModulePackages:
                m_value.module_packages->copy_from(*other.m_value.module_packages);
                break;
            case AttributeKind::ModuleMainClass:
                m_value.module_main_class = other.m_value.module_main_class;
                break;
            case AttributeKind::NestHost:
                m_value.nest_host = other.m_value.nest_host;
                break;
            case AttributeKind::NestMembers:
                m_value.nest_members->copy_from(*other.m_value.nest_members);
                break;
            case AttributeKind::Record:
                m_value.record->copy_from(*other.m_value.record);
                break;
            case AttributeKind::PermittedSubclasses:
                m_value.permitted_subclasses->copy_from(*other.m_value.permitted_subclasses);
                break;
            case AttributeKind::Custom:
                m_value.custom = other.m_value.custom;
                break;
            }
        }
        void move_from(Attribute& other)
        {
            switch (m_kind) {
            case AttributeKind::ConstantValue:
                m_value.constantvalue_index = other.m_value.constantvalue_index;
                break;
            case AttributeKind::Code:
                m_value.code->move_from(*other.m_value.code);
                break;
            case AttributeKind::StackMapTable:
                m_value.sm_table.move_from(other.m_value.sm_table);
                break;
            case AttributeKind::Exceptions:
                m_value.exceptions->move_from(*other.m_value.exceptions);
                break;
            case AttributeKind::InnerClasses:
                m_value.inner_classes->move_from(*other.m_value.inner_classes);
                break;
            case AttributeKind::EnclosingMethod:
                m_value.enclosing_method = other.m_value.enclosing_method;
                break;
            case AttributeKind::Synthetic:
                m_value.synthetic = other.m_value.synthetic;
                break;
            case AttributeKind::Signature:
                m_value.signature = other.m_value.signature;
                break;
            case AttributeKind::SourceFile:
                m_value.source_file = other.m_value.source_file;
                break;
            case AttributeKind::SourceDebugExtention:
                m_value.debug_extension = other.m_value.debug_extension;
                break;
            case AttributeKind::LineNumberTable:
                m_value.line_number_table->move_from(*other.m_value.line_number_table);
                break;
            case AttributeKind::LocalVariableTable:
                m_value.local_variable_table->move_from(*other.m_value.local_variable_table);
                break;
            case AttributeKind::LocalVariableTypeTable:
                m_value.local_variable_type_table->move_from(*other.m_value.local_variable_type_table);
                break;
            case AttributeKind::Deprecated:
                m_value.deprecated = other.m_value.deprecated;
                break;
            case AttributeKind::RuntimeVisibleAnnotations:
                m_value.runtime_visible_annotations->move_from(*other.m_value.runtime_visible_annotations);
                break;
            case AttributeKind::RuntimeInvisibleAnnotations:
                m_value.runtime_invisible_annotations->move_from(*other.m_value.runtime_invisible_annotations);
                break;
            case AttributeKind::RuntimeVisibleParameterAnnotations:
                m_value.runtime_visible_parameter_annotations->move_from(*other.m_value.runtime_visible_parameter_annotations);
                break;
            case AttributeKind::RuntimeInvisibleParameterAnnotations:
                m_value.runtime_invisible_parameter_annotations->move_from(*other.m_value.runtime_invisible_parameter_annotations);
                break;
            case AttributeKind::RuntimeVisibleTypeAnnotations:
                m_value.runtime_visible_type_annotations->move_from(*other.m_value.runtime_visible_type_annotations);
                break;
            case AttributeKind::RuntimeInvisibleTypeAnnotations:
                m_value.runtime_invisible_type_annotations->move_from(*other.m_value.runtime_invisible_type_annotations);
                break;
            case AttributeKind::AnnotationDefault:
                m_value.annotation_default->move_from(*other.m_value.annotation_default);
                break;
            case AttributeKind::BootstrapMethods:
                m_value.bootstrap_methods->move_from(*other.m_value.bootstrap_methods);
                break;
            case AttributeKind::MethodParameters:
                m_value.method_parameters->move_from(*other.m_value.method_parameters);
                break;
            case AttributeKind::Module:
                m_value.module->move_from(*other.m_value.module);
                break;
            case AttributeKind::ModulePackages:
                m_value.module_packages->move_from(*other.m_value.module_packages);
                break;
            case AttributeKind::ModuleMainClass:
                m_value.module_main_class = other.m_value.module_main_class;
                break;
            case AttributeKind::NestHost:
                m_value.nest_host = other.m_value.nest_host;
                break;
            case AttributeKind::NestMembers:
                m_value.nest_members->move_from(*other.m_value.nest_members);
                break;
            case AttributeKind::Record:
                m_value.record->move_from(*other.m_value.record);
                break;
            case AttributeKind::PermittedSubclasses:
                m_value.permitted_subclasses->move_from(*other.m_value.permitted_subclasses);
                break;
            case AttributeKind::Custom:
                m_value.custom = other.m_value.custom;
                break;
            }
        }
        void move_from(Attribute&& other)
        {
            switch (m_kind) {
            case AttributeKind::ConstantValue:
                m_value.constantvalue_index = other.m_value.constantvalue_index;
                break;
            case AttributeKind::Code:
                m_value.code->move_from(*other.m_value.code);
                break;
            case AttributeKind::StackMapTable:
                m_value.sm_table.move_from(other.m_value.sm_table);
                break;
            case AttributeKind::Exceptions:
                m_value.exceptions->move_from(*other.m_value.exceptions);
                break;
            case AttributeKind::InnerClasses:
                m_value.inner_classes->move_from(*other.m_value.inner_classes);
                break;
            case AttributeKind::EnclosingMethod:
                m_value.enclosing_method = other.m_value.enclosing_method;
                break;
            case AttributeKind::Synthetic:
                m_value.synthetic = other.m_value.synthetic;
                break;
            case AttributeKind::Signature:
                m_value.signature = other.m_value.signature;
                break;
            case AttributeKind::SourceFile:
                m_value.source_file = other.m_value.source_file;
                break;
            case AttributeKind::SourceDebugExtention:
                m_value.debug_extension = other.m_value.debug_extension;
                break;
            case AttributeKind::LineNumberTable:
                m_value.line_number_table->move_from(*other.m_value.line_number_table);
                break;
            case AttributeKind::LocalVariableTable:
                m_value.local_variable_table->move_from(*other.m_value.local_variable_table);
                break;
            case AttributeKind::LocalVariableTypeTable:
                m_value.local_variable_type_table->move_from(*other.m_value.local_variable_type_table);
                break;
            case AttributeKind::Deprecated:
                m_value.deprecated = other.m_value.deprecated;
                break;
            case AttributeKind::RuntimeVisibleAnnotations:
                m_value.runtime_visible_annotations->move_from(*other.m_value.runtime_visible_annotations);
                break;
            case AttributeKind::RuntimeInvisibleAnnotations:
                m_value.runtime_invisible_annotations->move_from(*other.m_value.runtime_invisible_annotations);
                break;
            case AttributeKind::RuntimeVisibleParameterAnnotations:
                m_value.runtime_visible_parameter_annotations->move_from(*other.m_value.runtime_visible_parameter_annotations);
                break;
            case AttributeKind::RuntimeInvisibleParameterAnnotations:
                m_value.runtime_invisible_parameter_annotations->move_from(*other.m_value.runtime_invisible_parameter_annotations);
                break;
            case AttributeKind::RuntimeVisibleTypeAnnotations:
                m_value.runtime_visible_type_annotations->move_from(*other.m_value.runtime_visible_type_annotations);
                break;
            case AttributeKind::RuntimeInvisibleTypeAnnotations:
                m_value.runtime_invisible_type_annotations->move_from(*other.m_value.runtime_invisible_type_annotations);
                break;
            case AttributeKind::AnnotationDefault:
                m_value.annotation_default->move_from(*other.m_value.annotation_default);
                break;
            case AttributeKind::BootstrapMethods:
                m_value.bootstrap_methods->move_from(*other.m_value.bootstrap_methods);
                break;
            case AttributeKind::MethodParameters:
                m_value.method_parameters->move_from(*other.m_value.method_parameters);
                break;
            case AttributeKind::Module:
                m_value.module->move_from(*other.m_value.module);
                break;
            case AttributeKind::ModulePackages:
                m_value.module_packages->move_from(*other.m_value.module_packages);
                break;
            case AttributeKind::ModuleMainClass:
                m_value.module_main_class = other.m_value.module_main_class;
                break;
            case AttributeKind::NestHost:
                m_value.nest_host = other.m_value.nest_host;
                break;
            case AttributeKind::NestMembers:
                m_value.nest_members->move_from(*other.m_value.nest_members);
                break;
            case AttributeKind::Record:
                m_value.record->move_from(*other.m_value.record);
                break;
            case AttributeKind::PermittedSubclasses:
                m_value.permitted_subclasses->move_from(*other.m_value.permitted_subclasses);
                break;
            case AttributeKind::Custom:
                m_value.custom = other.m_value.custom;
                break;
            }
        }
        Attribute(Attribute const& other)
        {
            copy_from(other);
        }
        Attribute(Attribute&& other)
        {
            move_from(other);
        }
        Attribute& operator=(Attribute const& other)
        {
            if (this != &other)
                copy_from(other);
            return *this;
        }
        Attribute& operator=(Attribute&& other)
        {
            if (this != &other)
                move_from(other);
            return *this;
        }
    } m_attribute;
};

}

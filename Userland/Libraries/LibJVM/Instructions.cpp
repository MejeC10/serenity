/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Error.h>
#include <AK/Utf8View.h>
#include <LibJVM/Class.h>
#include <LibJVM/ConstantPool.h>
#include <LibJVM/Instructions.h>
#include <LibJVM/JVM.h>
#include <LibJVM/Thread.h>
#include <LibJVM/Value.h>

namespace JVM {

// The current design requires that method increment their own PCs (including the one that represents the opcode).

// The calls to jvm.nop() exist to silence warnings about not using the JVM argument.
// All the instructions have to have the same signature, so they all have a jvm argument.
// If it turns out that no function needs a reference to the JVM, these should be removed

// FIXME: Switch JVM args to JVM&s.

ErrorOr<int> nop(JVM jvm, Thread thread)
{
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> aconst_null(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue(StackType::Null));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> iconst_m1(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((int)-1));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> iconst_0(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((int)0));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> iconst_1(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((int)1));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> iconst_2(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((int)2));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> iconst_3(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((int)3));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> iconst_4(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((int)4));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> iconst_5(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((int)5));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> lconst_0(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((long)0));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> lconst_1(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((long)1));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> fconst_0(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((float)0.0));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> fconst_1(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((float)1.0));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> fconst_2(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((float)2.0));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> dconst_0(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((double)0.0));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> dconst_1(JVM jvm, Thread thread)
{
    thread.push_operand(StackValue((double)1.0));
    thread.inc_pc(1);
    jvm.nop();
    return 0;
}
ErrorOr<int> bipush(JVM jvm, Thread thread)
{
    u8 byte = thread.current_frame().current_method[thread.pc() + 1]; // offset the opcode
    thread.push_operand(StackValue(byte));
    thread.inc_pc(2);
    jvm.nop();
    return 0;
}
ErrorOr<int> sipush(JVM jvm, Thread thread)
{
    short byte_upper = thread.current_frame().current_method[thread.pc() + 1];
    short byte_lower = thread.current_frame().current_method[thread.pc() + 2];
    short s = (byte_upper << 8) | byte_lower;
    thread.push_operand(StackValue(s));
    thread.inc_pc(3);
    jvm.nop();
    return 0;
}
ErrorOr<int> ldc(JVM jvm, Thread thread)
{
    auto placeholder = thread.current_frame(); // Just a placeholder to prevent compiler complaints about not using thread.
    /*
    u8 index = thread.current_frame().current_method[thread.pc() + 1];
    CPEntry entry = (*thread.current_frame().rt_const_pool.as_nonnull_ptr()).cp_entry((short) index);
    switch (entry.kind()) {
       case ConstantKind::Integer:
        thread.push_operand(StackValue(entry.as_int_info()));
        break;
       case ConstantKind::Float:
        thread.push_operand(StackValue(entry.as_float_info()));
        break;
       case ConstantKind::String:
         //The spec says that we should return a reference to an instance of class string
         break;
        case ConstantKind::Class:
         short index = entry.as_class_info();
         CPEntry entry = (*thread.current_frame().rt_const_pool.m_ptr).cp_entry(index);
         VERIFY(entry.kind() == ConstantKind::Utf8);
         Utf8Info utf8 = entry.as_utf8_info();
         VERIFY(utf8.bytes[0] == 'L');
         StringView name = StringView(utf8.bytes + 1, utf8.length); //offset the beginning 'L'.
         int c = jvm.resolve_class_reference(name);
         thread.push_operand(StackValue(c, Type::Class));

         break;
     }
     */
    jvm.nop();
    return 0;
}

}

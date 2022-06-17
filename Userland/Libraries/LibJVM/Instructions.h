/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <LibJVM/JVM.h>
#include <LibJVM/Thread.h>

namespace JVM {

ErrorOr<int> nop(JVM jvm, Thread thread);
ErrorOr<int> aconst_null(JVM jvm, Thread thread);
ErrorOr<int> iconst_m1(JVM jvm, Thread thread);
ErrorOr<int> iconst_0(JVM jvm, Thread thread);
ErrorOr<int> iconst_1(JVM jvm, Thread thread);
ErrorOr<int> iconst_2(JVM jvm, Thread thread);
ErrorOr<int> iconst_3(JVM jvm, Thread thread);
ErrorOr<int> iconst_4(JVM jvm, Thread thread);
ErrorOr<int> iconst_5(JVM jvm, Thread thread);
ErrorOr<int> lconst_0(JVM jvm, Thread thread);
ErrorOr<int> lconst_1(JVM jvm, Thread thread);
ErrorOr<int> fconst_0(JVM jvm, Thread thread);
ErrorOr<int> fconst_1(JVM jvm, Thread thread);
ErrorOr<int> fconst_2(JVM jvm, Thread thread);
ErrorOr<int> dconst_0(JVM jvm, Thread thread);
ErrorOr<int> dconst_1(JVM jvm, Thread thread);
ErrorOr<int> bipush(JVM jvm, Thread thread);
ErrorOr<int> sipush(JVM jvm, Thread thread);
ErrorOr<int> ldc(JVM jvm, Thread thread);

}

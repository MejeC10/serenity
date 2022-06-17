/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/HashMap.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/RefPtr.h>
#include <AK/StringView.h>
#include <AK/Vector.h>
#include <LibJS/Heap/Heap.h>
#include <LibJVM/Forward.h>
#include <LibJVM/Thread.h>

namespace JVM {

class JVM {

public:
    JVM()
    {
        m_threads = AK::Vector<AK::NonnullOwnPtr<Thread>>();
        m_loaded_classes = AK::HashMap<StringView, Class>();
    }
    bool load_from_class_file(StringView path);
    RefPtr<Class> resolve_class_reference(StringView ref);
    // Class get_class_from_index(int index);
    void nop() { } // This exists to prevent complaints about not using variables.
    // For more explanation, see Instructions.h.
private:
    AK::Vector<AK::NonnullOwnPtr<Thread>> m_threads;
    AK::HashMap<StringView, Class> m_loaded_classes;
};

}

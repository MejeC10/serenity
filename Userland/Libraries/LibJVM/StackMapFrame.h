/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/FixedArray.h>
#include <AK/Forward.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/StdLibExtras.h>
#include <LibJVM/DataAccess.h>
#include <LibJVM/Verification.h>

namespace JVM {

enum class StackMapFrameKind {
    Same,
    SameLocals1StackItem,
    SameLocals1StackItemExtended,
    Chop,
    SameExtended,
    Append,
    Full,
};

class StackMapFrame {
public:
    StackMapFrameKind kind() const
    {
        return m_kind;
    }
    short offset_delta() const
    {
        return m_offset_delta;
    };
    u8 tag() const
    {
        return m_tag;
    }
    void copy_from(StackMapFrame const& other)
    {
        m_offset_delta = other.m_offset_delta;
        m_tag = other.m_tag;
        m_kind = other.m_kind;
        m_chopped = other.m_chopped;
        if (m_kind == StackMapFrameKind::Append) {
            auto local_result = m_locals.try_create(other.m_locals.span());
        } else if (m_kind == StackMapFrameKind::Full) {
            auto local_result = m_locals.try_create(other.m_locals.span());
            auto stack_result = m_stack.try_create(other.m_stack.span());
        }
    }
    void move_from(StackMapFrame& other)
    {
        m_offset_delta = other.m_offset_delta;
        m_tag = other.m_tag;
        m_kind = other.m_kind;
        m_chopped = other.m_chopped;
        if (m_kind == StackMapFrameKind::Append) {
            m_locals.~FixedArray();
            m_locals.swap(other.m_locals);
        } else if (m_kind == StackMapFrameKind::Full) {
            m_locals.~FixedArray();
            m_locals.swap(other.m_locals);
            m_stack.~FixedArray();
            m_stack.swap(other.m_stack);
        }
    }
    void move_from(StackMapFrame&& other)
    {
        m_offset_delta = other.m_offset_delta;
        m_tag = other.m_tag;
        m_kind = other.m_kind;
        m_chopped = other.m_chopped;
        if (m_kind == StackMapFrameKind::Append) {
            m_locals.~FixedArray();
            m_locals.swap(other.m_locals);
        } else if (m_kind == StackMapFrameKind::Full) {
            m_locals.~FixedArray();
            m_locals.swap(other.m_locals);
            m_stack.~FixedArray();
            m_stack.swap(other.m_stack);
        }
    }
    StackMapFrame(StackMapFrame const& other)
    {
        copy_from(other);
    }
    StackMapFrame() { }
    StackMapFrame(u8 const* data, size_t& loc, size_t const length)
    {
        u8 kind = get_short(data, loc, length);
        if (kind < 64) {
            m_kind = StackMapFrameKind::Same;
            m_tag = kind;
            m_offset_delta = kind;
        } else if (kind < 128) {
            m_kind = StackMapFrameKind::SameLocals1StackItem;
            m_tag = kind;
            m_offset_delta = kind - 64;
            auto local_result = m_locals.try_create(1); // We use AK::FixedArray to hold the value of only_local.
            m_locals[0] = VerificationType(data, loc, length);
        } else if (kind == 247) {
            m_kind = StackMapFrameKind::SameLocals1StackItemExtended;
            m_tag = 247;
            m_offset_delta = get_short(data, loc, length);
            auto local_result = m_locals.try_create(1); // We use AK::FixedArray to hold the value of only_local.
            m_locals[0] = VerificationType(data, loc, length);
        } else if (kind > 247 && kind < 251) {
            m_kind = StackMapFrameKind::Chop;
            m_tag = kind;
            m_offset_delta = get_short(data, loc, length);
            m_chopped = 251 - kind;
        } else if (kind == 251) {
            m_kind = StackMapFrameKind::SameExtended;
            m_tag = 251;
            m_offset_delta = get_short(data, loc, length);
        } else if (kind < 255) {
            m_kind = StackMapFrameKind::Append;
            m_tag = kind;
            m_offset_delta = get_short(data, loc, length);
            AK::Span<VerificationType> locals_span = AK::Span<VerificationType>();
            locals_span.overwrite(loc, data, (kind - 251) * sizeof(VerificationType));
            auto locals_result = m_locals.try_create(locals_span);
        } else if (kind == 255) {
            m_tag = kind;
            m_offset_delta = get_short(data, loc, length);
            unsigned short num_locals = get_short(data, loc, length);
            AK::Span<VerificationType> locals_span = AK::Span<VerificationType>();
            locals_span.overwrite(loc, data, num_locals * sizeof(VerificationType));
            auto locals_result = m_locals.try_create(locals_span);
            unsigned short num_stack_items = get_short(data, loc, length);
            AK::Span<VerificationType> stack_span = AK::Span<VerificationType>();
            stack_span.overwrite(loc, data, num_stack_items * sizeof(VerificationType));
            auto stack_result = m_stack.try_create(stack_span);
        }
    }
    // I don't really understand how to construct or use ErrorOr's, but these should be wrapped in them to deal with invalid calls.
    u8 chopped()
    {
        if (m_kind != StackMapFrameKind::Chop)
            VERIFY_NOT_REACHED();
        return m_chopped;
    }
    ErrorOr<FixedArray<VerificationType>> additional_locals()
    {
        if (m_kind != StackMapFrameKind::Append)
            VERIFY_NOT_REACHED();
        return m_locals.try_clone();
    }
    ErrorOr<FixedArray<VerificationType>> locals()
    {
        if (m_kind != StackMapFrameKind::Full)
            VERIFY_NOT_REACHED();
        return m_locals.try_clone();
    }
    ErrorOr<FixedArray<VerificationType>> stack()
    {
        if (m_kind != StackMapFrameKind::Full)
            VERIFY_NOT_REACHED();
        return m_stack.try_clone();
    }

private:
    short m_offset_delta;
    u8 m_tag;
    u8 m_chopped;
    AK::FixedArray<VerificationType> m_locals;
    AK::FixedArray<VerificationType> m_stack;
    StackMapFrameKind m_kind;
};

struct StackMapTable {
    AK::FixedArray<StackMapFrame*> frames; // The pointers here should be NonnullOwnPtrs, but they can't copy and so it has to be a raw ptr.
    StackMapTable()
    {
    }
    ~StackMapTable()
    {
        frames.~FixedArray();
    }
    void copy_from(StackMapTable const& other)
    {
        auto _ = frames.try_create(other.frames.span());
    }
    void move_from(StackMapTable& other)
    {
        frames.~FixedArray();
        frames.swap(other.frames);
    }
    void move_from(StackMapTable&& other)
    {
        frames.~FixedArray();
        frames.swap(other.frames);
    }
    StackMapTable(StackMapTable const& other)
    {
        copy_from(other);
    }
    StackMapTable& operator=(StackMapTable const& other)
    {
        if (this != &other) {
            copy_from(other);
        }
        return *this;
    }
};

}

/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Assertions.h>
#include <AK/Error.h>
#include <LibJVM/DataAccess.h>

namespace JVM {

enum class VerificationKind {
    Top,
    Integer,
    Float,
    Double,
    Long,
    Null,
    UninitializedThis,
    Object,
    UninitializedVariable,

};

class VerificationType {
public:
    VerificationType(VerificationKind kind, short val)
        : m_kind(kind)
    {
        if (kind == VerificationKind::Object) {
            m_value.cpool_index = val;
        } else if (kind == VerificationKind::UninitializedVariable) {
            m_value.offset = val;
        } else {
            VERIFY_NOT_REACHED();
        }
    }
    VerificationType(u8 const* data, size_t& loc, size_t length)
    {
        VerificationKind kind = VerificationKind(get_byte(data, loc, length));
        m_kind = kind;
        if (kind == VerificationKind::Object) {
            m_value.cpool_index = get_short(data, loc, length);
        } else if (kind == VerificationKind::UninitializedVariable) {
            m_value.offset = get_short(data, loc, length);
        }
    }
    VerificationType(VerificationKind kind)
        : m_kind(kind)
    {
    }
    VerificationType() { }
    VerificationKind kind()
    {
        return m_kind;
    }
    short value() const
    {
        if (m_kind == VerificationKind::Object) {
            return m_value.cpool_index;
        } else if (m_kind == VerificationKind::UninitializedVariable) {
            return m_value.offset;
        } else {
            VERIFY_NOT_REACHED();
        }
    }

private:
    VerificationKind m_kind;
    union {
        short cpool_index;
        short offset;
    } m_value;
};

}

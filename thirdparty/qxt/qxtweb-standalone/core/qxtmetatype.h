
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

#ifndef QXTMETATYPE_H
#define QXTMETATYPE_H

#include <QMetaType>
#include <QDataStream>
#include <QGenericArgument>
#include <QtDebug>
#include <qxtglobal.h>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#define qxtcreate create
#else
#define qxtcreate construct
#endif

template <typename T>
class /*QXT_CORE_EXPORT*/ QxtMetaType
{
public:
    static inline T* construct(const T* copy = 0)
    {
        return QMetaType::qxtcreate(qMetaTypeId<T>(), reinterpret_cast<const void*>(copy));
    }

    static inline void destroy(T* data)
    {
        QMetaType::destroy(qMetaTypeId<T>(), data);
    }

    // no need to reimplement isRegistered since this class will fail at compile time if it isn't

    static inline bool load(QDataStream& stream, T* data)
    {
        return QMetaType::load(stream, qMetaTypeId<T>(), reinterpret_cast<void*>(data));
    }

    static inline bool save(QDataStream& stream, const T* data)
    {
        return QMetaType::save(stream, qMetaTypeId<T>(), reinterpret_cast<const void*>(data));
    }

    static inline int type()
    {
        return qMetaTypeId<T>();
    }

    static inline const char* name()
    {
        return QMetaType::typeName(qMetaTypeId<T>());
    }
};

template <>
class /*QXT_CORE_EXPORT*/ QxtMetaType<void>
{
public:
    static inline void* construct(const void* copy = 0)
    {
        Q_UNUSED(copy);
        return 0;
    }

    static inline void destroy(void* data)
    {
        Q_UNUSED(data);
    }

    static inline bool load(QDataStream& stream, void* data)
    {
        Q_UNUSED(stream);
        Q_UNUSED(data);
        return false;
    }

    static inline bool save(QDataStream& stream, const void* data)
    {
        Q_UNUSED(stream);
        Q_UNUSED(data);
        return false;
    }

    static inline int type()
    {
        return 0;
    }

    static inline const char* name()
    {
        return 0;
    }
};

inline void* qxtConstructByName(const char* typeName, const void* copy = 0)
{
    return QMetaType::qxtcreate(QMetaType::type(typeName), copy);
}

inline void qxtDestroyByName(const char* typeName, void* data)
{
    QMetaType::destroy(QMetaType::type(typeName), data);
}

inline void* qxtConstructFromGenericArgument(QGenericArgument arg)
{
    return qxtConstructByName(arg.name(), arg.data());
}

inline void qxtDestroyFromGenericArgument(QGenericArgument arg)
{
    qxtDestroyByName(arg.name(), arg.data());
}

#endif

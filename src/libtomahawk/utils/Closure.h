/* This file is part of Tomabawk.
   Copyright 2011, David Sansome <me@davidsansome.com>
   Copyright 2012, Leo Franchi <lfranchi@kde.org>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CLOSURE_H
#define CLOSURE_H

#include "DllMacro.h"

#include <tr1/functional>

#include <QMetaMethod>
#include <QObject>
#include <QSharedPointer>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace _detail {

class DLLEXPORT ClosureArgumentWrapper {
 public:
  virtual ~ClosureArgumentWrapper() {}

  virtual QGenericArgument arg() const = 0;
};

template<typename T>
class ClosureArgument : public ClosureArgumentWrapper {
 public:
  explicit ClosureArgument(const T& data) : data_(data) {}

  virtual QGenericArgument arg() const {
      return QArgument<T>(QMetaType::typeName(qMetaTypeId<T>()), data_);
  }

 private:
  T data_;
};

class DLLEXPORT Closure : public QObject, boost::noncopyable {
  Q_OBJECT

 public:
  Closure(QObject* sender, const char* signal,
          QObject* receiver, const char* slot,
          const ClosureArgumentWrapper* val0 = 0,
          const ClosureArgumentWrapper* val1 = 0,
          const ClosureArgumentWrapper* val2 = 0,
          const ClosureArgumentWrapper* val3 = 0);

  Closure(QObject* sender, const char* signal,
          std::tr1::function<void()> callback);

  void setAutoDelete( bool autoDelete ) { autoDelete_ = autoDelete; }

  virtual ~Closure();

  /**
   * If you don't this Closure to act on a signal, but just act like
   *  a closure in that it saves some args and delivers them on demand later
   *
   * Only call this if you passed a null QObject* as a sender! Otherwise you
   *  might delete your object twice :)
   */
  void forceInvoke();

 private slots:
  void Invoked();
  void Cleanup();

 private:
  void Connect(QObject* sender, const char* signal);

  QMetaMethod slot_;
  std::tr1::function<void()> callback_;
  bool autoDelete_;
  QObject* outOfThreadReceiver_;

  boost::scoped_ptr<const ClosureArgumentWrapper> val0_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val1_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val2_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val3_;
};

class DLLEXPORT SharedPointerWrapper {
 public:
  virtual ~SharedPointerWrapper() {}
  virtual QObject* data() const = 0;
};

template<typename T>
class SharedPointer : public SharedPointerWrapper {
 public:
  explicit SharedPointer(QSharedPointer<T> ptr)
      : ptr_(ptr) {
  }

  QObject* data() const {
    return ptr_.data();
  }

 private:
  QSharedPointer<T> ptr_;
};

// For use with a QSharedPointer as a sender.
class SharedClosure : public Closure {
  Q_OBJECT

 public:
  SharedClosure(SharedPointerWrapper* sender, const char* signal,
                QObject* receiver, const char* slot,
                const ClosureArgumentWrapper* val0 = 0,
                const ClosureArgumentWrapper* val1 = 0,
                const ClosureArgumentWrapper* val2 = 0,
                const ClosureArgumentWrapper* val3 = 0)
      : Closure(sender->data(), signal,
                receiver, slot,
                val0, val1, val2, val3),
        shared_sender_(sender) {
  }

 private:
  boost::scoped_ptr<SharedPointerWrapper> shared_sender_;
};

}  // namespace _detail

#define C_ARG(type, data) new _detail::ClosureArgument<type>(data)

_detail::Closure* NewClosure(
    QObject* sender,
    const char* signal,
    QObject* receiver,
    const char* slot);

template <typename T>
_detail::Closure* NewClosure(
    QObject* sender,
    const char* signal,
    QObject* receiver,
    const char* slot,
    const T& val0) {
  return new _detail::Closure(
      sender, signal, receiver, slot,
      C_ARG(T, val0));
}

template <typename T0, typename T1>
_detail::Closure* NewClosure(
    QObject* sender,
    const char* signal,
    QObject* receiver,
    const char* slot,
    const T0& val0,
    const T1& val1) {
  return new _detail::Closure(
      sender, signal, receiver, slot,
      C_ARG(T0, val0), C_ARG(T1, val1));
}

template <typename T0, typename T1, typename T2>
_detail::Closure* NewClosure(
    QObject* sender,
    const char* signal,
    QObject* receiver,
    const char* slot,
    const T0& val0,
    const T1& val1,
    const T2& val2) {
  return new _detail::Closure(
      sender, signal, receiver, slot,
      C_ARG(T0, val0), C_ARG(T1, val1), C_ARG(T2, val2));
}

template <typename T0, typename T1, typename T2, typename T3>
_detail::Closure* NewClosure(
    QObject* sender,
    const char* signal,
    QObject* receiver,
    const char* slot,
    const T0& val0,
    const T1& val1,
    const T2& val2,
    const T3& val3) {
  return new _detail::Closure(
      sender, signal, receiver, slot,
      C_ARG(T0, val0), C_ARG(T1, val1), C_ARG(T2, val2), C_ARG(T3, val3));
}

template <typename TP>
_detail::Closure* NewClosure(
    QSharedPointer<TP> sender,
    const char* signal,
    QObject* receiver,
    const char* slot) {
  return new _detail::SharedClosure(
      new _detail::SharedPointer<TP>(sender), signal, receiver, slot);
}

template <typename TP, typename T0>
_detail::Closure* NewClosure(
    QSharedPointer<TP> sender,
    const char* signal,
    QObject* receiver,
    const char* slot,
    const T0& val0) {
  return new _detail::SharedClosure(
        new _detail::SharedPointer<TP>(sender), signal, receiver, slot,
        C_ARG(T0, val0));
}

template <typename TP, typename T0, typename T1>
_detail::Closure* NewClosure(
    QSharedPointer<TP> sender,
    const char* signal,
    QObject* receiver,
    const char* slot,
    const T0& val0,
    const T1& val1) {
  return new _detail::SharedClosure(
        new _detail::SharedPointer<TP>(sender), signal, receiver, slot,
        C_ARG(T0, val0), C_ARG(T1, val1));
}

#endif  // CLOSURE_H

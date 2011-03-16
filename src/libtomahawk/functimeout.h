#ifndef FUNCTIMEOUT_H
#define FUNCTIMEOUT_H

#include <QObject>
#include <QTimer>
#include <QDebug>

#include "boost/function.hpp"
#include "boost/bind.hpp"

#include "dllmacro.h"

/*
    I want to do:
        QTimer::singleShot(1000, this, SLOT(doSomething(x)));
    instead, I'm doing:
        new FuncTimeout(1000, boost::bind(&MyClass::doSomething, this, x));

 */
namespace Tomahawk
{

class DLLEXPORT FuncTimeout : public QObject
{
Q_OBJECT

public:
    FuncTimeout( int ms, boost::function<void()> func )
        : m_func( func )
    {
        //qDebug() << Q_FUNC_INFO;
        QTimer::singleShot( ms, this, SLOT(exec() ) );
    };

    ~FuncTimeout()
    {
        //qDebug() << Q_FUNC_INFO;
    };

public slots:
    void exec()
    {
        m_func();
        this->deleteLater();
    };

private:
    boost::function<void()> m_func;
};

}; // ns

#endif // FUNCTIMEOUT_H

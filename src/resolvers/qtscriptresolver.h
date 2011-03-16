#ifndef QTSCRIPTRESOLVER_H
#define QTSCRIPTRESOLVER_H

#include "resolver.h"
#include "query.h"
#include "result.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>

class ScriptEngine : public QWebPage
{
Q_OBJECT

public:
    explicit ScriptEngine( QObject* parent )
        : QWebPage( parent )
    {}

public slots:
    bool shouldInterruptJavaScript()
    {
        QApplication::processEvents( QEventLoop::AllEvents, 42 );
        return false;
    }

protected:
    virtual void javaScriptConsoleMessage( const QString & message, int lineNumber, const QString & sourceID )
    { qDebug() << "JAVASCRIPT ERROR:" << message << lineNumber << sourceID; }
};

class QtScriptResolver : public Tomahawk::ExternalResolver
{
Q_OBJECT

public:
    explicit QtScriptResolver( const QString& scriptPath );
    virtual ~QtScriptResolver();

    virtual QString name() const            { return m_name; }
    virtual unsigned int weight() const     { return m_weight; }
    virtual unsigned int preference() const { return m_preference; }
    virtual unsigned int timeout() const    { return m_timeout; }

    virtual void resolve( const Tomahawk::query_ptr& query );

public slots:
    virtual void stop();

private slots:

private:
    ScriptEngine* m_engine;

    QString m_name;
    unsigned int m_weight, m_preference, m_timeout;

    bool m_ready, m_stopped;
};

#endif // QTSCRIPTRESOLVER_H

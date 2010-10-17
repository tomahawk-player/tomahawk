#ifndef MYSERVICE
#define MYSERVICE

#include <QxtHttpServerConnector>
#include <QxtHttpSessionManager>
#include <QxtWebSlotService>
#include <QxtWebPageEvent>


class MyService : public QxtWebSlotService{
    Q_OBJECT;
public:
    MyService(QxtAbstractWebSessionManager * sm, QObject * parent = 0 ): QxtWebSlotService(sm,parent){
    }
public slots:
    void index(QxtWebRequestEvent* event)
    {
        postEvent(new QxtWebPageEvent(event->sessionID, event->requestID, "<h1>It Works!</h1>"));
    }
};

#endif


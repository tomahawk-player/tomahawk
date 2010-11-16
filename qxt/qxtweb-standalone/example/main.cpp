#include <QCoreApplication>

#include "myservice.h"


int main(int argc, char ** argv){

        QCoreApplication app(argc,argv);

        QxtHttpServerConnector connector;

        QxtHttpSessionManager session;
        session.setPort(8080);
        session.setConnector(&connector);

        MyService s1(&session);
        session.setStaticContentService ( &s1);

        session.start();
        return app.exec();
}


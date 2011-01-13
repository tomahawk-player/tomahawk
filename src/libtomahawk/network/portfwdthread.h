#ifndef PORTFWDTHREAD_H
#define PORTFWDTHREAD_H

#include <QThread>
#include <QMutex>
#include <QHostAddress>

class Portfwd;

class PortFwdThread : public QThread
{
Q_OBJECT

public:
    explicit PortFwdThread( unsigned int port );
    ~PortFwdThread();

signals:
    void externalAddressDetected( QHostAddress ha, unsigned int port );

private slots:
    void work();

private:
    void run();

    Portfwd* m_portfwd;
    QHostAddress m_externalAddress;
    unsigned int m_externalPort, m_port;
};

#endif // PORTFWDTHREAD_H

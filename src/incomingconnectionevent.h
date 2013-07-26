#ifndef INCOMINGCONNECTIONEVENT_H
#define INCOMINGCONNECTIONEVENT_H

#include <QEvent>

class IncomingConnectionEvent : public QEvent
{
public:
    IncomingConnectionEvent(int descriptor);

    static int IncomingConnectionEventType;

    int descriptor() const;

private:
    int fd;
};

#endif // INCOMINGCONNECTIONEVENT_H

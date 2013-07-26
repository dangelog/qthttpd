#include "incomingconnectionevent.h"

int IncomingConnectionEvent::IncomingConnectionEventType = 0;

IncomingConnectionEvent::IncomingConnectionEvent(int descriptor)
    : QEvent(static_cast<QEvent::Type>(IncomingConnectionEventType)), fd(descriptor)
{
}

int IncomingConnectionEvent::descriptor() const
{
    return fd;
}

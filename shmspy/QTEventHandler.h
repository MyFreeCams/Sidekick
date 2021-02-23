
#pragma once

#include <mfc_ipc.h>

#include <QMessageBox>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>



class CQtEventHandler :  public QObject, public MFCIPC::CSimpleEventHandler
{
Q_OBJECT
public:
    CQtEventHandler(const char *pID)
            : CSimpleEventHandler(pID)
    {
    }

    ~CQtEventHandler()
    {
    }


    virtual void postIncomingEvent(MFCIPC::CIPCEvent &evt) override
    {
        incomingEvent(evt);
    }

    virtual void postIncomingEvent(MFCIPC::CIPCEventList &que) override
    {
        incomingEvents(que);
        for (MFCIPC::CIPCEvent e : que)
        {
            incomingEvent(e);
        }
    }

    virtual void postRemoveEvent(MFCIPC::CIPCEventList &el) override
    {
        removeEvents(el);
        for (auto e : el)
        {
            removeEvent(e);
        }
    }

    virtual void postRemoveEvent(MFCIPC::CIPCEvent &evt) override
    {
        removeEvent(evt);
    }


public slots:

    void incomingEvent(MFCIPC::CIPCEvent evt)
    {
        emit onIncomingEvent(evt);
    }

    void incomingEvents(MFCIPC::CIPCEventList evt)
    {
        emit onIncomingEvents(evt);
    }

    void removeEvent(MFCIPC::CIPCEvent evt)
    {
        emit onRemoveEvent(evt);
    }

    void removeEvents(MFCIPC::CIPCEventList el)
    {
        emit onRemoveEvents(el);
    }

signals:

    void onIncomingEvent(MFCIPC::CIPCEvent);

    void onIncomingEvents(MFCIPC::CIPCEventList);

    void onRemoveEvent(MFCIPC::CIPCEvent);

    void onRemoveEvents(MFCIPC::CIPCEventList);

protected:
private:
};

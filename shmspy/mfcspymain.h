#ifndef MFCSPYMAIN_H
#define MFCSPYMAIN_H

#include "IPCUtil.h"
#include "ShmemDefines.h"
#include "ShmemVector.h"
#include "ShmemSimpleType.h"
#include "ShmemContainer.h"
#include "ShmemSignal.h"


#include "EvtDefines.h"
#include "EventList.h"
#include "QTEventHandler.h"
#include "IPCEventHandler.h"
#include "IPCEvent.h"

#include <QMainWindow>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>



QT_BEGIN_NAMESPACE
namespace Ui { class MFCSpyMain; }
QT_END_NAMESPACE
class MFCSpyMain;
class QTimer;



class MFCSpyMain : public QMainWindow
{
    Q_OBJECT

public:
    MFCSpyMain(QWidget *parent = nullptr);
    ~MFCSpyMain();

    void updateGrids();
    void updateEventGrid();
    void updateEventGrid(MFCIPC::CIPCEventList &);

    void updateProcessGrid();


private slots:
    void handleDummyPlaceHolder();
    void handleClearClients();
    void handleConnectButton();
    void handleShmemReset();
    void handlesendEvent();
    void handleUpdateButton();
    void handleTriggerRouterMutexButton();
    void handleTriggerMaintenanceButton();
    void handleShowOnlyInUseEvents();
    void handleShowOnlyInUseProcesses();


    //
    // event based slots.
    //
    void incomingEvent(MFCIPC::CIPCEvent);
    void incomingEvents(MFCIPC::CIPCEventList );

    void removeEvent(MFCIPC::CIPCEvent);
    void removeEvents(MFCIPC::CIPCEventList );

    bool isStarted() { return m_bIsStarted; }
    void isStarted(bool b) { m_bIsStarted = b; }

private:

    CQtEventHandler &getEventHandler() { return m_myEH; }
private:
    Ui::MFCSpyMain *ui;
    CQtEventHandler m_myEH;
    bool m_bIsStarted = false;
};


#endif // MFCSPYMAIN_H

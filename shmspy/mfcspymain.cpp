#include "../libfcs/Log.h"
#include "fcslib_string.h"

#include "mfc_ipc.h"
#include "IPCUtil.h"
#include "ShmemDefines.h"
#include "ShmemVector.h"
#include "ShmemSimpleType.h"
#include "ShmemContainer.h"

#include "mfcspymain.h"
#include "Router.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include "./ui_mfcspymain.h"

#include <QMessageBox>


#include <nlohmann/json.hpp>
using njson = nlohmann::json;

boost::mutex g_mutexUpdate;

#include "mfcspymain.moc"

//---------------------------------------------------------------------
//
//
//
MFCSpyMain::MFCSpyMain(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MFCSpyMain)
    , m_myEH("MFCSpy")
{
    ui->setupUi(this);

    ui->btnOnlyShowEventInUse->setCheckState(Qt::CheckState::Checked);
    ui->btnOnlyShowClientInUse->setCheckState(Qt::CheckState::Checked);

    QTableWidget *ptwClients = ui->twClients;
    ptwClients->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ptwClients->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    ptwClients->setSelectionBehavior(QAbstractItemView::SelectRows);
    ptwClients->setSelectionMode(QAbstractItemView::SingleSelection);
    ptwClients->setShowGrid(true);
    ptwClients->setStyleSheet("QTableView {selection-background-color: blue;}");
    ptwClients->setColumnCount(8);

    //-------------------------------------------------------------------
    // add columns to the process table.
    QStringList sClientHeaders;
    sClientHeaders << "ID" << "Connected" << "Last Checkin" << "Last Update" << "Last Maintenance" << "Active" << "Mutex Name" << "Maintenance Bid";
    ptwClients->setHorizontalHeaderLabels(sClientHeaders);
    ptwClients->setColumnWidth(0,100);
    ptwClients->setColumnWidth(1,160);
    ptwClients->setColumnWidth(2,160);
    ptwClients->setColumnWidth(3,160);
    ptwClients->setColumnWidth(4,160);
    ptwClients->setColumnWidth(5,75);
    ptwClients->setColumnWidth(6,130);
    ptwClients->setColumnWidth(7,120);
    QTableWidget *ptwMsg = ui->twMessages;
    ptwMsg->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ptwMsg->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    ptwMsg->setSelectionBehavior(QAbstractItemView::SelectRows);
    ptwMsg->setSelectionMode(QAbstractItemView::SingleSelection);
    ptwMsg->setShowGrid(true);
    ptwMsg->setStyleSheet("QTableView {selection-background-color: blue;}");
    ptwMsg->setColumnCount(10);

    //-----------------------------------------------------------------
    // add columns to the event table. 
    QStringList sMsgHeaders;
    sMsgHeaders << "Topic" << "To" << "From" << "Type" << "In Use" << "isRead" << "Created" << "Expires" << "Read By" << "Payload"  ;
    ptwMsg->setHorizontalHeaderLabels(sMsgHeaders);
    int n = 0;
    ptwMsg->setColumnWidth(n++,100);
    ptwMsg->setColumnWidth(n++,75);
    ptwMsg->setColumnWidth(n++,75);
    ptwMsg->setColumnWidth(n++,50);
    ptwMsg->setColumnWidth(n++,50);
    ptwMsg->setColumnWidth(n++,50);
    ptwMsg->setColumnWidth(n++,150);
    ptwMsg->setColumnWidth(n++, 150);
    ptwMsg->setColumnWidth(n++, 150);
    ptwMsg->setColumnWidth(n++, 250);
    
    //-----------------------------------------------------------------
    // wire up the various event handlers
    //
    connect(ui->btnConnect,SIGNAL(clicked()),this,SLOT(handleConnectButton()));
    connect(ui->btnReset,SIGNAL(clicked()),this,SLOT(handleShmemReset()));
    connect(ui->btnUpdate,SIGNAL(clicked()), this,SLOT(handleUpdateButton()));
    connect(ui->btnRouterMutex,SIGNAL(clicked()), this,SLOT(handleTriggerRouterMutexButton()));
    connect(ui->btnMaintance,SIGNAL(clicked()), this,SLOT(handleTriggerMaintenanceButton()));
    connect(ui->btnOnlyShowEventInUse,SIGNAL(clicked()), this,SLOT(handleShowOnlyInUseEvents()));
    connect(ui->btnOnlyShowClientInUse,SIGNAL(clicked()), this,SLOT(handleShowOnlyInUseProcesses()));

    //-----------------------------------------------------------------
    // we have to register our types to use them in a signal/slot
    //
    //https://doc.qt.io/qt-5/qmetatype.html
    qRegisterMetaType<MFCIPC::CIPCEvent>("MFCIPC::CIPCEvent");
    qRegisterMetaType<MFCIPC::CIPCEventList>("MFCIPC::CIPCEventList");


    // ----------------------------------------------------------------
    // Add event slots.
    //
    // this ensures that the event call backs are occuring in the
    // qt main (gui) thread.
    //
    CQtEventHandler &eh = getEventHandler();

    //
    //
    // this works but I think using the qt connect syntax is more clear!
    //
    // std::function<void(MFCIPC::CIPCEventList)> f = std::bind(&MFCSpyMain::incomingEvents,this, std::placeholders::_1);
    // eh.setIncomingEventListFunc(this,f);
    //
    // and lambda of course!
    //
    // eh.setIncomingEventListFunc(this,[](MFCIPC::CIPCEventList lst) {
    //    _TRACE("I'm back!");
    //});

    // conect the incoming events signal to our gui slot.
    connect(&m_myEH,&CQtEventHandler::onIncomingEvents,this,&MFCSpyMain::incomingEvents);
    connect(&m_myEH,&CQtEventHandler::onIncomingEvent,this,&MFCSpyMain::incomingEvent);

    connect(&m_myEH,&CQtEventHandler::onRemoveEvent,this,&MFCSpyMain::removeEvent);
    connect(&m_myEH,&CQtEventHandler::onRemoveEvents,this,&MFCSpyMain::removeEvents);

    //-----------------------------------------------------------------
    // add subscriptions to the event handler
    //
    // we want all event

    // get system events.  (this is redundent)
    eh.getSubscriptions().addEventType(EVT_SYSTEM);


    // we want all events even ones not sent to this handler
    //
    // using regex on to addres to return all matches.
    eh.getSubscriptions().addTo("/:.*");

}

MFCSpyMain::~MFCSpyMain()
{
    delete ui;
}


//---------------------------------------------------------------------
// incomingEvent
//
// this is the slot for incoming events.  This is running in the
// QT main thread!
//
// since we are in the qt gui thread, we can update the gui as needed.
void MFCSpyMain::incomingEvent(MFCIPC::CIPCEvent evt)
{
    CQtEventHandler &eh = getEventHandler();

    switch (evt.getType())
    {
        case EVT_MAINTENANCE_END:
        {
            std::string s = evt.getPayload();
           /* njson jsMaint = njson::parse(evt.getPayload());

            if (jsMaint.contains("process"))
            {
                int n = jsMaint["process"].get<int>();
                std::string s = stdprintf("%d", n);
                ui->txtMaintPID->setText(s.c_str());
            }

            if (jsMaint.contains("recordsProcessed"))
            {
                std::string s = stdprintf("%d", jsMaint["recordsProcessed"].get<int>());
                ui->txtEvtsProcessed->setText(s.c_str());

            }

            if (jsMaint.contains("time"))
            {
                std::string s = jsMaint.value("time", std::string(""));
                ui->txtLastMaint->setText(s.c_str());
            }
*/
            break;
        }

        case EVT_MAINTENANCE_STATUS:
        {
          MFCIPC::CRouter *pRTR = MFCIPC::CRouter::getInstance();

            std::string s = pRTR->getShmemBlockVersion();
            ui->txtSHMVersion->setText(s.c_str());


            njson js = njson::parse(evt.getPayload());
            if (js.contains("size"))
            {
                std::string s = stdprintf("%d", js["size"].get<int>());
                ui->txtShmemSize->setText(s.c_str());

            }

            if (js.contains("free"))
            {
                std::string s = stdprintf("%d", js["free"].get<int>());
                ui->txtShmemFree->setText(s.c_str());
            }

            if (js.contains("EventSize"))
            {
                std::string s = stdprintf("%d", js["EventSize"].get<int>());
                ui->txtEvtCnt->setText(s.c_str());
            }

            break;
        }
    } // end switch

    evt.isRead(true);
    evt.markAsRead(eh.getID().c_str());
    evt.update(false);

}

//---------------------------------------------------------------------
// incomingEvents
//
// we have subscribed to all events, so clear the event list grid
// and read all the items.
//
// since we are in the qt gui thread, we can update the gui as needed.
void MFCSpyMain::incomingEvents(MFCIPC::CIPCEventList evt)
{

    if (ui->btnOnlyShowEventInUse->isChecked())
    {
        // we are only showing records in use, so the
        // the passed event list will contain our entire subscription.
        updateEventGrid(evt);
    }
    else
    {
        // we want to see all records from teh shared memory
        // vector
      MFCIPC::CRouter *pR = MFCIPC::CRouter::getInstance();
        MFCIPC::CIPCEventList el;
        pR->getAllEvents(el);
        updateEventGrid(el);
    }
}

//---------------------------------------------------------------------
// removeEvent
//
// slot for event removed.
//
// // since we are in the qt gui thread, we can update the gui as needed.
void MFCSpyMain::removeEvent(MFCIPC::CIPCEvent e)
{
    _TRACE("Event %d marked as not in use",e.getKey());

}

//--------------------------------------------------------------------
// removeEvents
//
// slot for list of events removed.
void MFCSpyMain::removeEvents(MFCIPC::CIPCEventList )
{
    updateGrids();
}

//---------------------------------------------------------------------
// updateEventGrid
//
// load the event list into the event grid.
//
// this call back is generated by libIPC
void MFCSpyMain::updateEventGrid(MFCIPC::CIPCEventList &el)
{
    QTableWidget *ptwMsg = ui->twMessages;
    ptwMsg->clearContents();
    ptwMsg->setRowCount(static_cast<int>(el.size()));
    int nRow = 0;
    for(auto e : el)
    {
        for(int nCol = 0; nCol < 10;nCol++)
        {
            std::string s = "";
            switch(nCol)
            {
                case 0:
                    s = e.getTopic();
                    break;
                case 1:
                    s = e.getTo();
                    break;
                case 2:
                    s = e.getFrom();
                    break;
                case 3:
                    s = MFCIPC::getMessageTypeName(e.getType());
                    break;
                case 4:
                    s = e.isInUse() ? "Yes" : "No";
                    break;
                case 5:
                    s = e.isRead() ? "Yes" : "No";
                    break;
                case 6:
                {
                    time_t t = boost::posix_time::to_time_t(e.getCreateDate());
                    char buffer[32];
                    // Format: Mo, 15.06.2009 20:20:00
                    std::tm * ptm = std::localtime(&t);
                    std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);
                    s = buffer; //boost::posix_time::to_simple_string(sc.getCreateDate());
                }
                    break;
                case 7:
                {
                    char buffer[32];
                    time_t tExp = boost::posix_time::to_time_t(e.getExpirationDate());
                    std::tm * ptmExp = std::localtime(&tExp);
                    std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", ptmExp);
                    std::string sTExt = buffer;
                }
                    break;
                case 8:
                    s = e.getReadBy();
                    break;
                case 9:
                    s = e.getPayload();
                    break;
            }

            QTableWidgetItem *pItem = new QTableWidgetItem(s.c_str());
            QSize sz = pItem->sizeHint();
            pItem->setSizeHint(sz);
            ptwMsg->setItem(nRow, nCol, pItem);

        }
        nRow++;

    }
}

//---------------------------------------------------------------------
// updateMessageGrid
//
//
// This will udpate the event grid by going directly to the router.
void MFCSpyMain::updateEventGrid()
{
    if (isStarted())
    {
      MFCIPC::CRouter *pR = MFCIPC::CRouter::getInstance();
        MFCIPC::CIPCEventList el;
        if (ui->btnOnlyShowEventInUse->isChecked())
        {
            // get our subscritpions records.
            getEventHandler().getSubscribedEvents(el);
        }
        else
        {
            // we want them all.
            pR->getAllEvents(el);
        }

        updateEventGrid(el);
    }
    else
    {
        QMessageBox::information(this, "Warning", "Connect to the router first");
    }
}

//---------------------------------------------------------------------
// updateProcessGrid
//
// the process records are stored in shared mem, but they are not
// events and there is no notification of updates.
void MFCSpyMain::updateProcessGrid()
{
    if (isStarted())
    {
      MFCIPC::CRouter *pRTR = MFCIPC::CRouter::getInstance();

        StringListList rows;
        pRTR->getProcessesAsString(!ui->btnOnlyShowClientInUse->isChecked(), rows);

        QTableWidget *ptwClients = ui->twClients;
        ptwClients->clearContents();
        ptwClients->setRowCount(static_cast<int>(rows.size()));

        int nRow = 0;
        for (StringListList::iterator iRow = rows.begin(); iRow != rows.end(); iRow++, nRow++)
        {
            StringList &cols = *iRow;
            int nCol = 0;
            for (StringList::iterator iCol = cols.begin(); iCol != cols.end(); iCol++)
            {
                std::string s = *iCol;
                QTableWidgetItem *pItem = new QTableWidgetItem(s.c_str());
                QSize sz = pItem->sizeHint();
                sz.setWidth(250);
                pItem->setSizeHint(sz);
                ptwClients->setItem(nRow, nCol++, pItem);

            }
        }
    }
    else
    {
        QMessageBox::information(this, "Warning", "Connect to the router first");
    }
}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::updateGrids()
{
    if (isStarted())
    {
        updateProcessGrid();

        updateEventGrid();
    }
    else
    {
        QMessageBox::information(this, "Warning", "Connect to the router first");
    }

}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handleDummyPlaceHolder()
{
    QMessageBox::information(this, "Not Implemented", "dummy place holder for buttons");
    
}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handleShowOnlyInUseEvents()
{
    if (isStarted())
    {
        updateGrids();
    }
    else
    {
        QMessageBox::information(this, "toggle show in use", "Connect to the router first");
    }
}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handleShowOnlyInUseProcesses()
{
    if (isStarted())
    {
        updateGrids();
    }
    else
    {
        QMessageBox::information(this, "toggle show in use", "Connect to the router first");
    }
}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handleUpdateButton()
{
    if (isStarted())
        updateGrids();
    else
    {
        QMessageBox::information(this, "Update", "Connect to the router first");
    }
}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handleClearClients()
{
    
}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handleConnectButton()
{
    if (! isStarted())
    {
        isStarted(true);
      MFCIPC::CRouter::setRouterID("MFCSpy");
      MFCIPC::CRouter *pR = MFCIPC::CRouter::getInstance();
        pR->start(MFCSPY_MAINTENANCE_BID); // defined in cmakelists.txt
        updateGrids();
    }
    else
    {
           QMessageBox::information(this, "Connect Router", "Already connected");
    }
}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handleShmemReset()
{
    if (! isStarted())
    {
        if (MFCIPC::CRouter::reset())
            QMessageBox::information(this, "Reset Shared Memory", "reset shared memory succeeded");
        else
            QMessageBox::information(this, "Reset Shared Memory", "reset shared memory FAILED!");
    }
    else
    {
         QMessageBox::information(this, "Reset Shared Memory", "Can't reset shared memory once the router is connected");
    }
}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handlesendEvent()
{
}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handleTriggerRouterMutexButton()
{
    if (isStarted())
    {
      MFCIPC::CRouter *pR = MFCIPC::CRouter::getInstance();
        if (pR)
            pR->TriggerRouterMutex();
    }
    else
    {
        QMessageBox::information(this, "Trigger Router", "Connect to the router first");
    }

}

//---------------------------------------------------------------------
//
//
//
void MFCSpyMain::handleTriggerMaintenanceButton()
{
    if (isStarted())
    {
      MFCIPC::CRouter *pR = MFCIPC::CRouter::getInstance();

        if (pR)
        {
            _TRACE("Manual Maintenance Trigger");
            pR->TriggerRouterMaintenance();
            updateGrids();
        }
    }
    else
    {
        QMessageBox::information(this, "Trigger Maintenance", "Connect to the router first");
    }
}



#include "mfc_ipc.h"
#include "IPCUtil.h"
#include "mfcspymain.h"




#include <QApplication>


#ifdef _WIN32
char* __progname = "ShmSpy";
#else
const char* __progname = "ShmSpy";
#endif



int main(int argc, char *argv[])
{
    MFCIPC::IPCUtil::setupLogPath();  // defined in IPCUtil.h

    _TRACE("ShmSpy started");

    QApplication a(argc, argv);

    MFCSpyMain w;
    w.show();
    return a.exec();
}




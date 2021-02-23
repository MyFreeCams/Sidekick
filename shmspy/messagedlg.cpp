#include "messagedlg.h"
#include "ui_messagedlg.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/time_parsing.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

MessageDlg::MessageDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MessageDlg)
{
    ui->setupUi(this);
    ui->cbMessageType->addItem("Exit", 0);
    ui->cbMessageType->addItem("Hello", 1);
    
    ui->dtExpiration->setDate(QDate::currentDate());
    ui->dtExpiration->setTime(QTime::currentTime());
}

MessageDlg::~MessageDlg()
{
    delete ui;
}


std::string MessageDlg::getTo()
{
    std::string s = ui->txtTo->text().toStdString();
    return s;
}

std::string MessageDlg::getFrom()
{
    std::string s = ui->txtFrom->text().toStdString();
    return s;
    
}

std::string MessageDlg::getPayload()
{
    std::string s = ui->txtPayload->toPlainText().toStdString();
    return s;
    
}

int MessageDlg::getType()
{
    return ui->cbMessageType->currentIndex();
    
}

boost::posix_time::ptime MessageDlg::getExpiration() {
    
    QDateTime dt = ui->dtExpiration->dateTime().toLocalTime();
    QDate dd = ui->dtExpiration->date();
    std::string s1 = dd.toString().toStdString();
    std::string s2 = dt.toString("yyyy-MM-ddThh:mm:ss").toStdString();
    boost::posix_time::ptime time = boost::date_time::parse_delimited_time<boost::posix_time::ptime>(s2.c_str(), 'T');
    std::string s3 = boost::posix_time::to_simple_string(time);
    return time;
   // boost::posix_time::ptime time = boost::posix_time::second_clock::local_time();;
    return time;
}


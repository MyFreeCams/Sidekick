#ifndef MESSAGEDLG_H
#define MESSAGEDLG_H

#include <QDialog>
#include <boost/date_time/posix_time/posix_time_types.hpp>
namespace Ui {
class MessageDlg;
}

class MessageDlg : public QDialog
{
    Q_OBJECT

public:
    explicit MessageDlg(QWidget *parent = nullptr);
    ~MessageDlg();

    std::string getTo();
    std::string getFrom();
    std::string getPayload();
    int getType();
    boost::posix_time::ptime getExpiration();

    //    const char *getFrom();
//    const char *get
private:
    Ui::MessageDlg *ui;
};

#endif // MESSAGEDLG_H

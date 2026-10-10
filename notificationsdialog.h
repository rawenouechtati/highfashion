#ifndef NOTIFICATIONSDIALOG_H
#define NOTIFICATIONSDIALOG_H

#include <QDialog>

namespace Ui {
class NotificationsDialog;
}

class NotificationsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NotificationsDialog(QWidget *parent = nullptr);
    ~NotificationsDialog();

private:
    Ui::NotificationsDialog *ui;
};

#endif // NOTIFICATIONSDIALOG_H

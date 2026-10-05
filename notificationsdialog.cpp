#include "notificationsdialog.h"
#include "ui_notificationsdialog.h"

NotificationsDialog::NotificationsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NotificationsDialog)
{
    ui->setupUi(this);
}

NotificationsDialog::~NotificationsDialog()
{
    delete ui;
}

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "notificationsdialog.h"
GServices::GServices(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GServices)
{
    ui->setupUi(this);

    connect(ui->btnNotifications, &QPushButton::clicked, this, [this]()
            {
                NotificationsDialog dialog(this);
                dialog.exec();
            });
}

GServices::~GServices()
{
    delete ui;
}

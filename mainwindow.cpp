#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "notificationsdialog.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->btnNotifications, &QPushButton::clicked, this, [this]()
            {
                NotificationsDialog dialog(this);
                dialog.exec();
            });
}

MainWindow::~MainWindow()
{
    delete ui;
}

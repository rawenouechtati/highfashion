#include "clientsmainwindow.h"
#include "ui_clientsmainwindow.h"

GClients::GClients(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GClients)
{
    ui->setupUi(this);
}

GClients::~GClients()
{
    delete ui;
}

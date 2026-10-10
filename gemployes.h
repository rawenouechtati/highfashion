#ifndef GEMPLOYES_H
#define GEMPLOYES_H

#include <QMainWindow>
#include "mainwindow.h"
#include "clientsmainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class GEmployes;
}
QT_END_NAMESPACE

class GEmployes : public QMainWindow
{
    Q_OBJECT

public:
    explicit GEmployes(QWidget *parent = nullptr);
    ~GEmployes() override;

private:
    Ui::GEmployes *ui;
    GServices *fenetreServices = nullptr;
    GClients *fenetreClients = nullptr;
};
#endif // GEMPLOYES_H

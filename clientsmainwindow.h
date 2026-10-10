#ifndef CLIENTSMAINWINDOW_H
#define CLIENTSMAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class GClients;
}
QT_END_NAMESPACE

class GClients : public QMainWindow
{
    Q_OBJECT

public:
    explicit GClients(QWidget *parent = nullptr);
    ~GClients() override;

private:
    Ui::GClients *ui;
};
#endif // CLIENTSMAINWINDOW_H

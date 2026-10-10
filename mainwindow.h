#ifndef GSERVICES_H
#define GSERVICES_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class GServices;
}
QT_END_NAMESPACE

class GServices : public QMainWindow
{
    Q_OBJECT

public:
    explicit GServices(QWidget *parent = nullptr);
    ~GServices() override;

private:
    Ui::GServices *ui;
};
#endif // GSERVICES_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QClientWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class ClientWindow;
}
QT_END_NAMESPACE

class ClientWindow : public QClientWindow
{
    Q_OBJECT

public:
    explicit ClientWindow(QWidget *parent = nullptr);
    ~ClientWindow() override;

private:
    Ui::ClientWindow *ui;
};
#endif // MAINWINDOW_H

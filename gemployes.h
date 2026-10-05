#ifndef GEMPLOYES_H
#define GEMPLOYES_H

#include <QMainWindow>

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
};
#endif // GEMPLOYES_H

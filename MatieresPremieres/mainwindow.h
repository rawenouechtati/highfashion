#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Database.h"
#include <QMainWindow>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class DashboardPage;
class MaterialsPage;
class CategoriesPage;
class SuppliersPage;
class MovementsPage;
class NotificationsPage;

// Fenêtre principale : la barre latérale et la zone de pages viennent de Forms/mainwindow.ui
// (modifiable dans Qt Designer) ; les pages sont créées ici et regroupées en onglets sous l'unique entrée « Matières premières ».
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const QString &dbPath = QString(), QWidget *parent = nullptr);
    ~MainWindow() override;

    Database *database() const { return db_; }
    void showPage(int index);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void showToast(const QString &text, const QString &type);
    void onAlertsCreated(const QVector<Alert> &created);

private:
    Ui::MainWindow *ui;
    Database *db_;
    QTabWidget *tabs_;   // 4 onglets sous l'unique entrée « Matières premières »
    DashboardPage *dashboard_;
    MaterialsPage *materials_;
    CategoriesPage *categories_;
    SuppliersPage *suppliers_;
    MovementsPage *movements_;
    NotificationsPage *notifications_;
    QWidget *toastHost_;
    QVBoxLayout *toastLayout_;
};

#endif  // MAINWINDOW_H

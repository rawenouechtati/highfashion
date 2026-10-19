#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDate>
#include <QVector>
#include "service.h"

class QLabel;
class QTextEdit;
class QPushButton;
class QComboBox;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRowSelected();
    void recalc();
    void onSave();
    void onCancel();
    void onSearch(const QString &text);
    void exportPdf();
    void genererRecu();
    void envoyerRappels();
    void onAjouter();
    void onModifier();
    void onSupprimer();

private:
    void setupSidebar();
    void setupHeader();
    void setupFiche();
    void setupTable();
    void setupChart();
    void setupStatusBar();

    void loadDemoData();
    void fillRow(int row);
    void applyRowStyle(int row);
    void fillForm(int idx);
    void updateChart();
    void updateBanner();
    void updateCount();



    Ui::MainWindow *ui;
    QVector<Service> m_services;
    int m_current = -1;

    QComboBox   *cmbEmploye = nullptr;
    QLabel      *lblSoldeVal = nullptr;
    QLabel      *lblBenefVal = nullptr;
    QLabel      *lblCount = nullptr;
    QTextEdit   *txtAlertes = nullptr;
    QPushButton *btnExportPDF = nullptr;
    QPushButton *btnRappels = nullptr;
    QPushButton *btnGenerer = nullptr;
    QPushButton *btnEnregistrer = nullptr;
    QPushButton *btnAnnuler = nullptr;
};

#endif // MAINWINDOW_H
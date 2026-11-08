#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDate>
#include <QVector>
#include "service.h"
#include <QTimer>
#include <QWidget>
#include <QList>
#include <QPushButton>
#include <QHash>
#include <QRect>
#include <QSize>
#include <QString>
#include <QShowEvent>

class QLabel;
class QTextEdit;
class QPushButton;
class QComboBox;
class QScrollArea;
class QBoxLayout;
class QResizeEvent;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// === MODULE CLIENT : widget de diagramme (Maysam) ===
class DiagrammeWidget : public QWidget
{
public:
    explicit DiagrammeWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(120);
    }
    void setDonnees(const QVector<int> &c, int sel) { comptes = c; trancheSel = sel; update(); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QVector<int> comptes = QVector<int>(5, 0);
    int trancheSel = -1;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *e) override;

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
    void onPlanning();

    // ===================================================================
    // === MODULE CLIENT (Maysam) : travailler ici uniquement ============
    // ===================================================================

    void on_cli_pushButton_13_clicked();
    void on_cli_pushButton_6_clicked();            // bouton "générer" (reçu)
    void on_cli_pushButtonOuvrir_clicked();        // bouton "ouvrir" (contrat)
    void on_cli_pushButtonEnregistrer_clicked();
    void on_cli_pushButtonAnnuler_clicked();
    void on_cli_pushButton_4_clicked();            // bouton "supprimer"
    void rechercherClients(const QString &texte);
    void trierClients();
    void onAjouterClicked();
    void onModifierClicked();
    void exporterListePdf();

private:
    void setupSidebar();
    void setupHeader();
    void setupFiche();
    void setupTable();
    void setupChart();
    void setupStatusBar();
    void setupLayout();
    void applyResponsive();

    void loadDemoData();
    void fillRow(int row);
    void applyRowStyle(int row);
    void fillForm(int idx);
    void updateChart();
    void updateBanner();
    void updateCount();
    QVector<int> trouverConflits(const Service &s, int ignore) const;
    void verifierConflit();



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

    QLabel      *lblSousTitre = nullptr;
    QLabel      *lblConflit = nullptr;
    QScrollArea *scrollDetails = nullptr;
    QBoxLayout  *m_content = nullptr;
    int          m_layoutState = -1;

    // === SLOTS GÉNÉRAUX (navigation entre pages) ===
    void setupNavigation();

    // === MODULE CLIENT (Maysam) ===
    void initModuleClients();
        void viderChamps();
    bool emailValide(const QString &email) const;
    bool modeAjout = false;
    bool modeModif = false;
    int ligneModif = -1;
    QTimer *timerHorloge;
   void chargerClients();
    int idModif = -1;
   QList<QPushButton*> boutonsMenu;
   QString styleMenuActif;
   QString styleMenuNormal;
   void activerMenu(QPushButton *actif);
    DiagrammeWidget *diagramme;
    void majDiagramme();

    QHash<QWidget*, QRect> geomOrig;

    QSize baseSize;
    void collecterGeometries(QWidget *parent);
    void appliquerEchelle();
};

#endif // MAINWINDOW_H
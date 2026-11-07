#ifndef CLIENTSMAINWINDOW_H
#define CLIENTSMAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QWidget>
#include <QVector>



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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

private slots:

    void on_pushButton_13_clicked();
    void on_pushButton_6_clicked();            // bouton "générer" (reçu)
    void on_pushButtonOuvrir_clicked();        // bouton "ouvrir" (contrat)
    void on_pushButtonEnregistrer_clicked();
    void on_pushButtonAnnuler_clicked();
    void on_pushButton_4_clicked();            // bouton "supprimer"
    void rechercherClients(const QString &texte);
    void trierClients();
    void onAjouterClicked();
    void onModifierClicked();

private:
    Ui::MainWindow *ui;
    void viderChamps();
    bool emailValide(const QString &email) const;
    bool modeAjout = false;
    bool modeModif = false;
    int ligneModif = -1;
    QTimer *timerHorloge;
   void chargerClients();

    DiagrammeWidget *diagramme;
    void majDiagramme();

};

#endif // CLIENTSMAINWINDOW_H
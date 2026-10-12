#ifndef CLIENTSMAINWINDOW_H
#define CLIENTSMAINWINDOW_H

#include <QMainWindow>

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
    void on_pushButton_6_clicked();            // bouton "générer" (reçu)
    void on_pushButtonOuvrir_clicked();        // bouton "ouvrir" (contrat)
    void on_pushButtonEnregistrer_clicked();
    void on_pushButtonAnnuler_clicked();
    void on_pushButton_4_clicked();            // bouton "supprimer"
    void rechercherClients(const QString &texte);
    void trierClients();

private:
    Ui::MainWindow *ui;
    int prochainId = 10;
    void viderChamps();
    bool emailValide(const QString &email) const;
};

#endif // CLIENTSMAINWINDOW_H
#include "clientsmainwindow.h"
#include "ui_clientsmainwindow.h"

#include <QMessageBox>
#include <QRegularExpression>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tableWidget->setColumnCount(7);
    ui->tableWidget->setHorizontalHeaderLabels(
        {"id", "nom et prenom", "email", "telephone", "cin", "points", "niveau"});
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::emailValide(const QString &email) const
{
    static const QRegularExpression re(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    return re.match(email).hasMatch();
}

void MainWindow::on_pushButtonEnregistrer_clicked()
{
    const QString nom   = ui->lineEditNom->text().trimmed();
    const QString email = ui->lineEditEmail->text().trimmed();
    const QString tel   = ui->lineEditTel->text().trimmed();
    const QString cin   = ui->lineEditCin->text().trimmed();

    if (nom.isEmpty() || email.isEmpty() || tel.isEmpty() || cin.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants", "Veuillez remplir tous les champs.");
        return;
    }
    if (!emailValide(email)) {
        QMessageBox::warning(this, "Email invalide", "Veuillez saisir un email valide.");
        return;
    }
    if (cin.length() != 8 || cin.toInt() == 0) {
        QMessageBox::warning(this, "CIN invalide", "Le CIN doit contenir 8 chiffres.");
        return;
    }

    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        QTableWidgetItem *it = ui->tableWidget->item(i, 4);
        if (it && it->text() == cin) {
            QMessageBox::warning(this, "Doublon", "Un client avec ce CIN existe déjà.");
            return;
        }
    }

    const int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    const QString id = QString("C%1").arg(prochainId++, 3, 10, QChar('0'));

    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(id));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(nom));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(email));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(tel));
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem(cin));
    ui->tableWidget->setItem(row, 5, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(row, 6, new QTableWidgetItem("bronze"));

    ui->tableWidget->scrollToBottom();

    QMessageBox::information(this, "Succès", "Client enregistré avec succès.");
    viderChamps();
}

void MainWindow::on_pushButtonAnnuler_clicked()
{
    viderChamps();
}

void MainWindow::viderChamps()
{
    ui->lineEditNom->clear();
    ui->lineEditEmail->clear();
    ui->lineEditTel->clear();
    ui->lineEditCin->clear();
    ui->lineEditNom->setFocus();
}



void MainWindow::on_pushButtonSupprimer_clicked()
{
    int row = ui->tableWidget->currentRow();

    if (row < 0) {
        QMessageBox::warning(this, "Aucune sélection",
                             "Veuillez sélectionner un client dans le tableau.");
        return;
    }

    const QString nom = ui->tableWidget->item(row, 1)->text();

    QMessageBox::StandardButton rep = QMessageBox::question(
        this, "Confirmation",
        "Voulez-vous vraiment supprimer le client « " + nom + " » ?",
        QMessageBox::Yes | QMessageBox::No);

    if (rep == QMessageBox::Yes) {
        ui->tableWidget->removeRow(row);
        QMessageBox::information(this, "Succès", "Client supprimé.");
    }
}
void on_pushButtonEnregistrer_clicked();
void on_pushButtonAnnuler_clicked();
void on_pushButtonSupprimer_clicked();   // <- à ajouter
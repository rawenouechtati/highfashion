#include "clientsmainwindow.h"
#include "ui_clientsmainwindow.h"

#include <QMessageBox>
#include <QRegularExpression>
#include <QTableWidgetItem>
#include <QString>
#include <QComboBox>
#include <QTextDocument>
#include <QPdfWriter>
#include <QPageSize>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDate>

#include <QLineEdit>

#include <QTextDocument>
#include <QTextFrame>
#include <QPdfWriter>
#include <QPageSize>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDate>
#include <QColor>
#include <QPainter>

#include <QInputDialog>

// Item qui se trie selon un nombre (stocké dans Qt::UserRole)
class ItemTri : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;

    bool operator<(const QTableWidgetItem &other) const override
    {
        return data(Qt::UserRole).toInt() < other.data(Qt::UserRole).toInt();
    }
};




static int rangNiveau(const QString &niveau)
{
    const QString n = niveau.trimmed().toLower();
    if (n == "or")     return 3;
    if (n == "argent") return 2;
    return 1;               // bronze
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tableWidget->setColumnCount(7);
    ui->tableWidget->setHorizontalHeaderLabels(
        {"id", "nom et prenom", "email", "telephone", "cin", "points", "niveau"});
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ trierClients(); });
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(ui->lineEditRecherche, &QLineEdit::textChanged,
            this, &MainWindow::rechercherClients);

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

    // points et niveau : ItemTri pour que le tri soit numérique
    const int points = 0;
    const QString niveau = "bronze";

    ItemTri *itemPoints = new ItemTri(QString::number(points));
    itemPoints->setData(Qt::UserRole, points);
    ui->tableWidget->setItem(row, 5, itemPoints);

    ItemTri *itemNiveau = new ItemTri(niveau);
    itemNiveau->setData(Qt::UserRole, rangNiveau(niveau));
    ui->tableWidget->setItem(row, 6, itemNiveau);

    ui->tableWidget->scrollToBottom();

    QMessageBox::information(this, "Succès", "Client enregistré avec succès.");
    viderChamps();
}

void MainWindow::on_pushButtonAnnuler_clicked()
{
    viderChamps();
}

void MainWindow::viderChamps() {
    ui->lineEditNom->clear();
    ui->lineEditEmail->clear();
    ui->lineEditTel->clear();
    ui->lineEditCin->clear();
    ui->lineEditNom->setFocus();
}

void MainWindow::on_pushButton_4_clicked()
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
void MainWindow::trierClients()
{
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        QTableWidgetItem *ip = ui->tableWidget->item(r, 5);
        QTableWidgetItem *in = ui->tableWidget->item(r, 6);
        if (!ip || !in) continue;

        const QString pts = ip->text();
        const QString niv = in->text();

        ItemTri *p = new ItemTri(pts);
        p->setData(Qt::UserRole, pts.toInt());
        ui->tableWidget->setItem(r, 5, p);

        ItemTri *n = new ItemTri(niv);
        n->setData(Qt::UserRole, rangNiveau(niv));
        ui->tableWidget->setItem(r, 6, n);
    }

    const bool parNiveau = ui->comboBox->currentText().toLower().contains("niveau");
    ui->tableWidget->sortItems(parNiveau ? 6 : 5, Qt::DescendingOrder);
    ui->tableWidget->scrollToTop();
}
void MainWindow::rechercherClients(const QString &texte)
{
    const QString recherche = texte.trimmed().toLower();

    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        bool trouve = recherche.isEmpty();   // champ vide : tout afficher

        // colonnes : 1 = nom, 2 = email, 4 = cin
        const int colonnes[] = {1, 2, 4};
        for (int c : colonnes) {
            QTableWidgetItem *it = ui->tableWidget->item(r, c);
            if (it && it->text().toLower().contains(recherche)) {
                trouve = true;
                break;
            }
        }

        ui->tableWidget->setRowHidden(r, !trouve);
    }
}
    void MainWindow::on_pushButtonOuvrir_clicked()
    {
        // infos du formulaire, sinon du client sélectionné dans le tableau
        QString nom   = ui->lineEditNom->text().trimmed();
        QString email = ui->lineEditEmail->text().trimmed();
        QString tel   = ui->lineEditTel->text().trimmed();
        QString cin   = ui->lineEditCin->text().trimmed();

        const int row = ui->tableWidget->currentRow();
        if (nom.isEmpty() && row >= 0) {
            auto texte = [&](int c) {
                QTableWidgetItem *it = ui->tableWidget->item(row, c);
                return it ? it->text() : QString();
            };
            nom   = texte(1);
            email = texte(2);
            tel   = texte(3);
            cin   = texte(4);
        }

        if (nom.isEmpty()) {
            QMessageBox::warning(this, "Contrat",
                                 "Remplissez le formulaire ou sélectionnez un client dans le tableau.");
            return;
        }

        const QString fichier = QFileDialog::getSaveFileName(
            this, "Enregistrer le contrat",
            "contrat_" + cin + ".pdf", "Fichiers PDF (*.pdf)");
        if (fichier.isEmpty())
            return;

        const QString html = QString(R"HTML(
<body style="font-family:'Segoe UI','Arial',sans-serif; color:#1F3A4D; font-size:10pt;">

<table width="100%" cellpadding="18" cellspacing="0" bgcolor="#1F3A4D">
<tr>
  <td width="68%" bgcolor="#1F3A4D">
    <span style="font-size:22pt; font-weight:bold; color:#F5F1E8;">ATELIER VINTAGE COUTURE</span><br>
    <span style="font-size:9pt; color:#B39B6A;">ÉLÉGANCE &amp; TRADITION — GESTION DES SERVICES</span>
  </td>
  <td width="32%" align="right" bgcolor="#1F3A4D">
    <table cellpadding="9" cellspacing="0"><tr>
      <td bgcolor="#8B1E2D" align="center">
        <span style="font-size:11pt; font-weight:bold; color:#F5F1E8;">CONTRAT DE SERVICE</span>
      </td></tr></table>
  </td>
</tr>
</table>
<table width="100%" cellpadding="0" cellspacing="0"><tr><td bgcolor="#B39B6A" height="4"></td></tr></table>

<table width="100%" cellpadding="14" cellspacing="0" bgcolor="#F5F1E8"><tr><td>

<table width="58%" cellpadding="14" cellspacing="0" bgcolor="#FFFFFF" style="border:1px solid #D9CFB8;">
<tr><td>
  <table width="100%" cellpadding="0" cellspacing="0"><tr>
    <td><span style="font-size:13pt; font-weight:bold; color:#6B1424;">INFORMATIONS CLIENT</span></td>
    <td align="right"><span style="background-color:#F5F1E8; color:#1F3A4D; font-size:9pt;">&nbsp;Client Vérifié&nbsp;</span></td>
  </tr></table>
  <table width="100%" cellpadding="0" cellspacing="0"><tr><td bgcolor="#D9CFB8" height="1"></td></tr></table>
  <p style="margin-top:10px; line-height:170%;">
    Nom &amp; Prénom : <b>%2</b><br>
    Email : <b>%3</b><br>
    Téléphone : <b>%4</b><br>
    N° CIN : <b>%5</b>
  </p>
  <span style="font-size:8pt; color:#B39B6A;">Date du contrat : %1</span>
</td></tr>
</table>

<br>

<table width="100%" cellpadding="14" cellspacing="0" bgcolor="#FFFFFF" style="border:1px solid #D9CFB8;">
<tr><td>
  <span style="font-size:13pt; font-weight:bold; color:#6B1424;">CONDITIONS GÉNÉRALES D'EXÉCUTION</span>
  <table width="100%" cellpadding="0" cellspacing="0"><tr><td bgcolor="#D9CFB8" height="1"></td></tr></table>
  <p style="margin-top:10px; line-height:170%;">
    1. <b>Exactitude des informations :</b> Le client garantit la véracité des coordonnées transmises lors de la commande.<br>
    2. <b>Délais et Livraison :</b> L'atelier s'engage à respecter les délais de livraison spécifiés ci-dessus, sauf cas de force majeure.<br>
    3. <b>Politique d'acompte :</b> Tout acompte versé lors de la signature reste définitivement acquis à l'atelier en cas d'annulation.<br>
    4. <b>Garantie Qualité :</b> L'atelier garantit un travail d'artisanat conforme aux standards de la haute couture vintage.<br>
    5. <b>Programme de fidélité :</b> Les points acquis sont strictement personnels et non cessibles à un tiers.
  </p>
</td></tr>
</table>

<br>

<table width="100%" cellpadding="0" cellspacing="12">
<tr>
  <td width="50%" bgcolor="#FFFFFF" style="border:1px solid #D9CFB8;">
    <table width="100%" cellpadding="14" cellspacing="0"><tr><td align="center">
      <b>Signature &amp; Bon pour accord Client</b>
      <br><br>
      <span style="color:#B39B6A;">- - - - - - - - - - - - - - - - - - - - - - - -</span><br>
      <span style="font-size:8pt;">Nom : %2</span>
      <br><br><br>
    </td></tr></table>
  </td>
  <td width="50%" bgcolor="#FFFFFF" style="border:1px solid #D9CFB8;">
    <table width="100%" cellpadding="14" cellspacing="0"><tr><td align="center">
      <b>Pour l'Atelier Vintage Couture</b>
      <br><br>
      <span style="color:#B39B6A;">- - - - - - - - - - - - - - - - - - - - - - - -</span><br>
      <span style="font-size:8pt;">Cachet et signature autorisée</span>
      <br><br><br>
    </td></tr></table>
  </td>
</tr>
</table>

</td></tr></table>
</body>
)HTML")
                                 .arg(QDate::currentDate().toString("dd/MM/yyyy"),
                                      nom.toHtmlEscaped(), email.toHtmlEscaped(),
                                      tel.toHtmlEscaped(), cin.toHtmlEscaped());

        QPdfWriter pdf(fichier);
        pdf.setPageSize(QPageSize(QPageSize::A4));
        pdf.setPageMargins(QMarginsF(0, 0, 0, 0));
        pdf.setResolution(96);

        QTextDocument doc;
        doc.setHtml(html);
        doc.setPageSize(QSizeF(pdf.width(), pdf.height()));

        // fond crème sur toute la page
        QTextFrameFormat ff = doc.rootFrame()->frameFormat();
        ff.setBackground(QColor("#F5F1E8"));
        ff.setMargin(0);
        doc.rootFrame()->setFrameFormat(ff);

        // échelle : si le contenu dépasse la page, on le réduit pour tenir sur 1 page
        const qreal hauteurDoc  = doc.size().height();
        const qreal hauteurPage = pdf.height();
        qreal echelle = 1.0;
        if (hauteurDoc > hauteurPage)
            echelle = hauteurPage / hauteurDoc;

        QPainter painter(&pdf);
        painter.fillRect(QRectF(0, 0, pdf.width(), pdf.height()), QColor("#F5F1E8"));
        painter.scale(echelle, echelle);
        doc.drawContents(&painter);
        painter.end();

        QDesktopServices::openUrl(QUrl::fromLocalFile(fichier));
    }


    void MainWindow::on_pushButton_6_clicked()
    {
        // infos du formulaire, sinon du client sélectionné dans le tableau
        QString nom = ui->lineEditNom->text().trimmed();
        QString cin = ui->lineEditCin->text().trimmed();

        const int row = ui->tableWidget->currentRow();
        if (nom.isEmpty() && row >= 0) {
            QTableWidgetItem *n = ui->tableWidget->item(row, 1);
            QTableWidgetItem *c = ui->tableWidget->item(row, 4);
            nom = n ? n->text() : QString();
            cin = c ? c->text() : QString();
        }

        if (nom.isEmpty()) {
            QMessageBox::warning(this, "Reçu",
                                 "Remplissez le formulaire ou sélectionnez un client dans le tableau.");
            return;
        }

        // montant
        bool ok = false;
        const double montant = QInputDialog::getDouble(
            this, "Reçu de paiement", "Montant payé (DT) :",
            0.0, 0.0, 1000000.0, 3, &ok);
        if (!ok || montant <= 0) {
            if (ok)
                QMessageBox::warning(this, "Reçu", "Le montant doit être supérieur à 0.");
            return;
        }

        // mode de paiement
        const QStringList modes = {"Espèces", "Carte bancaire", "Chèque", "Virement"};
        const QString mode = QInputDialog::getItem(
            this, "Reçu de paiement", "Mode de paiement :", modes, 0, false, &ok);
        if (!ok)
            return;

        const QString fichier = QFileDialog::getSaveFileName(
            this, "Enregistrer le reçu",
            "recu_" + cin + ".pdf", "Fichiers PDF (*.pdf)");
        if (fichier.isEmpty())
            return;

        const QString numero = "RC-" + QDate::currentDate().toString("yyyyMMdd")
                               + "-" + cin.right(4);

        const QString html = QString(R"HTML(
<body style="font-family:'Segoe UI','Arial',sans-serif; color:#1F3A4D; font-size:10pt;">

<table width="100%" cellpadding="18" cellspacing="0" bgcolor="#1F3A4D">
<tr>
  <td width="68%" bgcolor="#1F3A4D">
    <span style="font-size:22pt; font-weight:bold; color:#F5F1E8;">ATELIER VINTAGE COUTURE</span><br>
    <span style="font-size:9pt; color:#B39B6A;">ÉLÉGANCE &amp; TRADITION — GESTION DES SERVICES</span>
  </td>
  <td width="32%" align="right" bgcolor="#1F3A4D">
    <table cellpadding="9" cellspacing="0"><tr>
      <td bgcolor="#8B1E2D" align="center">
        <span style="font-size:11pt; font-weight:bold; color:#F5F1E8;">REÇU DE PAIEMENT</span>
      </td></tr></table>
  </td>
</tr>
</table>
<table width="100%" cellpadding="0" cellspacing="0"><tr><td bgcolor="#B39B6A" height="4"></td></tr></table>

<table width="100%" cellpadding="14" cellspacing="0" bgcolor="#F5F1E8"><tr><td>

<table width="100%" cellpadding="14" cellspacing="0" bgcolor="#FFFFFF" style="border:1px solid #D9CFB8;">
<tr><td>
  <table width="100%" cellpadding="0" cellspacing="0"><tr>
    <td><span style="font-size:13pt; font-weight:bold; color:#6B1424;">DÉTAILS DU REÇU</span></td>
    <td align="right"><span style="font-size:9pt; color:#1F3A4D;">N° %2</span></td>
  </tr></table>
  <table width="100%" cellpadding="0" cellspacing="0"><tr><td bgcolor="#D9CFB8" height="1"></td></tr></table>
  <p style="margin-top:10px; line-height:170%;">
    Date : <b>%1</b><br>
    Reçu de : <b>%3</b><br>
    N° CIN : <b>%4</b>
  </p>
</td></tr>
</table>

<br>

<table width="100%" cellpadding="14" cellspacing="0" bgcolor="#FFFFFF" style="border:1px solid #D9CFB8;">
<tr><td>
  <span style="font-size:13pt; font-weight:bold; color:#6B1424;">PAIEMENT</span>
  <table width="100%" cellpadding="0" cellspacing="0"><tr><td bgcolor="#D9CFB8" height="1"></td></tr></table>
  <br>
  <table width="100%" cellpadding="10" cellspacing="0">
    <tr>
      <td width="40%" bgcolor="#F5F1E8"><b>Mode de paiement</b></td>
      <td bgcolor="#F5F1E8">%6</td>
    </tr>
    <tr>
      <td bgcolor="#1F3A4D"><span style="color:#F5F1E8;"><b>MONTANT PAYÉ</b></span></td>
      <td bgcolor="#1F3A4D" align="right"><span style="font-size:16pt; font-weight:bold; color:#B39B6A;">%5</span></td>
    </tr>
  </table>
</td></tr>
</table>

<br>

<table width="100%" cellpadding="0" cellspacing="12">
<tr>
  <td width="50%">&nbsp;</td>
  <td width="50%" bgcolor="#FFFFFF" style="border:1px solid #D9CFB8;">
    <table width="100%" cellpadding="14" cellspacing="0"><tr><td align="center">
      <b>Pour l'Atelier Vintage Couture</b>
      <br><br>
      <span style="color:#B39B6A;">- - - - - - - - - - - - - - - - - - - - - - - -</span><br>
      <span style="font-size:8pt;">Cachet et signature autorisée</span>
    </td></tr></table>
  </td>
</tr>
</table>

<p align="center" style="font-size:8pt; color:#B39B6A;">Merci de votre confiance. Ce reçu fait foi de paiement.</p>

</td></tr></table>
</body>
)HTML")
                                 .arg(QDate::currentDate().toString("dd/MM/yyyy"),
                                      numero,
                                      nom.toHtmlEscaped(),
                                      cin.toHtmlEscaped(),
                                      QString::number(montant, 'f', 3) + " DT",
                                      mode.toHtmlEscaped());

        QPdfWriter pdf(fichier);
        pdf.setPageSize(QPageSize(QPageSize::A4));
        pdf.setPageMargins(QMarginsF(0, 0, 0, 0));
        pdf.setResolution(96);

        QTextDocument doc;
        doc.setHtml(html);
        doc.setPageSize(QSizeF(pdf.width(), pdf.height()));

        QTextFrameFormat ff = doc.rootFrame()->frameFormat();
        ff.setBackground(QColor("#F5F1E8"));
        ff.setMargin(0);
        doc.rootFrame()->setFrameFormat(ff);

        const qreal hauteurDoc  = doc.size().height();
        const qreal hauteurPage = pdf.height();
        qreal echelle = 1.0;
        if (hauteurDoc > hauteurPage)
            echelle = hauteurPage / hauteurDoc;

        QPainter painter(&pdf);
        painter.fillRect(QRectF(0, 0, pdf.width(), pdf.height()), QColor("#F5F1E8"));
        painter.scale(echelle, echelle);
        doc.drawContents(&painter);
        painter.end();

        QDesktopServices::openUrl(QUrl::fromLocalFile(fichier));
    }


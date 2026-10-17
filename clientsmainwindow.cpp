#include "clientsmainwindow.h"
#include "ui_clientsmainwindow.h"
#include "client.h"

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
#include <QHeaderView>
#include <QLineEdit>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextFrame>
#include <QColor>
#include <QPainter>
#include <QInputDialog>
#include <QDateTime>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>



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

static void colorerNiveaux(QTableWidget *t)
{
    for (int r = 0; r < t->rowCount(); ++r) {
        QTableWidgetItem *it = t->item(r, 6);
        if (!it) continue;
        const QString n = it->text().trimmed().toLower();
        QColor fond("#F4D9C2"), texte("#8A4B1F");                   // bronze
        if (n == "or")          { fond = QColor("#F7E3A1"); texte = QColor("#8A6D1F"); }
        else if (n == "argent") { fond = QColor("#E4E7EB"); texte = QColor("#4A5560"); }
        it->setBackground(fond);
        it->setForeground(texte);
        it->setTextAlignment(Qt::AlignCenter);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);          // <-- TOUJOURS en premier

    QTableWidget *t = ui->tableWidget;
    t->verticalHeader()->setVisible(false);                         // enlève les numéros 1, 2, 3
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // toutes les colonnes tiennent
    t->horizontalHeader()->setHighlightSections(false);
    t->verticalHeader()->setDefaultSectionSize(26);
    t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    t->setAlternatingRowColors(true);
    t->setShowGrid(false);
    t->setFrameShape(QFrame::NoFrame);
    t->setStyleSheet(
        "QTableWidget { background:#FFFFFF; alternate-background-color:#FAF6EC;"
        "  color:#1F3A4D; border:none; selection-background-color:#F6D9DC;"
        "  selection-color:#1F3A4D; }"
        "QTableWidget::item { padding-left:6px; border:none; }"
        "QHeaderView::section { background:#1F3A4D; color:#FFFFFF; border:none;"
        "  padding:6px; font-weight:bold; }");

    ui->tableWidget->setColumnCount(7);
    ui->tableWidget->setHorizontalHeaderLabels(
        {"id", "nom et prenom", "email", "telephone", "cin", "points", "niveau"});
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ trierClients(); });

    ui->tableWidget->setRowCount(0);

    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(ui->lineEditRecherche, &QLineEdit::textChanged,
            this, &MainWindow::rechercherClients);
    QPixmap logo(":/logo_atelier.jpeg");
    ui->label_13->setGeometry(10, 20, 111, 60);
    ui->label_13->setAlignment(Qt::AlignCenter);
    ui->label_13->setPixmap(logo.scaled(ui->label_13->size(),
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation));
    ui->label_13->raise();

    connect(ui->pushButton_2, &QPushButton::clicked,
            this, &MainWindow::onAjouterClicked);

    connect(ui->pushButton_3, &QPushButton::clicked,
            this, &MainWindow::onModifierClicked);

    // date et heure automatiques
    ui->dateTimeEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->dateTimeEdit->setReadOnly(true);                          // l'utilisateur ne la modifie pas
    ui->dateTimeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons); // enlève les flèches ^ v

    timerHorloge = new QTimer(this);
    connect(timerHorloge, &QTimer::timeout, this, [this]() {
        ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    });
    timerHorloge->start(1000);   // mise à jour chaque seconde


    // diagramme dans le cadre "Répartition"
    diagramme = new DiagrammeWidget(ui->groupBox_2);
    diagramme->setGeometry(8, 22, ui->groupBox_2->width() - 16,
                           ui->groupBox_2->height() - 30);
    diagramme->show();

    connect(ui->tableWidget, &QTableWidget::currentCellChanged,
            this, [this](int, int, int, int){ majDiagramme(); });
     chargerClients();

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

    const bool modification = (modeModif && ligneModif >= 0);
    int id = 0;
    if (modification)
        id = ui->tableWidget->item(ligneModif, 0)->text().mid(1).toInt();

    Client c(id, nom, email, tel, cin);
    const bool ok = modification ? c.modifier() : c.ajouter();
    if (!ok) {
        const QString err = c.derniereErreur();
        if (err.contains("ORA-00001"))
            QMessageBox::warning(this, "Doublon", "Un client avec ce CIN existe déjà.");
        else
            QMessageBox::critical(this, "Erreur", err);
        return;
    }
    chargerClients();
    QMessageBox::information(this, "Succès",
                             modification ? "Client modifié avec succès." : "Client enregistré avec succès.");
    viderChamps();
    modeAjout = false;
    modeModif = false;
    ligneModif = -1;
    ui->groupBox->setTitle("Fiche client");
}

void MainWindow::on_pushButtonAnnuler_clicked()
{


    modeAjout = false;
    modeModif = false;
    ligneModif = -1;
    viderChamps();
    ui->groupBox->setTitle("Fiche client");
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
        const int id = ui->tableWidget->item(row, 0)->text().mid(1).toInt();

        Client c(id);
        if (!c.supprimer()) {
            QMessageBox::critical(this, "Erreur", c.derniereErreur());
            return;
        }
        chargerClients();
        QMessageBox::information(this, "Succès", "Client supprimé.");
    }
    majDiagramme();
}

void MainWindow::rechercherClients(const QString &texte)
{
    const QString recherche = texte.trimmed().toLower();
    int visibles = 0;
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
        if (trouve) ++visibles;

    }

    ui->lblCount->setText(QString("%1 clients affichés sur %2")
                              .arg(visibles).arg(ui->tableWidget->rowCount()));


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


void MainWindow::on_pushButton_13_clicked()
{
    QMessageBox::StandardButton rep = QMessageBox::question(
        this, "Déconnexion",
        "Voulez-vous vraiment vous déconnecter ?",
        QMessageBox::Yes | QMessageBox::No);

    if (rep == QMessageBox::Yes)
        close();
}


void MainWindow::onAjouterClicked()
{
    modeModif = false;
    ligneModif = -1;
    modeAjout = true;
    viderChamps();                       // vide les champs + focus sur "nom et prenom"
    ui->tableWidget->clearSelection();   // désélectionne la ligne du tableau

    ui->groupBox->setTitle("Fiche client  (nouveau)");

}
void MainWindow::onModifierClicked()
{
    const int row = ui->tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Aucune sélection",
                             "Veuillez sélectionner un client dans le tableau.");
        return;
    }

    auto texte = [&](int c) {
        QTableWidgetItem *it = ui->tableWidget->item(row, c);
        return it ? it->text() : QString();
    };

    // remplir la fiche avec les coordonnées du client
    ui->lineEditNom->setText(texte(1));
    ui->lineEditEmail->setText(texte(2));
    ui->lineEditTel->setText(texte(3));
    ui->lineEditCin->setText(texte(4));

    modeAjout = false;
    modeModif = true;
    ligneModif = row;

    ui->groupBox->setTitle("Fiche client  " + texte(0) + "  (modification)");
    ui->lineEditNom->setFocus();
}

void MainWindow::majDiagramme()
{
    auto tranche = [](int pts) {
        if (pts < 100) return 0;
        if (pts < 200) return 1;
        if (pts < 300) return 2;
        if (pts < 500) return 3;
        return 4;
    };

    QVector<int> comptes(5, 0);
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        QTableWidgetItem *it = ui->tableWidget->item(r, 5);
        if (it)
            comptes[tranche(it->text().toInt())]++;
    }

    int sel = -1;
    const int row = ui->tableWidget->currentRow();
    if (row >= 0 && ui->tableWidget->item(row, 5))
        sel = tranche(ui->tableWidget->item(row, 5)->text().toInt());

    diagramme->setDonnees(comptes, sel);
}

void DiagrammeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QStringList noms = {"0-99", "100-199", "200-299", "300-499", "500+"};
    int maxVal = 4;
    for (int v : comptes) maxVal = qMax(maxVal, v);

    const int margeG = 30, margeB = 25, margeH = 10, margeD = 10;
    const QRect zone(margeG, margeH, width() - margeG - margeD,
                     height() - margeH - margeB);

    // lignes de repère
    p.setPen(QColor("#D9CFB8"));
    for (int i = 0; i <= 4; ++i) {
        const int y = zone.bottom() - i * zone.height() / 4;
        p.drawLine(zone.left(), y, zone.right(), y);
        p.setPen(QColor("#1F3A4D"));
        p.drawText(0, y - 8, margeG - 5, 16, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(i * maxVal / 4));
        p.setPen(QColor("#D9CFB8"));
    }

    // barres
    const int n = comptes.size();
    const int largeurCase = zone.width() / n;
    const int largeurBarre = largeurCase * 6 / 10;

    for (int i = 0; i < n; ++i) {
        const int h = comptes[i] * zone.height() / maxVal;
        const QRect barre(zone.left() + i * largeurCase + (largeurCase - largeurBarre) / 2,
                          zone.bottom() - h, largeurBarre, h);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(i == trancheSel ? "#8B1E2D" : "#1F3A4D"));
        p.drawRoundedRect(barre, 3, 3);

        p.setPen(QColor("#1F3A4D"));
        p.drawText(barre.left() - 10, barre.top() - 16, largeurBarre + 20, 14,
                   Qt::AlignCenter, QString::number(comptes[i]));
        p.drawText(zone.left() + i * largeurCase, zone.bottom() + 4, largeurCase, 16,
                   Qt::AlignCenter, noms[i]);
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
    colorerNiveaux(ui->tableWidget);
}

void MainWindow::chargerClients()
{
    ui->tableWidget->setRowCount(0);

    QSqlQuery q;
    if (!Client::afficher(q))  {
        QMessageBox::critical(this, "Base de données", q.lastError().text());
        return;
    }

    while (q.next()) {
        const int r = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(r);

        ui->tableWidget->setItem(r, 0, new QTableWidgetItem(
                                           QString("C%1").arg(q.value(0).toInt(), 3, 10, QChar('0'))));
        ui->tableWidget->setItem(r, 1, new QTableWidgetItem(q.value(1).toString()));
        ui->tableWidget->setItem(r, 2, new QTableWidgetItem(q.value(2).toString()));
        ui->tableWidget->setItem(r, 3, new QTableWidgetItem(q.value(3).toString()));
        ui->tableWidget->setItem(r, 4, new QTableWidgetItem(q.value(4).toString()));

        const int pts = q.value(5).toInt();
        auto *ip = new ItemTri(QString::number(pts));
        ip->setData(Qt::UserRole, pts);
        ui->tableWidget->setItem(r, 5, ip);

        const QString niv = q.value(6).toString();
        auto *in = new ItemTri(niv);
        in->setData(Qt::UserRole, rangNiveau(niv));
        ui->tableWidget->setItem(r, 6, in);
    }
    colorerNiveaux(ui->tableWidget);
    const int total = ui->tableWidget->rowCount();
    ui->lblCount->setText(QString("%1 clients affichés sur %2").arg(total).arg(total));
    majDiagramme();
}


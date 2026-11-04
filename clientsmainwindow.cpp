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
#include <QDebug>
#include <QLabel>
#include <QGroupBox>
#include <QEvent>
#include <QTimer>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <functional>

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
    ui->dateTimeEdit->setReadOnly(true);                             // l'utilisateur ne la modifie pas
    ui->dateTimeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons); // enlève les flèches ^ v

    timerHorloge = new QTimer(this);
    connect(timerHorloge, &QTimer::timeout, this, [this]() {
        ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    });
    timerHorloge->start(1000);   // mise à jour chaque seconde

    connect(ui->pushButton_5, &QPushButton::clicked,
            this, &MainWindow::exporterListePdf);

    // diagramme dans le cadre "Répartition"
    diagramme = new DiagrammeWidget(ui->groupBox_2);
    diagramme->setGeometry(8, 22, ui->groupBox_2->width() - 16,
                           ui->groupBox_2->height() - 30);
    diagramme->show();

    connect(ui->tableWidget, &QTableWidget::currentCellChanged,
            this, [this](int, int, int, int){ majDiagramme(); });

    // couleurs du menu définies directement ici
    styleMenuActif =
        "QPushButton { background:#8B1E2D; color:#FFFFFF;"
        " border:1px solid #8B1E2D; border-radius:4px; padding:4px; }";
    styleMenuNormal =
        "QPushButton { background:#2A4B63; color:#FFFFFF;"
        " border:1px solid #3C5A70; border-radius:4px; padding:4px; }"
        "QPushButton:hover { background:#355F7C; }";

    const QStringList titres = {"tableau de bord", "article", "service", "client", "employ"};
    const auto boutons = ui->centralwidget->findChildren<QPushButton*>();
    for (QPushButton *b : boutons) {
        const QString tx = b->text().trimmed().toLower();
        for (const QString &titre : titres) {
            if (tx.startsWith(titre)) { boutonsMenu.append(b); break; }
        }
    }

    qDebug() << "boutons du menu trouves :" << boutonsMenu.size();

    for (QPushButton *b : boutonsMenu)
        connect(b, &QPushButton::clicked, this, [this, b]() { activerMenu(b); });

    for (QPushButton *b : boutonsMenu)                       // "Client" actif au démarrage
        if (b->text().trimmed().toLower() == "client")
            activerMenu(b);

    chargerClients();

    // =====================================================================
    //  NOUVELLE INTERFACE : vrais layouts, occupe toute la fenêtre
    // =====================================================================
    const QString cadreCarte =
        "#carte { background:#FFFFFF; border:1px solid #D9CFB8; border-radius:8px; }";
    auto carte = [cadreCarte]() {
        QFrame *f = new QFrame;
        f->setObjectName("carte");
        f->setStyleSheet(cadreCarte);
        return f;
    };

    const QString styleGroupe =
        "QGroupBox { background:#FFFFFF; border:1px solid #D9CFB8; border-radius:8px;"
        "  margin-top:12px; color:#1F3A4D; }"
        "QGroupBox::title { subcontrol-origin:margin; left:14px; padding:0 6px; }";

    const QString btnPlein =
        "QPushButton { background:#8B1E2D; color:#FFFFFF; border:1px solid #8B1E2D;"
        "  border-radius:6px; padding:8px 16px; font-weight:bold; }"
        "QPushButton:hover { background:#A02437; }";
    const QString btnContour =
        "QPushButton { background:#FFFFFF; color:#1F3A4D; border:1px solid #C9BFA8;"
        "  border-radius:6px; padding:8px 16px; }"
        "QPushButton:hover { background:#F5F1E8; }";
    const QString btnRouge =
        "QPushButton { background:#FFFFFF; color:#8B1E2D; border:1px solid #E3B8BE;"
        "  border-radius:6px; padding:8px 16px; }"
        "QPushButton:hover { background:#FBEFF1; }";
    const QString btnMarine =
        "QPushButton { background:#1F3A4D; color:#FFFFFF; border:1px solid #1F3A4D;"
        "  border-radius:6px; padding:8px 16px; font-weight:bold; }"
        "QPushButton:hover { background:#2A4B63; }";
    const QString styleChamp =
        "QLineEdit { background:#FFFFFF; border:1px solid #D9CFB8; border-radius:6px;"
        "  padding:6px 10px; color:#1F3A4D; }"
        "QLineEdit:focus { border:1px solid #8B1E2D; }";

    // cache les anciens widgets posés à la main dans un conteneur
    auto nettoyer = [](QWidget *conteneur, const QList<QWidget*> &garder) {
        const auto enfants = conteneur->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *w : enfants)
            if (!garder.contains(w))
                w->hide();
    };
    nettoyer(ui->groupBox, {ui->lineEditNom, ui->lineEditEmail, ui->lineEditTel,
                            ui->lineEditCin, ui->pushButton_6, ui->pushButtonOuvrir,
                            ui->pushButtonEnregistrer, ui->pushButtonAnnuler});
    nettoyer(ui->groupBox_2, {diagramme});
    delete ui->groupBox->layout();
    delete ui->groupBox_2->layout();

    // un champ de saisie avec son libellé et son message d'erreur
    auto ajouterChamp = [this, styleChamp](QVBoxLayout *lay, const QString &libelle,
                                           QLineEdit *champ,
                                           std::function<QString(const QString &)> verif) {
        QLabel *lbl = new QLabel(libelle);
        lbl->setStyleSheet("color:#1F3A4D; font-weight:bold; background:transparent;");
        QLabel *err = new QLabel;
        err->setStyleSheet("color:#C0392B; font-size:8pt; background:transparent;");
        err->setWordWrap(true);
        err->hide();
        champ->setStyleSheet(styleChamp);
        champ->setMinimumHeight(34);
        lay->addWidget(lbl);
        lay->addWidget(champ);
        lay->addWidget(err);
        connect(champ, &QLineEdit::textChanged, this, [err, verif](const QString &texte) {
            const QString v = texte.trimmed();
            const QString msg = v.isEmpty() ? QString() : verif(v);
            err->setText(msg);
            err->setVisible(!msg.isEmpty());
        });
    };

    QWidget *racine = new QWidget;
    racine->setObjectName("racine");
    racine->setStyleSheet("#racine { background:#F5F1E8; }");
    QHBoxLayout *principal = new QHBoxLayout(racine);
    principal->setContentsMargins(0, 0, 0, 0);
    principal->setSpacing(0);

    // ---------------- barre latérale ----------------
    QFrame *side = new QFrame;
    side->setObjectName("side");
    side->setStyleSheet("#side { background:#1F3A4D; }");
    side->setMinimumWidth(190);
    side->setMaximumWidth(250);
    QVBoxLayout *ls = new QVBoxLayout(side);
    ls->setContentsMargins(12, 16, 12, 14);
    ls->setSpacing(8);

    const QPixmap logoRedim = logo.scaledToWidth(170, Qt::SmoothTransformation);
    ui->label_13->setPixmap(logoRedim);
    ui->label_13->setAlignment(Qt::AlignCenter);
    ui->label_13->setFixedHeight(qMax(60, logoRedim.height() + 4));
    ls->addWidget(ui->label_13);
    ls->addSpacing(14);

    QList<QPushButton*> ordre;
    for (const QString &nomMenu : titres)
        for (QPushButton *bm : boutonsMenu)
            if (bm->text().trimmed().toLower().startsWith(nomMenu) && !ordre.contains(bm))
                ordre.append(bm);
    for (QPushButton *bm : ordre) {
        bm->setMinimumHeight(40);
        bm->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        ls->addWidget(bm);
    }
    ls->addStretch();

    QLabel *lblUser = new QLabel("r.clientele");
    lblUser->setStyleSheet("color:#FFFFFF; font-weight:bold; background:transparent;");
    QLabel *lblRole = new QLabel("Responsable clientèle");
    lblRole->setStyleSheet("color:#B39B6A; background:transparent;");
    ls->addWidget(lblUser);
    ls->addWidget(lblRole);
    ui->pushButton_13->setText("Déconnexion");
    ui->pushButton_13->setMinimumHeight(36);
    ui->pushButton_13->setStyleSheet(
        "QPushButton { background:transparent; color:#FFFFFF; border:1px solid #3C5A70;"
        "  border-radius:6px; padding:6px; }"
        "QPushButton:hover { background:#2A4B63; }");
    ls->addWidget(ui->pushButton_13);
    principal->addWidget(side);

    // styles du menu (texte aligné à gauche)
    styleMenuActif =
        "QPushButton { background:#8B1E2D; color:#FFFFFF; border:none; border-radius:6px;"
        "  padding:8px 14px; text-align:left; font-weight:bold; }";
    styleMenuNormal =
        "QPushButton { background:transparent; color:#E8E2D2; border:none; border-radius:6px;"
        "  padding:8px 14px; text-align:left; }"
        "QPushButton:hover { background:#2A4B63; }";
    for (QPushButton *bm : boutonsMenu)
        if (bm->text().trimmed().toLower() == "client")
            activerMenu(bm);

    // ---------------- colonne principale ----------------
    QVBoxLayout *colonne = new QVBoxLayout;
    colonne->setContentsMargins(20, 16, 20, 10);
    colonne->setSpacing(12);
    principal->addLayout(colonne, 1);

    // en-tête
    QHBoxLayout *entete = new QHBoxLayout;
    QVBoxLayout *blocTitre = new QVBoxLayout;
    QLabel *lblTitre = new QLabel("Gestion des clients");
    lblTitre->setStyleSheet("font-family:'Georgia','Times New Roman',serif; font-size:22pt;"
                            " font-weight:bold; color:#1F3A4D; background:transparent;");
    QLabel *lblSous = new QLabel("Fichier clientèle, fidélité et recommandations personnalisées");
    lblSous->setStyleSheet("color:#5C6B77; background:transparent;");
    blocTitre->addWidget(lblTitre);
    blocTitre->addWidget(lblSous);
    entete->addLayout(blocTitre);
    entete->addStretch();

    QVBoxLayout *blocDate = new QVBoxLayout;
    ui->dateTimeEdit->setFixedWidth(170);
    ui->dateTimeEdit->setAlignment(Qt::AlignRight);
    ui->dateTimeEdit->setStyleSheet("QDateTimeEdit { border:none; background:transparent;"
                                    " color:#1F3A4D; font-weight:bold; }");
    QLabel *lblSession = new QLabel("Session ouverte - Responsable clientèle");
    lblSession->setAlignment(Qt::AlignRight);
    lblSession->setStyleSheet("color:#5C6B77; background:transparent;");
    blocDate->addWidget(ui->dateTimeEdit, 0, Qt::AlignRight);
    blocDate->addWidget(lblSession);
    entete->addLayout(blocDate);
    colonne->addLayout(entete);

    // corps : colonne gauche + fiche client
    QHBoxLayout *corps = new QHBoxLayout;
    corps->setSpacing(14);
    QVBoxLayout *gauche = new QVBoxLayout;
    gauche->setSpacing(12);

    // barre d'outils
    QFrame *barre = carte();
    QHBoxLayout *lb = new QHBoxLayout(barre);
    lb->setContentsMargins(10, 8, 10, 8);
    lb->setSpacing(8);
    ui->lineEditRecherche->setPlaceholderText("Rechercher par nom, email ou CIN...");
    ui->lineEditRecherche->setStyleSheet(styleChamp);
    ui->lineEditRecherche->setMinimumHeight(34);
    ui->comboBox->setStyleSheet("QComboBox { background:#FFFFFF; border:1px solid #C9BFA8;"
                                " border-radius:6px; padding:6px 10px; color:#1F3A4D; }");
    ui->comboBox->setMinimumHeight(34);
    ui->pushButton_2->setText("+ Ajouter");
    ui->pushButton_3->setText("Modifier");
    ui->pushButton_4->setText("Supprimer");
    ui->pushButton_5->setText("Export PDF");
    ui->pushButton_2->setStyleSheet(btnPlein);
    ui->pushButton_3->setStyleSheet(btnContour);
    ui->pushButton_4->setStyleSheet(btnRouge);
    ui->pushButton_5->setStyleSheet(btnMarine);
    lb->addWidget(ui->lineEditRecherche, 1);
    lb->addWidget(ui->comboBox);
    lb->addWidget(ui->pushButton_2);
    lb->addWidget(ui->pushButton_3);
    lb->addWidget(ui->pushButton_4);
    lb->addWidget(ui->pushButton_5);
    gauche->addWidget(barre);

    // tableau
    QFrame *cTable = carte();
    QVBoxLayout *lt = new QVBoxLayout(cTable);
    lt->setContentsMargins(10, 10, 10, 8);
    ui->tableWidget->setMinimumHeight(150);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(30);
    ui->lblCount->setStyleSheet("color:#5C6B77; background:transparent;");
    lt->addWidget(ui->tableWidget, 1);
    lt->addWidget(ui->lblCount);
    gauche->addWidget(cTable, 3);

    // répartition + recommandations
    QHBoxLayout *bas = new QHBoxLayout;
    bas->setSpacing(12);

    ui->groupBox_2->setTitle("Répartition par points");
    ui->groupBox_2->setStyleSheet(styleGroupe);
    QVBoxLayout *lg2 = new QVBoxLayout(ui->groupBox_2);
    lg2->setContentsMargins(10, 22, 10, 10);
    lg2->addWidget(diagramme);
    bas->addWidget(ui->groupBox_2, 1);

    QGroupBox *reco = new QGroupBox("Recommandations : Sarra ben ali");
    reco->setStyleSheet(styleGroupe);
    QGridLayout *lr = new QGridLayout(reco);
    lr->setContentsMargins(14, 24, 14, 12);
    lr->setHorizontalSpacing(10);
    lr->setVerticalSpacing(2);
    lr->setColumnStretch(0, 1);
    struct Reco { QString nom, detail, taux; };
    const Reco recos[] = {
        {"Robe cocktail « New Look »", "Dior 1950 · 3 200 DT", "96 %"},
        {"Robe du soir",               "Givenchy 1950 · 2 600 DT", "91 %"},
        {"Robe corolle",               "Balmain 1950 · 1 950 DT", "84 %"},
        {"Ensemble jupe-veste",        "Dior 1950 · 2 800 DT", "78 %"}
    };
    int ligne = 0;
    for (const Reco &r : recos) {
        QLabel *n = new QLabel(r.nom);
        n->setStyleSheet("color:#1F3A4D; font-weight:bold; background:transparent;");
        QLabel *d = new QLabel(r.detail);
        d->setStyleSheet("color:#7A8791; background:transparent;");
        QPushButton *demo = new QPushButton("Démonstration");
        demo->setStyleSheet(
            "QPushButton { background:#FFFFFF; color:#8B1E2D; border:1px solid #D9B8BD;"
            "  border-radius:5px; padding:5px 12px; }"
            "QPushButton:hover { background:#FBEFF1; }");
        QLabel *p = new QLabel(r.taux);
        p->setStyleSheet("color:#1F3A4D; font-weight:bold; background:transparent;");
        lr->addWidget(n, ligne, 0);
        lr->addWidget(d, ligne + 1, 0);
        lr->addWidget(demo, ligne, 1, 2, 1, Qt::AlignVCenter);
        lr->addWidget(p, ligne, 2, 2, 1, Qt::AlignVCenter);
        ligne += 2;
    }
    QLabel *lblStyle = new QLabel("Style : années 50 · budget : 4 000 DT");
    lblStyle->setStyleSheet("color:#7A8791; font-size:8pt; background:transparent;");
    lr->addWidget(lblStyle, ligne, 0, 1, 3);
    bas->addWidget(reco, 1);

    gauche->addLayout(bas, 2);
    corps->addLayout(gauche, 3);

    // ---------------- fiche client ----------------
    ui->groupBox->setStyleSheet(styleGroupe);
    ui->groupBox->setMinimumWidth(300);
    ui->groupBox->setMaximumWidth(430);
    QVBoxLayout *lf = new QVBoxLayout(ui->groupBox);
    lf->setContentsMargins(16, 26, 16, 16);
    lf->setSpacing(4);

    ajouterChamp(lf, "Nom et prénom", ui->lineEditNom, [](const QString &v) {
        static const QRegularExpression re(R"(^[\p{L} '\-]+$)");
        return re.match(v).hasMatch() ? QString()
                                      : QString("Le nom ne doit contenir que des lettres");
    });
    ajouterChamp(lf, "Email", ui->lineEditEmail, [this](const QString &v) {
        return emailValide(v) ? QString()
                              : QString("Email invalide (ex : nom@domaine.com)");
    });
    ajouterChamp(lf, "Téléphone", ui->lineEditTel, [](const QString &v) {
        static const QRegularExpression re(R"(^[0-9 ]{8,12}$)");
        return re.match(v).hasMatch() ? QString()
                                      : QString("Le téléphone doit contenir 8 à 12 chiffres");
    });
    ajouterChamp(lf, "CIN", ui->lineEditCin, [](const QString &v) {
        static const QRegularExpression re(R"(^\d{8}$)");
        return re.match(v).hasMatch() ? QString()
                                      : QString("Le CIN doit contenir exactement 8 chiffres");
    });
    lf->addSpacing(6);

    // carte reçu
    QFrame *cRecu = carte();
    QHBoxLayout *lrc = new QHBoxLayout(cRecu);
    lrc->setContentsMargins(12, 10, 12, 10);
    QVBoxLayout *trc = new QVBoxLayout;
    QLabel *a1 = new QLabel("Reçu du paiement");
    a1->setStyleSheet("color:#1F3A4D; font-weight:bold; background:transparent;");
    QLabel *a2 = new QLabel("Ticket : montant et mode");
    a2->setStyleSheet("color:#7A8791; font-size:8pt; background:transparent;");
    trc->addWidget(a1);
    trc->addWidget(a2);
    lrc->addLayout(trc, 1);
    ui->pushButton_6->setText("Générer");
    ui->pushButton_6->setStyleSheet(btnPlein);
    lrc->addWidget(ui->pushButton_6);
    lf->addWidget(cRecu);

    // carte contrat
    QFrame *cContrat = carte();
    QHBoxLayout *lcc = new QHBoxLayout(cContrat);
    lcc->setContentsMargins(12, 10, 12, 10);
    QVBoxLayout *tcc = new QVBoxLayout;
    QLabel *b1 = new QLabel("Contrat en PDF");
    b1->setStyleSheet("color:#1F3A4D; font-weight:bold; background:transparent;");
    QLabel *b2 = new QLabel("Conditions et signature");
    b2->setStyleSheet("color:#7A8791; font-size:8pt; background:transparent;");
    tcc->addWidget(b1);
    tcc->addWidget(b2);
    lcc->addLayout(tcc, 1);
    ui->pushButtonOuvrir->setText("Ouvrir");
    ui->pushButtonOuvrir->setStyleSheet(btnContour);
    lcc->addWidget(ui->pushButtonOuvrir);
    lf->addWidget(cContrat);

    lf->addStretch();
    QHBoxLayout *lbtn = new QHBoxLayout;
    ui->pushButtonEnregistrer->setText("Enregistrer");
    ui->pushButtonAnnuler->setText("Annuler");
    ui->pushButtonEnregistrer->setStyleSheet(btnPlein);
    ui->pushButtonAnnuler->setStyleSheet(btnContour);
    ui->pushButtonEnregistrer->setMinimumHeight(38);
    ui->pushButtonAnnuler->setMinimumHeight(38);
    lbtn->addWidget(ui->pushButtonEnregistrer, 1);
    lbtn->addWidget(ui->pushButtonAnnuler, 1);
    lf->addLayout(lbtn);

    corps->addWidget(ui->groupBox, 1);
    colonne->addLayout(corps, 1);

    // on remplace l'ancien contenu par la nouvelle interface
    QWidget *ancien = takeCentralWidget();
    setCentralWidget(racine);
    if (ancien) {
        ancien->hide();
        ancien->deleteLater();
    }
    statusBar()->showMessage("Base Oracle : connectée   |   Rôle : Responsable clientèle");
}   // <-- fin du constructeur

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

    static const QRegularExpression reCin(R"(^\d{8}$)");
    if (!reCin.match(cin).hasMatch()) {
        QMessageBox::warning(this, "CIN invalide", "Le CIN doit contenir 8 chiffres.");
        return;
    }

    static const QRegularExpression reTel(R"(^[0-9 ]{8,12}$)");
    if (!reTel.match(tel).hasMatch()) {
        QMessageBox::warning(this, "Téléphone invalide",
                             "Le téléphone doit contenir entre 8 et 12 chiffres.");
        return;
    }

    const bool modification = (modeModif && idModif > 0);
    const int id = modification ? idModif : 0;

    Client c(id, nom, email, tel, cin);
    const bool ok = modification ? c.modifier() : c.ajouter();

    if (!ok) {
        const QString err = c.derniereErreur();
        if (err.contains("ORA-00001")) {
            if (err.contains("UQ_CLIENT_EMAIL"))
                QMessageBox::warning(this, "Doublon", "Cet email est déjà utilisé.");
            else
                QMessageBox::warning(this, "Doublon", "Un client avec ce CIN existe déjà.");
        } else if (err.contains("ORA-02290")) {
            QMessageBox::warning(this, "Données invalides",
                                 "Une valeur ne respecte pas les règles de la base (CIN, téléphone, email...).");
        } else {
            QMessageBox::critical(this, "Erreur", err);
        }
        return;
    }

    chargerClients();
    QMessageBox::information(this, "Succès",
                             modification ? "Client modifié avec succès." : "Client enregistré avec succès.");
    viderChamps();
    modeAjout = false;
    modeModif = false;
    ligneModif = -1;
    idModif = -1;
    ui->groupBox->setTitle("Fiche client");
}

void MainWindow::on_pushButtonAnnuler_clicked()
{
    modeAjout = false;
    modeModif = false;
    ligneModif = -1;
    viderChamps();
    ui->groupBox->setTitle("Fiche client");
    idModif = -1;
}

void MainWindow::viderChamps()
{
    ui->lineEditNom->clear();
    ui->lineEditEmail->clear();
    ui->lineEditTel->clear();
    ui->lineEditCin->clear();
    ui->lineEditNom->setFocus();
}

void MainWindow::on_pushButton_4_clicked()
{
    const int row = ui->tableWidget->currentRow();

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

    if (rep != QMessageBox::Yes)
        return;

    const int id = ui->tableWidget->item(row, 0)->text().mid(1).toInt();

    Client c(id);
    if (!c.supprimer()) {
        const QString err = c.derniereErreur();
        if (err.contains("ORA-02292"))
            QMessageBox::warning(this, "Suppression impossible",
                                 "Ce client est lié à un service ou un événement.");
        else
            QMessageBox::critical(this, "Erreur", err);
        return;
    }

    chargerClients();                   // recharge aussi le diagramme
    QMessageBox::information(this, "Succès", "Client supprimé.");
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
    idModif = -1;
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
    idModif = texte(0).mid(1).toInt();   // l'id du client, ex. "C005" -> 5

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
    //trierClients();                                           // réapplique le tri choisi
    //rechercherClients(ui->lineEditRecherche->text());         // réapplique le filtre
    majDiagramme();
}

void MainWindow::exporterListePdf()
{
    // lignes visibles du tableau (respecte la recherche et le tri)
    QString lignes;
    int nb = 0;
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        if (ui->tableWidget->isRowHidden(r))
            continue;

        auto txt = [&](int c) {
            QTableWidgetItem *it = ui->tableWidget->item(r, c);
            return it ? it->text().toHtmlEscaped() : QString();
        };

        const QString niv = txt(6).toLower();
        QString fond = "#F4D9C2", couleur = "#8A4B1F";            // bronze
        if (niv == "or")          { fond = "#F7E3A1"; couleur = "#8A6D1F"; }
        else if (niv == "argent") { fond = "#E4E7EB"; couleur = "#4A5560"; }

        const QString fondLigne = (nb % 2 == 0) ? "#FFFFFF" : "#FAF6EC";

        lignes += "<tr bgcolor=\"" + fondLigne + "\">";
        for (int c = 0; c < 6; ++c)
            lignes += "<td>" + txt(c) + "</td>";
        lignes += "<td align=\"center\" bgcolor=\"" + fond + "\">"
                  "<span style=\"color:" + couleur + ";\"><b>" + txt(6) + "</b></span></td></tr>";
        ++nb;
    }

    if (nb == 0) {
        QMessageBox::warning(this, "Export PDF", "Aucun client à exporter.");
        return;
    }

    const QString fichier = QFileDialog::getSaveFileName(
        this, "Enregistrer la liste des clients",
        "liste_clients_" + QDate::currentDate().toString("yyyyMMdd") + ".pdf",
        "Fichiers PDF (*.pdf)");
    if (fichier.isEmpty())
        return;

    QString html = R"HTML(
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
        <span style="font-size:11pt; font-weight:bold; color:#F5F1E8;">LISTE DES CLIENTS</span>
      </td></tr></table>
  </td>
</tr>
</table>
<table width="100%" cellpadding="0" cellspacing="0"><tr><td bgcolor="#B39B6A" height="4"></td></tr></table>

<table width="100%" cellpadding="14" cellspacing="0" bgcolor="#F5F1E8"><tr><td>

<table width="100%" cellpadding="14" cellspacing="0" bgcolor="#FFFFFF" style="border:1px solid #D9CFB8;">
<tr><td>
  <table width="100%" cellpadding="0" cellspacing="0"><tr>
    <td><span style="font-size:13pt; font-weight:bold; color:#6B1424;">FICHIER CLIENTÈLE</span></td>
    <td align="right"><span style="font-size:9pt; color:#1F3A4D;">)HTML";

    html += QDate::currentDate().toString("dd/MM/yyyy")
            + " — " + QString::number(nb) + " client(s)</span></td>";

    html += R"HTML(
  </tr></table>
  <table width="100%" cellpadding="0" cellspacing="0"><tr><td bgcolor="#D9CFB8" height="1"></td></tr></table>
  <br>
  <table width="100%" cellpadding="6" cellspacing="0">
    <tr bgcolor="#1F3A4D">
      <td><span style="color:#F5F1E8;"><b>ID</b></span></td>
      <td><span style="color:#F5F1E8;"><b>Nom et prénom</b></span></td>
      <td><span style="color:#F5F1E8;"><b>Email</b></span></td>
      <td><span style="color:#F5F1E8;"><b>Téléphone</b></span></td>
      <td><span style="color:#F5F1E8;"><b>CIN</b></span></td>
      <td><span style="color:#F5F1E8;"><b>Points</b></span></td>
      <td align="center"><span style="color:#F5F1E8;"><b>Niveau</b></span></td>
    </tr>
)HTML";

    html += lignes;

    html += R"HTML(
  </table>
</td></tr>
</table>

<p align="center" style="font-size:8pt; color:#B39B6A;">Atelier Vintage Couture — Document généré automatiquement.</p>

</td></tr></table>
</body>
)HTML";

    QPdfWriter pdf(fichier);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageMargins(QMarginsF(0, 0, 0, 0));
    pdf.setResolution(96);

    QTextDocument doc;
    doc.setHtml(html);

    QTextFrameFormat ff = doc.rootFrame()->frameFormat();
    ff.setBackground(QColor("#F5F1E8"));
    ff.setMargin(0);
    doc.rootFrame()->setFrameFormat(ff);

    doc.print(&pdf);        // gère aussi plusieurs pages si la liste est longue

    QDesktopServices::openUrl(QUrl::fromLocalFile(fichier));
}

void MainWindow::activerMenu(QPushButton *actif)
{
    for (QPushButton *b : boutonsMenu)
        b->setStyleSheet(b == actif ? styleMenuActif : styleMenuNormal);
}

void MainWindow::collecterGeometries(QWidget *parent)
{
    for (QObject *o : parent->children()) {
        QWidget *w = qobject_cast<QWidget*>(o);
        if (!w || w->isWindow())
            continue;

        geomOrig.insert(w, w->geometry());

        // on descend seulement dans les simples conteneurs
        const QString classe = w->metaObject()->className();
        if (classe == "QWidget" || classe == "QFrame" || classe == "QGroupBox")
            collecterGeometries(w);
    }
}

// UNE SEULE version de appliquerEchelle (géométrie + polices)
void MainWindow::appliquerEchelle()
{
    static QHash<QWidget*, qreal> fontOrig;
    if (baseSize.isEmpty() || geomOrig.isEmpty())
        return;

    const QSize s = ui->centralwidget->size();
    const qreal sx = qreal(s.width())  / baseSize.width();
    const qreal sy = qreal(s.height()) / baseSize.height();
    const qreal sf = qMin(sx, sy);                       // échelle des polices

    for (auto it = geomOrig.cbegin(); it != geomOrig.cend(); ++it) {
        QWidget *w = it.key();
        const QRect &g = it.value();
        w->setGeometry(qRound(g.x() * sx), qRound(g.y() * sy),
                       qRound(g.width() * sx), qRound(g.height() * sy));

        if (!fontOrig.contains(w))
            fontOrig.insert(w, w->font().pointSizeF());
        if (fontOrig[w] > 0) {
            QFont f = w->font();
            f.setPointSizeF(fontOrig[w] * sf);
            w->setFont(f);
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent *ev)
{
    QMainWindow::resizeEvent(ev);
}

void MainWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
}
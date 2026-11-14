#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "service.h"
#include "modulematieres.h"   // module Matieres premieres (dossier Matieres/)
#include "modulearticles.h"    // module Articles (dossier Articles/)
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDirIterator>
#include <QDateEdit>
#include <QCalendarWidget>
#include <QDialog>
#include <QListWidget>
#include <QSet>
#include <QPair>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QResizeEvent>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTextEdit>
#include <QtMath>
#include <QStackedWidget>
#include <QStatusBar>
// --- includes du module Client ---
#include "client.h"
#include <QPageSize>
#include <QInputDialog>
#include <QDateTime>
#include <QTextFrame>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <functional>
// --- includes du module Employe ---
#include "employe.h"
#include <QHeaderView>
#include <QBarSet>
#include <QBarSeries>
#include <QChart>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QMap>
#include <QFrame>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <QSqlQueryModel>
#include <QRegularExpression>
#include "posteTravail.h"
#include "planpostes.h"
#include "validationui.h"
#include "repartitionpostes.h"
#include "pictogrammes.h"
#include <QAction>
#include <QIcon>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QScrollArea>
#include <QMenuBar>
#include <QStatusBar>
#include "documentrh.h"
#include "radarstatistiques.h"
#include <QChartView>
#include <QDate>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QPainter>
#include <QPushButton>
#include <QUrl>
// --- module Evenement ---
#include <QCollator>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextCharFormat>
#include <QTime>
#include <QCalendarWidget>
#include <QListWidget>
#include <climits>

// ====================================================================================
// === SLOTS GÉNÉRAUX ===
// Construction de la fenetre principale et navigation entre les modules (stackedWidget principal).
// Les boutons de navigation sont programmes ici, pas dans les modules.
// ====================================================================================
MainWindow::MainWindow(const Employe &employeConnecte, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_employeConnecte(employeConnecte)
{
    ui->setupUi(this);
    setWindowTitle("SmartStitch - Atelier Vintage Couture");
    setMinimumSize(640, 480);
    const QRect avail = screen()->availableGeometry();
    resize(qMin(1600, avail.width()), qMin(860, avail.height()));

    connecterNavigation();        // zone SLOTS GÉNÉRAUX
    initialiserModuleService();   // zone MODULE SERVICE
    initialiserModuleEmploye();   // zone MODULE EMPLOYE
    initialiserModuleClient();    // zone MODULE CLIENT
    initialiserModuleEvenement(); // zone MODULE EVENEMENT
    initialiserModuleMatiere();   // zone MODULE MATIERE PREMIERE
    initialiserModuleArticle();   // zone MODULE ARTICLE

    // Page d'accueil : la page Employes (elle s'adapte au poste de l'employe connecte).
    // La barre d'etat "Role : Responsable des services" ne concerne que la page Services.
    ui->SWSmartStitch->setCurrentIndex(PageEmploye);
    statusBar()->setVisible(false);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::connecterNavigation()
{
    // menu lateral du module Services
    connect(ui->navTableauBordServ, &QPushButton::clicked, this, &MainWindow::allerVersServices);
    connect(ui->navArticlesServ,    &QPushButton::clicked, this, &MainWindow::allerVersArticles);
    connect(ui->navServicesServ,    &QPushButton::clicked, this, &MainWindow::allerVersServices);
    connect(ui->navClientsServ,     &QPushButton::clicked, this, &MainWindow::allerVersClients);
    connect(ui->navEmployesServ,    &QPushButton::clicked, this, &MainWindow::allerVersEmployes);

    // menu lateral du module Employes
    connect(ui->navArticlesEmpl,    &QPushButton::clicked, this, &MainWindow::allerVersArticles);
    connect(ui->navServicesEmpl,    &QPushButton::clicked, this, &MainWindow::allerVersServices);
    connect(ui->navClientsEmpl,     &QPushButton::clicked, this, &MainWindow::allerVersClients);
    connect(ui->navEmployesEmpl,    &QPushButton::clicked, this, &MainWindow::allerVersEmployes);

    // menu lateral du module Clients
    connect(ui->navTableauBordClient, &QPushButton::clicked, this, &MainWindow::allerVersServices);
    connect(ui->navArticlesClient,    &QPushButton::clicked, this, &MainWindow::allerVersArticles);
    connect(ui->navServicesClient,    &QPushButton::clicked, this, &MainWindow::allerVersServices);
    connect(ui->navClientsClient,     &QPushButton::clicked, this, &MainWindow::allerVersClients);
    connect(ui->navEmployesClient,    &QPushButton::clicked, this, &MainWindow::allerVersEmployes);

    // boutons "Evenements" et "Matieres premieres" ajoutes aux menus des autres modules
    connect(ui->navEvenementsServ,   &QPushButton::clicked, this, &MainWindow::allerVersEvenements);
    connect(ui->navMatieresServ,     &QPushButton::clicked, this, &MainWindow::allerVersMatieres);
    connect(ui->navEvenementsEmpl,   &QPushButton::clicked, this, &MainWindow::allerVersEvenements);
    connect(ui->navMatieresEmpl,     &QPushButton::clicked, this, &MainWindow::allerVersMatieres);
    connect(ui->navEvenementsClient, &QPushButton::clicked, this, &MainWindow::allerVersEvenements);
    connect(ui->navMatieresClient,   &QPushButton::clicked, this, &MainWindow::allerVersMatieres);

    // menu lateral du module Evenements
    connect(ui->btnNavEvenementsEvt,  &QPushButton::clicked, this, &MainWindow::allerVersEvenements);
    connect(ui->btnNavArticlesEvt,    &QPushButton::clicked, this, &MainWindow::allerVersArticles);
    connect(ui->btnNavServicesEvt,    &QPushButton::clicked, this, &MainWindow::allerVersServices);
    connect(ui->btnNavClientsEvt,     &QPushButton::clicked, this, &MainWindow::allerVersClients);
    connect(ui->btnNavEmployesEvt,    &QPushButton::clicked, this, &MainWindow::allerVersEmployes);
    connect(ui->btnNavMatieresEvt,    &QPushButton::clicked, this, &MainWindow::allerVersMatieres);
    connect(ui->btnNavDeconnexionEvt, &QPushButton::clicked, this, &MainWindow::onDeconnexionEvenementClicked);

    // menu lateral du module Matieres premieres
    connect(ui->btnNavEvenementsMat,  &QPushButton::clicked, this, &MainWindow::allerVersEvenements);
    connect(ui->btnNavArticlesMat,    &QPushButton::clicked, this, &MainWindow::allerVersArticles);
    connect(ui->btnNavServicesMat,    &QPushButton::clicked, this, &MainWindow::allerVersServices);
    connect(ui->btnNavClientsMat,     &QPushButton::clicked, this, &MainWindow::allerVersClients);
    connect(ui->btnNavEmployesMat,    &QPushButton::clicked, this, &MainWindow::allerVersEmployes);
    connect(ui->btnNavMatieresMat,    &QPushButton::clicked, this, &MainWindow::allerVersMatieres);
    connect(ui->btnNavDeconnexionMat, &QPushButton::clicked, this, &MainWindow::onDeconnexionMatiereClicked);

    // menu lateral du module Articles
    connect(ui->btnNavEvenementsArt,  &QPushButton::clicked, this, &MainWindow::allerVersEvenements);
    connect(ui->btnNavArticlesArt,    &QPushButton::clicked, this, &MainWindow::allerVersArticles);
    connect(ui->btnNavServicesArt,    &QPushButton::clicked, this, &MainWindow::allerVersServices);
    connect(ui->btnNavClientsArt,     &QPushButton::clicked, this, &MainWindow::allerVersClients);
    connect(ui->btnNavEmployesArt,    &QPushButton::clicked, this, &MainWindow::allerVersEmployes);
    connect(ui->btnNavMatieresArt,    &QPushButton::clicked, this, &MainWindow::allerVersMatieres);
    connect(ui->btnNavDeconnexionArt, &QPushButton::clicked, this, &MainWindow::onDeconnexionArticleClicked);

    // a chaque changement de page : la barre d'etat ("Role : Responsable des services") ne concerne
    // que la page Services, et la liste des clients est rechargee depuis la base
    connect(ui->SWSmartStitch, &QStackedWidget::currentChanged, this, [this](int index) {
        statusBar()->setVisible(index == PageService);
        if (index == PageClient)
            chargerClients();
        if (index == PageMatiere)
            ouvrirModuleMatiere();   // les matieres sont relues dans Oracle a chaque affichage
        if (index == PageArticle)
            ouvrirModuleArticle();   // les articles sont relus dans Oracle a chaque affichage
    });
}

void MainWindow::allerVersEmployes()   { ui->SWSmartStitch->setCurrentIndex(PageEmploye); }
void MainWindow::allerVersServices()
{
    remplirListesService();   // relit articles, clients et employés à chaque ouverture de la page
    ui->SWSmartStitch->setCurrentIndex(PageService);
}
void MainWindow::allerVersClients()    { ui->SWSmartStitch->setCurrentIndex(PageClient); }
void MainWindow::allerVersArticles()   { ui->SWSmartStitch->setCurrentIndex(PageArticle); }
void MainWindow::allerVersMatieres()   { ui->SWSmartStitch->setCurrentIndex(PageMatiere); }
void MainWindow::allerVersEvenements() { ui->SWSmartStitch->setCurrentIndex(PageEvenement); }
// === FIN SLOTS GÉNÉRAUX ===


// ====================================================================================
// === MODULE SERVICE ===
// ====================================================================================
// ---------- Badges colorés (colonnes Type et Statut) ----------
class BadgeDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(opt, idx);
        s.setWidth(QFontMetrics(opt.font).horizontalAdvance(idx.data().toString()) + 30);
        return s;
    }
    void paint(QPainter *p, const QStyleOptionViewItem &opt,
               const QModelIndex &idx) const override
    {
        QStyleOptionViewItem o(opt);
        initStyleOption(&o, idx);
        const QString t = o.text;
        o.text.clear();
        QStyle *st = o.widget ? o.widget->style() : QApplication::style();
        st->drawControl(QStyle::CE_ItemViewItem, &o, p, o.widget);
        static const QHash<QString, QPair<QColor, QColor>> map = {
            {"Location",         {QColor("#FBEFC9"), QColor("#9A6B00")}},
            {"Restauration",     {QColor("#D9EEE3"), QColor("#1E6B4A")}},
            {"Nettoyage",        {QColor("#E3E6EA"), QColor("#3A4A5C")}},
            {"Authentification", {QColor("#E3E6EA"), QColor("#3A4A5C")}},
            {"En cours",         {QColor("#DCE8F5"), QColor("#1F4E8C")}},
            {"En retard",        {QColor("#F8D7DB"), QColor("#8B1E2D")}},
            {"Terminé",          {QColor("#D9EEE3"), QColor("#1E6B4A")}},
            {"En attente",       {QColor("#EDEDED"), QColor("#555555")}}
        };
        auto it = map.constFind(t);
        if (it == map.constEnd()) {
            p->save();
            p->drawText(opt.rect.adjusted(6, 0, 0, 0), Qt::AlignVCenter, t);
            p->restore();
            return;
        }
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        QFontMetrics fm(opt.font);
        QRect r(opt.rect.left() + 6, opt.rect.center().y() - 9, fm.horizontalAdvance(t) + 18, 18);
        p->setBrush(it->first);
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(r, 9, 9);
        p->setPen(it->second);
        p->drawText(r, Qt::AlignCenter, t);
        p->restore();
    }
};

static void setCombo(QComboBox *c, const QString &t)
{
    if (t.isEmpty()) { c->setCurrentIndex(-1); return; }
    int i = c->findText(t);
    if (i < 0) { c->addItem(t); i = c->count() - 1; }
    c->setCurrentIndex(i);
}

static QString dt(double v)
{
    return QString("%1 DT").arg(v, 0, 'f', (v == qFloor(v)) ? 0 : 2);
}

// Libellé affiché à l'utilisateur pour un service : la référence (jamais l'ID interne)
static QString refService(const Service &s)
{
    return s.reference.isEmpty() ? s.type : s.reference;
}

// Remplit les combos Article / Client / Employé avec les données Oracle (en gardant la sélection)
void MainWindow::remplirListesService()
{
    auto remplir = [](QComboBox *c, const QStringList &liste, bool ligneVide) {
        if (!c || liste.isEmpty()) return;          // base injoignable : on ne vide pas la liste
        const QString courant = c->currentText();
        QSignalBlocker blocage(c);
        c->clear();
        if (ligneVide) c->addItem("");              // première ligne vide (champ facultatif)
        c->addItems(liste);
        if (courant.isEmpty())
            c->setCurrentIndex(ligneVide ? 0 : -1);
        else
            c->setCurrentIndex(c->findText(courant));
    };
    remplir(ui->cmbArticle, Service::listerArticles(), false);
    {   // Client : l'ID Oracle de chaque client est mémorisé dans les données de la ligne
        const ListeChoix clients = Service::listerClients();
        if (!clients.isEmpty()) {
            const int courant = ui->cmbClient->currentData().toInt();
            QSignalBlocker blocage(ui->cmbClient);
            ui->cmbClient->clear();
            ui->cmbClient->addItem("", 0);               // ligne vide : client facultatif
            for (const auto &c : clients)
                ui->cmbClient->addItem(c.second, c.first);
            const int i = ui->cmbClient->findData(courant);
            ui->cmbClient->setCurrentIndex(i >= 0 ? i : 0);
        }
    }
    remplir(cmbEmploye,     Service::listerEmployes(), false);
}

void MainWindow::initialiserModuleService()
{
    // métier innovant : planning intelligent (remplace les notifications système)
    ui->btnNotifications->setText("📅 Planning");
    connect(ui->btnNotifications, &QPushButton::clicked, this, &MainWindow::onPlanning);

    setupSidebar();
    setupHeader();
    setupFiche();
    setupTable();
    setupChart();
    setupStatusBar();
    setupLayout();      // assemble tous les layouts (responsive)

    remplirListesService();   // Article / Client / Employé depuis la base

    // Chargement via la classe Service
    m_services = Service::chargerDepuisDb();
    for (Service &sv : m_services) {             // nom du client lié (table PASSER)
        const int ic = ui->cmbClient->findData(sv.idClient);
        if (sv.idClient > 0 && ic >= 0) sv.client = ui->cmbClient->itemText(ic);
    }
    ui->tableServices->setRowCount(m_services.size());
    for (int r = 0; r < m_services.size(); ++r) fillRow(r);

    updateChart();
    updateBanner();
    updateCount();
    if (ui->tableServices->rowCount() > 0)
        ui->tableServices->selectRow(0);
}

// ---------- Menu latéral : logo + navigation + profil (layout vertical) ----------
void MainWindow::setupSidebar()
{
    QFrame *sb = ui->frameSidebar;
    sb->setFixedWidth(190);

    auto *lay = new QVBoxLayout(sb);
    lay->setContentsMargins(10, 24, 10, 14);
    lay->setSpacing(10);

    auto *logo = new QLabel(sb);
    logo->setFixedHeight(72);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("background: transparent;");

    // cherche le logo dans les ressources, quel que soit son dossier
    QString path;
    QDirIterator it(":/", QStringList() << "logo.jpeg*", QDir::Files,
                    QDirIterator::Subdirectories);
    if (it.hasNext()) path = it.next();
    qDebug() << "Logo trouve :" << path;
    QPixmap pm(path);
    if (path.isEmpty() || pm.isNull()) {
        qWarning("Logo introuvable ou illisible");
        logo->setText("Atelier Vintage Couture");
        logo->setWordWrap(true);
        logo->setStyleSheet("background:transparent; color:white; font-size:16px; font-weight:bold;");
    } else {
        logo->setPixmap(pm.scaled(QSize(170, 72), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    lay->addWidget(logo);
    lay->addSpacing(30);

    const QList<QPushButton *> nav = {ui->navTableauBordServ, ui->navArticlesServ, ui->navServicesServ,
                                      ui->navClientsServ, ui->navEmployesServ,
                                      ui->navEvenementsServ, ui->navMatieresServ};
    for (QPushButton *b : nav) {
        b->setMinimumHeight(46);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        lay->addWidget(b);
    }
    lay->addStretch(1);

    auto *profilRow = new QHBoxLayout;
    profilRow->setSpacing(8);
    auto *avatar = new QLabel("RS", sb);
    avatar->setFixedSize(36, 36);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("background:#B39B6A; color:white; font-weight:bold; border-radius:18px;");
    auto *profil = new QLabel("<b>r.services</b><br><small>Responsable des services</small>", sb);
    profil->setWordWrap(true);
    profil->setStyleSheet("background:transparent; color:white;");
    profilRow->addWidget(avatar);
    profilRow->addWidget(profil, 1);
    lay->addLayout(profilRow);

    auto *logout = new QPushButton("Déconnexion", sb);
    logout->setMinimumHeight(30);
    logout->setFlat(true);
    logout->setStyleSheet("QPushButton{background:transparent; color:#D8CFBC; text-align:left; padding-left:8px;}"
                          "QPushButton:hover{color:white; background:transparent;}");
    lay->addWidget(logout);
    connect(logout, &QPushButton::clicked, this, &MainWindow::deconnexionDemandee);
}

// ---------- En-tête : création des widgets (placés par setupLayout) ----------
void MainWindow::setupHeader()
{
    lblSousTitre = new QLabel("Location, restauration, nettoyage, authentification — "
                              "planning intelligent et alertes par e-mail",
                              ui->contenuServices);
    lblSousTitre->setWordWrap(true);
    lblSousTitre->setStyleSheet("color:#6B7A88; font-size:11px; background:transparent;");

    ui->txtRecherche->setPlaceholderText("Rechercher par client ou tarif...");
    ui->txtRecherche->setMinimumHeight(32);
    ui->cmbTri->setMinimumHeight(32);
    ui->cmbTri->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    ui->cmbTri->setMinimumContentsLength(10);

    btnExportPDF = new QPushButton("Export PDF", ui->contenuServices);
    btnExportPDF->setMinimumSize(130, 32);
    btnExportPDF->setStyleSheet("QPushButton{background-color:#1F3A4D; color:white; border-radius:6px;}"
                                "QPushButton:hover{background-color:#8B1E2D;}");
    connect(btnExportPDF, &QPushButton::clicked, this, &MainWindow::exportPdf);

    btnRappels = new QPushButton("✉ Envoyer les rappels", ui->contenuServices);
    btnRappels->setMinimumSize(170, 32);
    btnRappels->setStyleSheet("QPushButton{background-color:#8B1E2D; color:white;}");
    connect(btnRappels, &QPushButton::clicked, this, &MainWindow::envoyerRappels);

    // la bannière s'adapte à la largeur : le texte passe à la ligne si nécessaire
    ui->lblAlerte->setWordWrap(true);
    ui->lblAlerte->setMinimumHeight(41);
    ui->lblAlerte->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    connect(ui->btnAjouter,   &QPushButton::clicked, this, &MainWindow::onAjouter);
    connect(ui->btnModifier,  &QPushButton::clicked, this, &MainWindow::onModifier);
    connect(ui->btnSupprimer, &QPushButton::clicked, this, &MainWindow::onSupprimer);
    connect(ui->txtRecherche, &QLineEdit::textChanged, this, &MainWindow::onSearch);
}

// ---------- Fiche service (QGridLayout / QVBoxLayout) ----------
void MainWindow::setupFiche()
{
    QWidget *f = ui->frameDetails;

    auto *lblEmp = new QLabel("Employé assigné", f);
    lblEmp->setFont(ui->lblType->font());

    cmbEmploye = new QComboBox(f);
    // la liste des employés vient d'Oracle : voir remplirListesService()

    for (QWidget *w : {(QWidget *)ui->lblSoldeTitre, (QWidget *)ui->lblSolde,
                       (QWidget *)ui->lblBenefice, (QWidget *)ui->lblValeurBenefice})
        w->hide();

    auto makeCard = [f](const QString &name, const QString &titre, QLabel *&valeur,
                        const QString &bg, const QString &fg) -> QFrame * {
        auto *card = new QFrame(f);
        card->setObjectName(name);
        card->setMinimumHeight(56);
        card->setStyleSheet(QString("QFrame#%1{background:%2; border:1px solid %3; border-radius:6px;}"
                                    "QLabel{background:transparent; border:none; color:%3;}")
                                .arg(name, bg, fg));
        auto *v = new QVBoxLayout(card);
        v->setContentsMargins(12, 6, 12, 6);
        v->setSpacing(0);
        auto *t = new QLabel(titre.toUpper(), card);
        t->setStyleSheet(QString("font-size:10px; font-weight:bold; color:%1;").arg(fg));
        valeur = new QLabel("0 DT", card);
        valeur->setStyleSheet(QString("font-size:18px; font-weight:bold; color:%1;").arg(fg));
        v->addWidget(t);
        v->addWidget(valeur);
        return card;
    };
    QFrame *cardSolde = makeCard("cardSoldeNew", "Solde restant", lblSoldeVal, "#FBE9EC", "#8B1E2D");
    QFrame *cardBenef = makeCard("cardBenefNew", "Bénéfice",      lblBenefVal, "#FFF4DC", "#9A6B00");

    connect(ui->spinTarif,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::recalc);
    connect(ui->spinCout,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::recalc);
    // Par défaut un QDoubleSpinBox est limité à 99,99 : un tarif de 300 était écrasé en 99.99
    // à l'affichage, puis enregistré tel quel dans Oracle. On élargit les limites (NUMBER(10,2)).
    for (QDoubleSpinBox *sp : {ui->spinTarif, ui->spinCout, ui->spinAcompte}) {
        sp->setDecimals(2);
        sp->setRange(0.0, 99999999.99);
    }

    connect(ui->spinAcompte, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::recalc);

    // --- reçu
    auto *recu = new QFrame(f);
    recu->setObjectName("cardRecu");
    recu->setMinimumHeight(55);
    recu->setStyleSheet("QFrame#cardRecu{background:white; border:1px solid #E2D9C6; border-radius:8px;}"
                        "QLabel{background:transparent; border:none;}");
    auto *rl = new QHBoxLayout(recu);
    rl->setContentsMargins(12, 8, 12, 8);
    auto *rt = new QLabel("<b>Reçu de service</b><br><small>Ticket : tarif, acompte, solde</small>", recu);
    rt->setWordWrap(true);
    btnGenerer = new QPushButton("Générer", recu);
    btnGenerer->setMinimumSize(100, 31);
    btnGenerer->setStyleSheet("QPushButton{background-color:#8B1E2D; color:white; font-weight:bold;}");
    rl->addWidget(rt, 1);
    rl->addWidget(btnGenerer);
    connect(btnGenerer, &QPushButton::clicked, this, &MainWindow::genererRecu);

    // --- alertes
    ui->chkEcheance->hide();
    ui->chkRetard->hide();
    ui->frameAlertesEmail->setTitle("Alertes du service · système et e-mail");
    ui->frameAlertesEmail->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->frameAlertesEmail->setMaximumHeight(180);
    auto *al = new QHBoxLayout(ui->frameAlertesEmail);
    al->setContentsMargins(10, 14, 10, 8);
    al->setSpacing(10);
    txtAlertes = new QTextEdit(ui->frameAlertesEmail);
    txtAlertes->setReadOnly(true);
    txtAlertes->setMinimumHeight(54);
    txtAlertes->setStyleSheet("QTextEdit{background:#FFF8F0; border:1px solid #E2D9C6; border-radius:6px;}");
    txtAlertes->setPlainText("● Rappel envoyé par e-mail à Mouna Ayari — échéance dans 2 jours.");
    al->addWidget(txtAlertes, 1);
    ui->btnConfigurerAlertes->setText("Configurer");
    ui->btnConfigurerAlertes->setMinimumHeight(29);
    al->addWidget(ui->btnConfigurerAlertes, 0, Qt::AlignBottom);

    // --- boutons du bas
    btnEnregistrer = new QPushButton("Enregistrer", f);
    btnEnregistrer->setMinimumHeight(34);
    btnEnregistrer->setStyleSheet("QPushButton{background-color:#8B1E2D; color:white; font-weight:bold;}");
    btnAnnuler = new QPushButton("Annuler", f);
    btnAnnuler->setMinimumHeight(34);
    btnAnnuler->setStyleSheet("QPushButton{background-color:white; color:#1F3A4D; border:1px solid #B39B6A;}"
                              "QPushButton:hover{background-color:#F5F1E8;}");
    connect(btnEnregistrer, &QPushButton::clicked, this, &MainWindow::onSave);
    connect(btnAnnuler,     &QPushButton::clicked, this, &MainWindow::onCancel);

    // --- champs : grille 2 colonnes qui s'étire avec la largeur du panneau
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(4);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    const QList<QComboBox *> combos = {ui->cmbArticle, ui->cmbClient, ui->cmdType,
                                       cmbEmploye, ui->cmbStatut};
    for (QComboBox *c : combos) {
        c->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        c->setMinimumContentsLength(8);
    }
    const QList<QWidget *> champs = {ui->cmbArticle, ui->cmbClient, ui->cmdType, cmbEmploye,
                                     ui->dateDebutEdit, ui->dateFin, ui->spinTarif, ui->spinCout,
                                     ui->spinAcompte, ui->cmbStatut};
    for (QWidget *w : champs) {
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        w->setMinimumHeight(28);
    }

    struct Ligne { QWidget *l0, *w0, *l1, *w1; };
    const QList<Ligne> lignes = {
                                  {ui->lblArticle,   ui->cmbArticle,     ui->lblClient,  ui->cmbClient},
                                  {ui->lblType,      ui->cmdType,        lblEmp,         cmbEmploye},
                                  {ui->lblDateDebut, ui->dateDebutEdit,  ui->lblDateFin, ui->dateFin},
                                  {ui->lblTarif,     ui->spinTarif,      ui->lblCout,    ui->spinCout},
                                  {ui->lblAcompte,   ui->spinAcompte,    ui->lblStatut,  ui->cmbStatut},
                                  };
    int r = 0;
    for (const Ligne &l : lignes) {
        for (QWidget *lbl : {l.l0, l.l1}) {
            lbl->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            lbl->setMinimumHeight(0);
            lbl->setContentsMargins(0, r == 0 ? 0 : 4, 0, 0);
        }
        grid->addWidget(l.l0, r, 0);
        grid->addWidget(l.l1, r, 1);
        grid->addWidget(l.w0, r + 1, 0);
        grid->addWidget(l.w1, r + 1, 1);
        r += 2;
    }

    lblConflit = new QLabel(f);
    lblConflit->setWordWrap(true);
    lblConflit->setStyleSheet("background:#FBE9EC; color:#8B1E2D; border:1px solid #8B1E2D;"
                              "border-radius:6px; padding:6px; font-size:11px; font-weight:bold;");
    lblConflit->hide();

    // vérification en direct dès que l'article ou les dates changent
    connect(ui->cmbArticle, &QComboBox::currentTextChanged, this, [this]() { verifierConflit(); });
    connect(ui->dateDebutEdit, &QDateEdit::dateChanged, this, [this]() { verifierConflit(); });
    connect(ui->dateFin,       &QDateEdit::dateChanged, this, [this]() { verifierConflit(); });

    auto *root = new QVBoxLayout(f);
    root->setContentsMargins(14, 30, 14, 10);
    root->setSpacing(8);
    root->addLayout(grid);
    root->addWidget(lblConflit);

    auto *cards = new QHBoxLayout;
    cards->setSpacing(12);
    cards->addWidget(cardSolde, 1);
    cards->addWidget(cardBenef, 1);
    root->addLayout(cards);

    root->addWidget(recu);
    root->addWidget(ui->frameAlertesEmail, 100);   // absorbe l'espace libre (jusqu'à 180 px)

    auto *btns = new QHBoxLayout;
    btns->setSpacing(12);
    btns->addWidget(btnEnregistrer, 1);
    btns->addWidget(btnAnnuler, 1);
    root->addLayout(btns);
    root->addStretch(1);   // sur très grand écran, le surplus va sous les boutons
}

// ---------- Tableau ----------
void MainWindow::setupTable()
{
    auto *t = ui->tableServices;
    t->setHorizontalHeaderLabels({"Référence", "Article", "Client", "Type", "Employé",
                                  "Fin prévue", "Tarif", "Statut"});
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setAlternatingRowColors(true);
    t->setShowGrid(false);
    t->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    t->verticalHeader()->hide();
    t->verticalHeader()->setDefaultSectionSize(27);

    // colonnes : les textes longs se partagent la largeur disponible,
    // les colonnes courtes (ID, type, date, tarif, statut) s'ajustent à leur contenu
    auto *h = t->horizontalHeader();
    h->setStretchLastSection(false);
    h->setHighlightSections(false);
    h->setMinimumSectionSize(70);
    h->setSectionResizeMode(0, QHeaderView::ResizeToContents);   // ID
    h->setSectionResizeMode(1, QHeaderView::Stretch);            // Article
    h->setSectionResizeMode(2, QHeaderView::Stretch);            // Client
    h->setSectionResizeMode(3, QHeaderView::ResizeToContents);   // Type
    h->setSectionResizeMode(4, QHeaderView::Stretch);            // Employé
    h->setSectionResizeMode(5, QHeaderView::ResizeToContents);   // Fin prévue
    h->setSectionResizeMode(6, QHeaderView::ResizeToContents);   // Tarif
    h->setSectionResizeMode(7, QHeaderView::ResizeToContents);   // Statut

    t->setItemDelegateForColumn(3, new BadgeDelegate(t));
    t->setItemDelegateForColumn(7, new BadgeDelegate(t));
    t->setStyleSheet(
        "QTableWidget{background:white; alternate-background-color:#FAF7F1; border:1px solid #E2D9C6; border-radius:8px;}"
        "QHeaderView::section{background:#1F3A4D; color:white; padding:6px; border:none; font-weight:bold;}"
        "QTableWidget::item:selected{background:#F3D9DD; color:#1F3A4D;}");

    lblCount = new QLabel(ui->contenuServices);
    lblCount->setWordWrap(true);
    lblCount->setStyleSheet("color:#6B7A88; font-size:10px; background:transparent;");

    connect(t, &QTableWidget::itemSelectionChanged, this, &MainWindow::onRowSelected);
}

void MainWindow::loadDemoData()
{
    m_services = {
                   {"SV001","Tailleur bouclé · Chanel","Ines Mansouri","Location","Mouna Ayari",QDate(2026,9,20),QDate(2026,9,28),120,45,80,"En cours"},
                   {"SV002","Smoking « Le Smoking »","","Authentification","Mouna Ayari",QDate(2026,9,15),QDate(2026,9,24),200,70,100,"En cours"},
                   {"SV003","Robe trapèze · Balenciaga","","Restauration","Sami Guesmi",QDate(2026,8,30),QDate(2026,9,10),350,140,150,"En retard"},
                   {"SV004","Sac matelassé · Chanel","Yassine Trabelsi","Location","Mouna Ayari",QDate(2026,9,18),QDate(2026,9,25),260,90,120,"En cours"},
                   {"SV005","Robe du soir · Givenchy","","Nettoyage","Karim Fekih",QDate(2026,9,10),QDate(2026,9,18),80,25,80,"Terminé"},
                   {"SV006","Manteau ceinturé · Carven","","Restauration","Amine Sassi",QDate(2026,9,20),QDate(2026,9,30),300,120,100,"En attente"},
                   {"SV007","Foulard en soie · Hermès","","Nettoyage","Salma Trabelsi",QDate(2026,9,12),QDate(2026,9,19),70,20,70,"Terminé"},
                   {"SV008","Robe fourreau · Balmain","","Authentification","Mouna Ayari",QDate(2026,8,25),QDate(2026,9,5),180,60,90,"En retard"},
                   {"SV009","Cape en velours · Dior","Nour Gharbi","Location","Mouna Ayari",QDate(2026,9,1),QDate(2026,9,12),150,55,75,"En cours"},
                   };
    auto *t = ui->tableServices;
    t->setRowCount(m_services.size());
    for (int r = 0; r < m_services.size(); ++r) fillRow(r);
}

void MainWindow::fillRow(int r)
{
    const Service &s = m_services[r];
    const QStringList v = { s.reference.isEmpty() ? QString("Nouveau") : s.reference, s.article, s.client.isEmpty() ? "—" : s.client, s.type,
                           s.employe, s.fin.toString("dd/MM/yyyy"),
                           QString::number(s.tarif, 'f', 0), s.statut };
    for (int c = 0; c < v.size(); ++c)
        ui->tableServices->setItem(r, c, new QTableWidgetItem(v[c]));
    applyRowStyle(r);
}

void MainWindow::applyRowStyle(int r)
{
    const bool late = (m_services[r].statut == "En retard");
    for (int c = 0; c < ui->tableServices->columnCount(); ++c)
        if (auto *it = ui->tableServices->item(r, c))
            it->setBackground(late ? QBrush(QColor("#FBE3E6")) : QBrush());
}

// ---------- Fiche <-> tableau ----------
void MainWindow::onRowSelected()
{
    m_current = ui->tableServices->currentRow();
    fillForm(m_current);
}

void MainWindow::fillForm(int i)
{
    if (i < 0 || i >= m_services.size()) return;
    const Service &s = m_services[i];
    ui->frameDetails->setTitle(s.reference.isEmpty() ? QString("Fiche service") : "Fiche service · " + s.reference);
    setCombo(ui->cmbArticle, s.article);
    const int ic = ui->cmbClient->findData(s.idClient);
    ui->cmbClient->setCurrentIndex(ic >= 0 ? ic : 0);   // 0 = ligne vide
    setCombo(ui->cmdType,    s.type);
    setCombo(cmbEmploye,     s.employe);
    setCombo(ui->cmbStatut,  s.statut);
    ui->dateDebutEdit->setDate(s.debut);
    ui->dateFin->setDate(s.fin);
    ui->spinTarif->setValue(s.tarif);
    ui->spinCout->setValue(s.cout);
    ui->spinAcompte->setValue(s.acompte);
    recalc();
    verifierConflit();
}

void MainWindow::recalc()
{
    lblSoldeVal->setText(dt(ui->spinTarif->value() - ui->spinAcompte->value()));
    lblBenefVal->setText(dt(ui->spinTarif->value() - ui->spinCout->value()));
}

void MainWindow::onSave()
{
    if (m_current < 0) return;
    Service s = m_services[m_current];   // copie : rien n'est modifié tant que tout n'est pas valide
    s.article = ui->cmbArticle->currentText();
    s.client  = ui->cmbClient->currentText();
    s.idClient = ui->cmbClient->currentData().toInt();
    s.type    = ui->cmdType->currentText();
    s.employe = cmbEmploye->currentText();
    s.statut  = ui->cmbStatut->currentText();
    s.debut   = ui->dateDebutEdit->date();
    s.fin     = ui->dateFin->date();
    s.tarif   = ui->spinTarif->value();
    s.cout    = ui->spinCout->value();
    s.acompte = ui->spinAcompte->value();

    // Contrôle de saisie AVANT tout enregistrement
    QString erreur;
    if (!s.valider(erreur)) {
        QMessageBox::warning(this, "Contrôle de saisie", erreur);
        return;
    }

    // Métier innovant : un même article ne peut pas être dans deux services qui se chevauchent
    const QVector<int> conflits = trouverConflits(s, m_current);
    if (!conflits.isEmpty()) {
        QStringList lignes;
        QDate libre = s.debut;
        for (int i : conflits) {
            const Service &o = m_services[i];
            lignes << QString("• %1 (%2 → %3, %4)").arg(refService(o), o.debut.toString("dd/MM/yyyy"),
                                                        o.fin.toString("dd/MM/yyyy"), o.type);
            libre = qMax(libre, o.fin.addDays(1));
        }
        QMessageBox::warning(this, "Conflit de dates",
                             QString("L'article « %1 » est déjà engagé dans :\n%2\n\n"
                                     "Prochaine disponibilité : %3.\nChangez les dates ou l'article.")
                                 .arg(s.article, lignes.join("\n"), libre.toString("dd/MM/yyyy")));
        return;
    }

    // 1) Base de données d'abord : INSERT si le service n'a pas encore d'ID, sinon UPDATE
    const bool ok = s.id.isEmpty() ? s.ajouter() : s.modifier();
    if (!ok) {
        QMessageBox::critical(this, "Oracle",
                              "Échec de l'enregistrement dans la base.\nVoir la console pour le détail.");
        return;
    }

    // 2) Puis l'interface (s.id contient maintenant l'ID généré, jamais affiché)
    m_services[m_current] = s;
    fillRow(m_current);
    ui->tableServices->selectRow(m_current);
    updateChart();
    updateBanner();
    updateCount();
    statusBar()->showMessage("Service " + s.reference + " enregistré", 3000);
}

void MainWindow::onCancel() { fillForm(m_current); }

void MainWindow::onSearch(const QString &text)
{
    auto *t = ui->tableServices;
    for (int r = 0; r < t->rowCount(); ++r) {
        bool ok = text.isEmpty();
        for (int c : {0, 1, 2, 6})
            if (t->item(r, c) && t->item(r, c)->text().contains(text, Qt::CaseInsensitive))
                ok = true;
        t->setRowHidden(r, !ok);
    }
    updateCount();
}

void MainWindow::updateCount()
{
    int n = 0;
    for (int r = 0; r < ui->tableServices->rowCount(); ++r)
        if (!ui->tableServices->isRowHidden(r)) ++n;
    lblCount->setText(QString("%1 services affichés · les retards sont détectés à chaque "
                              "ouverture et toutes les heures").arg(n));
}

void MainWindow::updateBanner()
{
    QStringList ids;
    for (const Service &s : std::as_const(m_services))
        if (s.statut == "En retard") ids << refService(s);
    ui->lblAlerte->setText(ids.isEmpty()
                               ? "Aucun service en retard"
                               : QString("⚠ %1 services dépassent leur date de fin prévue (%2)")
                                     .arg(ids.size()).arg(ids.join(" · ")));
}

// ---------- Statistiques : le plus demandé / le plus rentable ----------
void MainWindow::setupChart()
{
    ui->frameStats->setTitle("Statistiques annuelles : demande et rentabilité");
    for (QWidget *w : {(QWidget *)ui->lblPlusDemande, (QWidget *)ui->lblServiceDemande,
                       (QWidget *)ui->barDemande, (QWidget *)ui->lblPlusRentable,
                       (QWidget *)ui->lblServiceRentable, (QWidget *)ui->barRentabilite})
        w->hide();

    auto *row = new QHBoxLayout(ui->frameStats);
    row->setContentsMargins(10, 30, 10, 10);
    row->setSpacing(12);

    auto makeCard = [this, row](const QString &prefix, const QString &titre,
                                const QString &main, const QString &tint) {
        auto *card = new QFrame(ui->frameStats);
        card->setObjectName(prefix + "_card");
        card->setProperty("main", main);
        card->setProperty("tint", tint);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        card->setStyleSheet(QString("QFrame#%1_card{background:white; border:1px solid #E2D9C6; border-radius:10px;}"
                                    "QLabel{background:transparent; border:none;}").arg(prefix));

        auto *outer = new QVBoxLayout(card);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        auto *accent = new QFrame(card);
        accent->setFixedHeight(5);
        accent->setStyleSheet(QString("background:%1; border:none; border-top-left-radius:9px; border-top-right-radius:9px;").arg(main));
        outer->addWidget(accent);

        auto *body = new QVBoxLayout;
        body->setContentsMargins(17, 12, 17, 12);
        body->setSpacing(4);
        outer->addLayout(body, 1);

        auto *t = new QLabel(titre.toUpper(), card);
        QFont ft = t->font();
        ft.setBold(true);
        ft.setPixelSize(11);
        ft.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        t->setFont(ft);
        t->setStyleSheet("color:#6B7A88;");
        body->addWidget(t);

        auto *name = new QLabel("—", card);
        name->setObjectName(prefix + "_name");
        name->setStyleSheet(QString("font-size:26px; font-weight:bold; color:%1;").arg(main));
        body->addWidget(name);

        auto *sub = new QLabel("", card);
        sub->setObjectName(prefix + "_sub");
        sub->setStyleSheet("font-size:12px; color:#6B7A88;");
        body->addWidget(sub);

        auto *line = new QFrame(card);
        line->setFixedHeight(1);
        line->setStyleSheet("background:#EFE7D6; border:none;");
        body->addWidget(line);
        body->addSpacing(6);

        // 4 lignes : libellé | barre (s'étire) | valeur
        auto *grid = new QGridLayout;
        grid->setHorizontalSpacing(12);
        grid->setVerticalSpacing(10);
        grid->setColumnStretch(1, 1);
        for (int i = 0; i < 4; ++i) {
            auto *lbl = new QLabel("", card);
            lbl->setObjectName(QString("%1_lbl_%2").arg(prefix).arg(i));
            lbl->setMinimumWidth(110);
            lbl->setStyleSheet("font-size:12px; color:#1F3A4D;");

            auto *bar = new QProgressBar(card);
            bar->setObjectName(QString("%1_bar_%2").arg(prefix).arg(i));
            bar->setRange(0, 1000);
            bar->setValue(0);
            bar->setTextVisible(false);
            bar->setFixedHeight(14);
            bar->setMinimumWidth(40);

            auto *val = new QLabel("", card);
            val->setObjectName(QString("%1_val_%2").arg(prefix).arg(i));
            val->setMinimumWidth(70);
            val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            val->setStyleSheet("font-size:12px; font-weight:bold; color:#1F3A4D;");

            grid->addWidget(lbl, i, 0);
            grid->addWidget(bar, i, 1);
            grid->addWidget(val, i, 2);
        }
        body->addLayout(grid);
        body->addStretch(1);

        row->addWidget(card, 1);
    };

    makeCard("dem", "Service le plus demandé",  "#8B1E2D", "#D9A9B0");
    makeCard("ren", "Service le plus rentable", "#1F3A4D", "#A9B8C4");
}

void MainWindow::updateChart()
{
    const QStringList types = {"Location", "Restauration", "Nettoyage", "Authentification"};
    struct Row { QString type; double val; };

    auto refresh = [&](const QString &prefix, bool profit) {
        auto *card = ui->frameStats->findChild<QFrame *>(prefix + "_card");
        if (!card) return;
        const QString main = card->property("main").toString();
        const QString tint = card->property("tint").toString();

        QVector<Row> rows;
        double total = 0;
        for (const QString &t : types) {
            double v = 0;
            for (const Service &s : std::as_const(m_services))
                if (s.type == t) v += profit ? (s.tarif - s.cout) : 1;
            rows.append({t, v});
            total += v;
        }

        std::sort(rows.begin(), rows.end(),
                  [](const Row &a, const Row &b) { return a.val > b.val; });

        const double mx = rows.first().val;
        card->findChild<QLabel *>(prefix + "_name")
            ->setText(mx > 0 ? rows.first().type : "—");
        card->findChild<QLabel *>(prefix + "_sub")->setText(
            profit ? QString("Bénéfice total : %1").arg(dt(mx))
                   : QString("%1 service(s) · %2 % des demandes")
                         .arg(int(mx)).arg(total > 0 ? qRound(mx * 100 / total) : 0));

        for (int i = 0; i < 4; ++i) {
            card->findChild<QLabel *>(QString("%1_lbl_%2").arg(prefix).arg(i))->setText(rows[i].type);
            card->findChild<QLabel *>(QString("%1_val_%2").arg(prefix).arg(i))
                ->setText(profit ? dt(rows[i].val) : QString::number(int(rows[i].val)));

            // la QProgressBar suit automatiquement la largeur de la carte
            auto *bar = card->findChild<QProgressBar *>(QString("%1_bar_%2").arg(prefix).arg(i));
            const int pv = (mx > 0 && rows[i].val > 0)
                               ? qMax(30, int(1000 * rows[i].val / mx)) : 0;
            bar->setValue(pv);
            bar->setStyleSheet(QString("QProgressBar{background:#F1ECE0; border:none; border-radius:7px;}"
                                       "QProgressBar::chunk{background:%1; border-radius:7px;}")
                                   .arg(i == 0 ? main : tint));
        }
    };

    refresh("dem", false);
    refresh("ren", true);
}

// ---------- Assemblage des layouts ----------
void MainWindow::setupLayout()
{
    // colonne centrale : en-tête, tableau, statistiques
    auto *mid = new QWidget(ui->contenuServices);
    auto *vm = new QVBoxLayout(mid);
    vm->setContentsMargins(14, 10, 14, 10);
    vm->setSpacing(8);

    // ligne 1 : titre + boutons d'action
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(10);
    auto *titres = new QVBoxLayout;
    titres->setSpacing(0);
    ui->lblTitle->setMinimumHeight(0);
    titres->addWidget(ui->lblTitle);
    titres->addWidget(lblSousTitre);
    row1->addLayout(titres, 1);
    for (QPushButton *b : {ui->btnAjouter, ui->btnModifier, ui->btnSupprimer, ui->btnNotifications}) {
        b->setMinimumSize(90, 32);
        row1->addWidget(b, 0, Qt::AlignTop);
    }
    vm->addLayout(row1);

    // ligne 2 : recherche + tri + export
    auto *row2 = new QHBoxLayout;
    row2->setSpacing(10);
    row2->addWidget(ui->txtRecherche, 3);
    row2->addWidget(ui->cmbTri, 2);
    row2->addWidget(btnExportPDF);
    vm->addLayout(row2);

    // ligne 3 : bannière d'alerte + rappels
    auto *row3 = new QHBoxLayout;
    row3->setSpacing(10);
    row3->addWidget(ui->lblAlerte, 1);
    row3->addWidget(btnRappels, 0, Qt::AlignVCenter);
    vm->addLayout(row3);

    // tableau : prend toute la hauteur restante
    ui->tableServices->setMinimumHeight(130);
    vm->addWidget(ui->tableServices, 1);
    vm->addWidget(lblCount);

    // statistiques : hauteur naturelle, largeur partagée entre les 2 cartes
    ui->frameStats->setMinimumHeight(270);
    vm->addWidget(ui->frameStats, 0);

    // panneau de droite : scrollable si la fenêtre est trop basse
    scrollDetails = new QScrollArea(ui->contenuServices);
    scrollDetails->setFrameShape(QFrame::NoFrame);
    scrollDetails->setWidgetResizable(true);
    scrollDetails->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollDetails->setWidget(ui->frameDetails);

    // zone de contenu : horizontale en grand, verticale en très petit (cf. applyResponsive)
    m_content = new QBoxLayout(QBoxLayout::LeftToRight);
    m_content->setSpacing(0);
    m_content->addWidget(mid, 3);
    m_content->addWidget(scrollDetails, 1);

    auto *main = new QHBoxLayout(ui->contenuServices);
    main->setContentsMargins(0, 0, 10, 0);
    main->setSpacing(0);
    main->addWidget(ui->frameSidebar);
    main->addLayout(m_content, 1);

    // filet de sécurité : si la fenêtre est plus petite que le contenu minimal,
    // des barres de défilement apparaissent au lieu de déformer l'interface
    // (le contenu du module Services est maintenant la page PServices du stackedWidget principal)
    auto *scroll = new QScrollArea(ui->PServices);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(ui->contenuServices);
    auto *pageServices = new QVBoxLayout(ui->PServices);
    pageServices->setContentsMargins(0, 0, 0, 0);
    pageServices->addWidget(scroll);

    applyResponsive();
}

// ---------- Adaptation selon la largeur de la fenêtre ----------
void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    applyResponsive();
}

void MainWindow::applyResponsive()
{
    if (!scrollDetails || !m_content) return;

    // 0 = large (plein écran), 1 = compact, 2 = très étroit (fiche sous le tableau)
    const int w = width();
    const int state = (w < 1150) ? 2 : (w < 1450) ? 1 : 0;
    if (state == m_layoutState) return;
    m_layoutState = state;

    const bool compact = (state >= 1);
    lblSousTitre->setVisible(!compact);
    QFont f = ui->lblTitle->font();
    f.setPointSize(compact ? 16 : 20);
    ui->lblTitle->setFont(f);

    if (state == 2) {
        m_content->setDirection(QBoxLayout::TopToBottom);
        m_content->setStretch(0, 1);
        m_content->setStretch(1, 0);
        scrollDetails->setMinimumWidth(0);
        scrollDetails->setMaximumWidth(QWIDGETSIZE_MAX);
        scrollDetails->setMinimumHeight(780);
        ui->tableServices->setMinimumHeight(260);
    } else {
        m_content->setDirection(QBoxLayout::LeftToRight);
        m_content->setStretch(0, 3);
        m_content->setStretch(1, 1);
        scrollDetails->setMinimumHeight(0);
        scrollDetails->setMinimumWidth(compact ? 400 : 450);
        scrollDetails->setMaximumWidth(compact ? 470 : 580);
        ui->tableServices->setMinimumHeight(130);
    }
}

// ---------- Détection des conflits de dates ----------
// Un service "Terminé" libère l'article : il n'entre pas en conflit.
QVector<int> MainWindow::trouverConflits(const Service &s, int ignore) const
{
    QVector<int> res;
    if (s.article.trimmed().isEmpty() || !s.debut.isValid() || !s.fin.isValid()) return res;
    for (int i = 0; i < m_services.size(); ++i) {
        if (i == ignore) continue;
        const Service &o = m_services[i];
        if (o.article != s.article || o.statut == "Terminé") continue;
        if (s.debut <= o.fin && o.debut <= s.fin) res << i;
    }
    return res;
}

void MainWindow::verifierConflit()
{
    if (!lblConflit) return;
    if (m_current < 0 || m_current >= m_services.size()) { lblConflit->hide(); return; }

    Service tmp = m_services[m_current];
    tmp.article = ui->cmbArticle->currentText();
    tmp.debut   = ui->dateDebutEdit->date();
    tmp.fin     = ui->dateFin->date();

    const QVector<int> c = trouverConflits(tmp, m_current);
    if (c.isEmpty()) { lblConflit->hide(); return; }

    QStringList ids;
    QDate libre = tmp.debut;
    for (int i : c) { ids << refService(m_services[i]); libre = qMax(libre, m_services[i].fin.addDays(1)); }
    lblConflit->setText(QString("⚠ Conflit : article déjà engagé dans %1. Prochaine disponibilité : %2")
                            .arg(ids.join(", "), libre.toString("dd/MM/yyyy")));
    lblConflit->show();
}

// ---------- Planning intelligent : calendrier coloré + conflits ----------
static QColor statutColor(const QString &st)
{
    if (st == "En retard") return QColor("#8B1E2D");
    if (st == "En cours")  return QColor("#1F4E8C");
    if (st == "Terminé")   return QColor("#1E6B4A");
    return QColor("#888888");
}

// Calendrier qui dessine, sous chaque jour, un point coloré par service actif
// et encadre en rouge les jours où un même article est engagé deux fois.
class PlanningCalendar : public QCalendarWidget
{
public:
    using QCalendarWidget::QCalendarWidget;
    QHash<QDate, QVector<QColor>> points;
    QSet<QDate> conflits;

protected:
    void paintCell(QPainter *p, const QRect &r, QDate d) const override
    {
        QCalendarWidget::paintCell(p, r, d);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (conflits.contains(d)) {
            p->setPen(QPen(QColor("#8B1E2D"), 2));
            p->setBrush(QColor(139, 30, 45, 40));
            p->drawRect(r.adjusted(1, 1, -2, -2));
        }
        const auto it = points.constFind(d);
        if (it != points.constEnd()) {
            p->setPen(Qt::NoPen);
            const int n = qMin(int(it->size()), 4);
            for (int i = 0; i < n; ++i) {
                p->setBrush(it->at(i));
                p->drawEllipse(QPoint(r.left() + 10 + i * 10, r.bottom() - 8), 4, 4);
            }
        }
        p->restore();
    }
};

void MainWindow::onPlanning()
{
    QDialog dlg(ui->PServices);   // garde la feuille de style de la page Services
    dlg.setWindowTitle("Planning intelligent des services");
    dlg.setStyleSheet("QDialog{background:#F5F1E8;}");
    dlg.resize(980, 600);
    dlg.setMinimumSize(640, 460);

    auto *cal = new PlanningCalendar;
    cal->setLocale(QLocale(QLocale::French));
    cal->setFirstDayOfWeek(Qt::Monday);
    cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    cal->setGridVisible(true);
    cal->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 1) un point coloré par service actif, chaque jour
    for (const Service &sv : std::as_const(m_services)) {
        if (!sv.debut.isValid() || !sv.fin.isValid()) continue;
        int garde = 0;
        for (QDate d = sv.debut; d <= sv.fin && garde < 800; d = d.addDays(1), ++garde)
            cal->points[d].append(statutColor(sv.statut));
    }

    // 2) conflits : même article, deux services non terminés qui se chevauchent
    QStringList resumes;
    QDate premier;
    for (int i = 0; i < m_services.size(); ++i)
        for (int j = i + 1; j < m_services.size(); ++j) {
            const Service &a = m_services[i], &b = m_services[j];
            if (a.article.isEmpty() || a.article != b.article) continue;
            if (a.statut == "Terminé" || b.statut == "Terminé") continue;
            const QDate dbt = qMax(a.debut, b.debut), fin = qMin(a.fin, b.fin);
            if (dbt > fin) continue;
            int garde = 0;
            for (QDate d = dbt; d <= fin && garde < 800; d = d.addDays(1), ++garde)
                cal->conflits.insert(d);
            if (!premier.isValid() || dbt < premier) premier = dbt;
            resumes << QString("• %1 ⟷ %2 · %3 · libre le %4")
                           .arg(refService(a), refService(b), a.article,
                                qMax(a.fin, b.fin).addDays(1).toString("dd/MM/yyyy"));
        }

    // panneau de droite
    auto *banner = new QLabel;
    banner->setWordWrap(true);
    if (resumes.isEmpty()) {
        banner->setText("✔ Aucun conflit : le planning est cohérent.");
        banner->setStyleSheet("background:#D9EEE3; color:#1E6B4A; border:1px solid #1E6B4A;"
                              "border-radius:6px; padding:8px; font-weight:bold;");
    } else {
        QStringList vus = resumes.mid(0, 3);
        if (resumes.size() > 3) vus << QString("… et %1 autre(s)").arg(resumes.size() - 3);
        banner->setText(QString("⚠ %1 conflit(s) détecté(s)\n%2").arg(resumes.size()).arg(vus.join("\n")));
        banner->setStyleSheet("background:#FBE9EC; color:#8B1E2D; border:1px solid #8B1E2D;"
                              "border-radius:6px; padding:8px; font-weight:bold;");
    }

    auto *legend = new QLabel(
        "<span style='color:#1F4E8C'>●</span> En cours &nbsp; "
        "<span style='color:#8B1E2D'>●</span> En retard &nbsp; "
        "<span style='color:#1E6B4A'>●</span> Terminé &nbsp; "
        "<span style='color:#888888'>●</span> En attente &nbsp; "
        "<span style='color:#8B1E2D'>▢</span> Conflit");
    legend->setWordWrap(true);

    auto *titreListe = new QLabel;
    titreListe->setStyleSheet("font-weight:bold;");
    titreListe->setWordWrap(true);

    auto *liste = new QListWidget;
    auto *btnFermer = new QPushButton("Fermer");
    btnFermer->setMinimumHeight(32);
    connect(btnFermer, &QPushButton::clicked, &dlg, &QDialog::accept);

    // liste des services du jour sélectionné
    auto refresh = [this, cal, liste, titreListe]() {
        const QDate d = cal->selectedDate();
        titreListe->setText("Services du " + QLocale(QLocale::French).toString(d, "dddd d MMMM yyyy"));
        liste->clear();
        for (int i = 0; i < m_services.size(); ++i) {
            const Service &sv = m_services[i];
            if (!sv.debut.isValid() || !sv.fin.isValid() || d < sv.debut || d > sv.fin) continue;
            bool conf = false;
            if (sv.statut != "Terminé" && !sv.article.isEmpty())
                for (int j = 0; j < m_services.size() && !conf; ++j) {
                    const Service &o = m_services[j];
                    conf = (j != i && o.article == sv.article && o.statut != "Terminé"
                            && o.debut <= d && d <= o.fin);
                }
            auto *it = new QListWidgetItem(QString("%1%2 · %3 · %4")
                                               .arg(conf ? "⚠ " : "", refService(sv), sv.article, sv.statut));
            it->setForeground(conf ? QColor("#8B1E2D") : statutColor(sv.statut));
            it->setData(Qt::UserRole, i);
            liste->addItem(it);
        }
        if (liste->count() == 0) {
            auto *vide = new QListWidgetItem("Aucun service ce jour");
            vide->setFlags(Qt::NoItemFlags);
            liste->addItem(vide);
        }
    };
    connect(cal, &QCalendarWidget::selectionChanged, &dlg, refresh);

    // double-clic : ouvrir le service dans le tableau principal
    connect(liste, &QListWidget::itemDoubleClicked, &dlg, [this, &dlg](QListWidgetItem *it) {
        if (!it->data(Qt::UserRole).isValid()) return;
        ui->tableServices->selectRow(it->data(Qt::UserRole).toInt());
        dlg.accept();
    });

    auto *root = new QHBoxLayout(&dlg);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(14);
    root->addWidget(cal, 3);
    auto *side = new QVBoxLayout;
    side->setSpacing(8);
    side->addWidget(banner);
    side->addWidget(legend);
    side->addWidget(titreListe);
    side->addWidget(liste, 1);
    side->addWidget(btnFermer);
    root->addLayout(side, 2);

    // on ouvre directement sur le premier conflit, sinon sur aujourd'hui
    cal->setSelectedDate(premier.isValid() ? premier : QDate::currentDate());
    refresh();
    dlg.exec();
}

// ---------- Barre d'état ----------
void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("● Base Oracle : connectée   ·   Rôle : Responsable des services");
    auto *l = new QLabel(QLocale(QLocale::French).toString(QDate::currentDate(), "dddd d MMMM yyyy")
                         + "   ·   Atelier Vintage Couture");
    statusBar()->addPermanentWidget(l);
}

// ---------- Export PDF, reçu, rappels ----------
void MainWindow::exportPdf()
{
    QString file = QFileDialog::getSaveFileName(this, "Export PDF", "services.pdf", "PDF (*.pdf)");
    if (file.isEmpty()) return;

    QString html = "<h2>Atelier Vintage Couture — Services</h2>"
                   "<table border='1' cellspacing='0' cellpadding='4' width='100%'>"
                   "<tr bgcolor='#1F3A4D'>";
    for (int c = 0; c < ui->tableServices->columnCount(); ++c)
        html += "<th><font color='white'>" + ui->tableServices->horizontalHeaderItem(c)->text() + "</font></th>";
    html += "</tr>";

    for (int r = 0; r < ui->tableServices->rowCount(); ++r) {
        if (ui->tableServices->isRowHidden(r)) continue;
        html += "<tr>";
        for (int c = 0; c < ui->tableServices->columnCount(); ++c)
            html += "<td>" + ui->tableServices->item(r, c)->text().toHtmlEscaped() + "</td>";
        html += "</tr>";
    }
    html += "</table>";

    QPdfWriter w(file);
    w.setPageSize(QPageSize(QPageSize::A4));
    w.setPageOrientation(QPageLayout::Landscape);
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&w);
    statusBar()->showMessage("PDF exporté : " + file, 4000);
}

void MainWindow::genererRecu()
{
    if (m_current < 0) return;
    const Service &s = m_services[m_current];
    QString file = QFileDialog::getSaveFileName(this, "Reçu de service", "recu_" + s.reference + ".pdf", "PDF (*.pdf)");
    if (file.isEmpty()) return;

    QString html = QString("<h2>Atelier Vintage Couture</h2><h3>Reçu de service %1</h3>"
                           "<p>Article : %2<br>Client : %3<br>Type : %4<br>Fin prévue : %5</p>"
                           "<table border='1' cellspacing='0' cellpadding='6'>"
                           "<tr><td>Tarif</td><td>%6</td></tr>"
                           "<tr><td>Acompte versé</td><td>%7</td></tr>"
                           "<tr><td><b>Solde restant</b></td><td><b>%8</b></td></tr></table>")
                       .arg(refService(s), s.article, s.client.isEmpty() ? "—" : s.client, s.type,
                            s.fin.toString("dd/MM/yyyy"), dt(s.tarif), dt(s.acompte), dt(s.tarif - s.acompte));

    QPdfWriter w(file);
    w.setPageSize(QPageSize(QPageSize::A5));
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&w);
    txtAlertes->append("● Reçu généré pour " + refService(s));
}

void MainWindow::envoyerRappels()
{
    int n = 0;
    for (const Service &s : std::as_const(m_services))
        if (s.statut == "En retard") {
            txtAlertes->append(QString("● Rappel envoyé à %1 — %2 en retard.").arg(s.employe, refService(s)));
            ++n;
        }
    // TODO : envoi réel par e-mail (SMTP) ici
    QMessageBox::information(this, "Rappels", QString("%1 rappel(s) envoyé(s).").arg(n));
}

// ---------- Ajouter / Modifier / Supprimer ----------
void MainWindow::onAjouter()
{
    Service s;               // id vide : l'ID Oracle est généré à l'enregistrement
    s.type    = "Location";
    s.employe = cmbEmploye->itemText(0);
    s.debut   = QDate::currentDate();
    s.fin     = QDate::currentDate().addDays(7);
    s.tarif = s.cout = s.acompte = 0;
    s.statut  = "En attente";

    m_services.append(s);
    int row = ui->tableServices->rowCount();
    ui->tableServices->setRowCount(row + 1);
    fillRow(row);
    ui->tableServices->selectRow(row);
    ui->tableServices->scrollToBottom();
    updateChart();
    updateCount();
    ui->cmbArticle->setFocus();
    statusBar()->showMessage("Nouveau service : remplissez la fiche puis cliquez sur Enregistrer", 6000);
}

void MainWindow::onModifier()
{
    if (m_current < 0) {
        QMessageBox::information(this, "Modifier", "Sélectionnez d'abord un service dans le tableau.");
        return;
    }
    ui->cmbArticle->setFocus();
    statusBar()->showMessage("Modifiez la fiche à droite puis cliquez sur Enregistrer", 5000);
}

void MainWindow::onSupprimer()
{
    if (m_current < 0) {
        QMessageBox::information(this, "Supprimer", "Sélectionnez d'abord un service dans le tableau.");
        return;
    }
    if (QMessageBox::question(this, "Supprimer", "Supprimer ce service ?") != QMessageBox::Yes)
        return;

    // Base d'abord (un service jamais enregistré n'a pas d'ID : rien à supprimer dans Oracle)
    const QString id = m_services[m_current].id;
    if (!id.isEmpty()) {
        Service svc;
        if (!svc.supprimer(id)) {
            QMessageBox::critical(this, "Oracle",
                                  "Échec de la suppression dans la base.\nDes articles ou des employés sont peut-être encore liés à ce service.\nVoir la console pour le détail.");
            return;
        }
    }

    int row = m_current;
    {
        QSignalBlocker blocker(ui->tableServices);
        m_services.remove(row);
        ui->tableServices->removeRow(row);
    }
    m_current = -1;
    if (!m_services.isEmpty())
        ui->tableServices->selectRow(qMin(row, int(m_services.size()) - 1));

    updateChart();
    updateBanner();
    updateCount();
    statusBar()->showMessage("Service supprimé", 4000);
}
// === FIN MODULE SERVICE ===


// ====================================================================================
// === MODULE EMPLOYE ===
// ====================================================================================
// Note : plus aucune requete SQL ni acces direct a la base dans cette classe (Vue) --
// tout est delegue aux classes Modele Employe / PosteTravail, cf. consigne de l'atelier
// CRUD du tuteur ("Aucune requete SQL... en dehors de la classe C++").

void MainWindow::initialiserModuleEmploye()
{
    QPixmap pixmap(":/images/logo.png");
    ui->labelLogoEmpl->setPixmap(pixmap.scaled(
        ui->labelLogoEmpl->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        ));
    connect(ui->btnAjouterEmploye, &QPushButton::clicked, this, &MainWindow::onAjouterEmployeClicked);
    connect(ui->btnModifierEmploye, &QPushButton::clicked, this, &MainWindow::onModifierEmployeClicked);
    connect(ui->btnSupprimerEmploye, &QPushButton::clicked, this, &MainWindow::onSupprimerEmployeClicked);
    connect(ui->btnEnregistrerEmploye, &QPushButton::clicked, this, &MainWindow::onEnregistrerEmployeClicked);
    connect(ui->btnAnnulerEmploye, &QPushButton::clicked, this, &MainWindow::onAnnulerEmployeClicked);
    connect(ui->tableEmployes, &QTableWidget::cellClicked, this, &MainWindow::onLigneEmployeSelectionnee);
    connect(ui->champRecherche, &QLineEdit::textChanged, this, &MainWindow::onRechercheEmployeChanged);
    connect(ui->btnDeconnexionEmpl, &QPushButton::clicked, this, &MainWindow::deconnexionDemandee);

    ui->champMotDePasse->setEchoMode(QLineEdit::Password);

    viderFormulaireEmploye();
    chargerEmployesDepuisBase(); // charge les employes existants depuis Oracle (construit aussi le graphique)

    // Plan de l'atelier : un seul tableau de bord, en lecture seule pour le gerant, avec
    // "Prendre / Liberer mon poste" pour les autres employes.
    m_planPostesGerant = new PlanPostes(false);
    m_planPostesGerant->setDefilementInterne(false); // la page Gerant defile deja en entier
    m_planPostesEmploye = new PlanPostes(true);
    m_planPostesEmploye->setDefilementInterne(false); // pas de defilement : le plan prend la hauteur de ses cartes
    connect(m_planPostesEmploye, &PlanPostes::changerPosteDemande, this, &MainWindow::changerMonPosteTravail);
    adapterMiseEnPageEmploye();
    configurerFormulaireEmploye();
    connect(ui->btnExporterDocument, &QPushButton::clicked, this, &MainWindow::onExporterDocumentRHClicked);

    chargerPostesTravailDepuisBase();
    remplirComboPostesTravail();
    construirePlanDisponibilitePostes();

    // Plusieurs instances de l'application (un employe par poste) partagent la meme base :
    // le plan se met a jour tout seul toutes les 15 s, en plus du rafraichissement fait
    // juste apres chaque ajout/modification/suppression locale. Parente a "this" : le timer
    // est detruit avec la fenetre.
    m_timerPostes = new QTimer(this);
    m_timerPostes->setInterval(15000);
    connect(m_timerPostes, &QTimer::timeout, this, &MainWindow::rafraichirPostesTravail);
    m_timerPostes->start();

    ui->tableEmployes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    appliquerRestrictionsSelonPosteEmploye();
}

void MainWindow::appliquerRestrictionsSelonPosteEmploye()
{
    // 2 pages dans "SMEmpl" :
    //  - index 0 ("pageGestionEmpl") : gestion complete des employes -> reservee au Gerant
    //  - index 1 ("pageEspaceEmpl") : espace personnel generique (Couturier, Vendeur, Styliste)
    const QString poste = m_employeConnecte.poste();

    const bool estGerant = poste.compare("Gérant", Qt::CaseInsensitive) == 0
                            || poste.compare("Gerant", Qt::CaseInsensitive) == 0;

    // Affiche dans la barre laterale, pour tout le monde (remplace le "Mouna Ayari" fige du .ui)
    ui->labelUtilisateurEmpl->setText(m_employeConnecte.prenom() + " " + m_employeConnecte.nom()
                                   + "\n" + poste);

    if (estGerant) {
        ui->SMEmpl->setCurrentIndex(0);
        ui->labelTitreEmpl->setText("Gestion des employés");
        ui->labelTitreEmpl->setVisible(false);   // le titre est deja dans la carte "Gestion des employés"
        return;
    }

    // Couturier / Vendeur / Styliste : espace personnel generique.
    // Seul le Gerant peut aller vers les autres modules : on masque ces boutons du menu lateral.
    ui->navArticlesEmpl->hide();
    ui->navServicesEmpl->hide();
    ui->navClientsEmpl->hide();
    ui->navEvenementsEmpl->hide();
    ui->navMatieresEmpl->hide();
    ui->SMEmpl->setCurrentIndex(1);
    ui->labelTitreEmpl->setText("Mon espace — " + poste);

    // "Mon profil" : affichage en lecture seule des infos de l'employe connecte. On
    // s'assure que son poste exact est bien dans la liste du combo avant de le
    // selectionner (la liste pre-remplie dans le .ui peut differer legerement,
    // ex. anciennes donnees "Couturier").
    ui->lineProfilNomEmpl->setText(m_employeConnecte.prenom() + " " + m_employeConnecte.nom());
    ui->lineProfilNomEmpl->setReadOnly(true);
    if (ui->comboProfilPosteEmpl->findText(poste) < 0)
        ui->comboProfilPosteEmpl->addItem(poste);
    ui->comboProfilPosteEmpl->setCurrentText(poste);
    ui->comboProfilPosteEmpl->setEnabled(false);
    ui->lineProfilEmailEmpl->setText(m_employeConnecte.email());
    ui->lineProfilEmailEmpl->setToolTip(m_employeConnecte.email());
    ui->lineProfilEmailEmpl->setReadOnly(true);
    ui->lineProfilTelEmpl->setText(m_employeConnecte.telephone());
    ui->lineProfilTelEmpl->setReadOnly(true);

    // Mes documents RH : conge par defaut du lendemain a +4 jours.
    ui->dateDebutConge->setMinimumDate(QDate::currentDate());
    ui->dateDebutConge->setDate(QDate::currentDate().addDays(1));
    ui->dateFinConge->setMinimumDate(QDate::currentDate());
    ui->dateFinConge->setDate(QDate::currentDate().addDays(5));

    construireRadarStatistiques();
}

void MainWindow::construireGraphiqueStatistiquesEmploye()
{
    // Repartition des employes par poste (role), calculee sur la liste chargee depuis la
    // table EMPLOYE : une part par poste autorise, dans l'ordre des postes de la base.
    if (!m_repartition)
        return;   // la page n'est pas encore construite (constructeur) : adapterMiseEnPageEmploye() le rappelle

    QMap<QString, int> nombreParPoste;
    for (const Employe &e : m_employes)
        nombreParPoste[e.poste()]++;

    QList<QPair<QString, int>> parts;
    for (const QString &poste : Employe::postes())
        parts.append({poste, nombreParPoste.take(poste)});
    for (auto it = nombreParPoste.constBegin(); it != nombreParPoste.constEnd(); ++it)   // ancienne valeur hors liste
        parts.append({it.key().isEmpty() ? QString("Non renseigné") : it.key(), it.value()});
    m_repartition->setDonnees(parts);
}

void MainWindow::chargerEmployesDepuisBase()
{
    QString erreur;
    m_employes = Employe::chargerTous(&erreur);
    if (!erreur.isEmpty()) {
        QMessageBox::warning(this, "Chargement",
                              "Impossible de charger les employés : " + erreur);
    }

    actualiserTableauEmployes();
    construireGraphiqueStatistiquesEmploye();
}

void MainWindow::actualiserTableauEmployes()
{
    ui->tableEmployes->setRowCount(0);
    for (const Employe &e : m_employes) {
        int row = ui->tableEmployes->rowCount();
        ui->tableEmployes->insertRow(row);
        ui->tableEmployes->setItem(row, 0, new QTableWidgetItem(e.nom()));
        ui->tableEmployes->setItem(row, 1, new QTableWidgetItem(e.prenom()));
        ui->tableEmployes->setItem(row, 2, new QTableWidgetItem(e.poste()));
        ui->tableEmployes->setItem(row, 3, new QTableWidgetItem(e.email()));
        ui->tableEmployes->setItem(row, 4, new QTableWidgetItem(e.telephone()));
        ui->tableEmployes->setItem(row, 5, new QTableWidgetItem(e.statut()));
    }
}

void MainWindow::viderFormulaireEmploye()
{
    ui->champNom->clear();
    ui->champPrenom->clear();
    ui->comboPoste->setCurrentIndex(-1);   // "Choisir..." : rien n'est choisi au depart
    ui->comboStatut->setCurrentIndex(-1);
    ui->comboPosteTravail->setCurrentIndex(0); // "Aucun"
    if (m_comboServiceEmploye)
        m_comboServiceEmploye->setCurrentIndex(-1);
    ui->champEmail->clear();
    ui->champTelephone->clear();
    ui->champMotDePasse->clear();
    m_ligneEmployeSelectionnee = -1;
    m_modeModificationEmploye = false;
    ui->tableEmployes->clearSelection();

    // formulaire vierge : pas de message d'erreur affiche, mais Enregistrer reste desactive
    // tant que les champs obligatoires ne sont pas remplis
    if (m_validationEmploye) {
        m_validationEmploye->effacerMessages();
        m_validationEmploye->evaluer();
    }
}

void MainWindow::chargerFormulaireDepuisEmploye(const Employe &e)
{
    ui->champNom->setText(e.nom());
    ui->champPrenom->setText(e.prenom());
    ui->comboPoste->setCurrentIndex(ui->comboPoste->findData(e.poste()));     // -1 si valeur inconnue
    ui->comboStatut->setCurrentIndex(ui->comboStatut->findData(e.statut()));
    const int indexPoste = ui->comboPosteTravail->findData(e.posteTravail());
    ui->comboPosteTravail->setCurrentIndex(indexPoste >= 0 ? indexPoste : 0);
    m_comboServiceEmploye->setCurrentIndex(m_comboServiceEmploye->findData(QString::number(e.idService())));
    ui->champEmail->setText(e.email());
    ui->champTelephone->setText(e.telephone());
    ui->champMotDePasse->clear(); // on ne réaffiche jamais un mot de passe existant
}

void MainWindow::onLigneEmployeSelectionnee(int row, int /*column*/)
{
    m_ligneEmployeSelectionnee = row;
}

void MainWindow::onAjouterEmployeClicked()
{
    viderFormulaireEmploye();
    ui->champNom->setFocus();
}

void MainWindow::onModifierEmployeClicked()
{
    if (m_ligneEmployeSelectionnee < 0 || m_ligneEmployeSelectionnee >= m_employes.size()) {
        QMessageBox::warning(this, "Modifier", "Sélectionne d'abord un employé dans le tableau.");
        return;
    }
    chargerFormulaireDepuisEmploye(m_employes.at(m_ligneEmployeSelectionnee));
    m_modeModificationEmploye = true;   // le mot de passe devient facultatif (vide = on garde l'ancien)
    m_validationEmploye->revalider();   // signale tout de suite une ancienne donnee devenue invalide
}

void MainWindow::onSupprimerEmployeClicked()
{
    if (m_ligneEmployeSelectionnee < 0 || m_ligneEmployeSelectionnee >= m_employes.size()) {
        QMessageBox::warning(this, "Supprimer", "Sélectionne d'abord un employé dans le tableau.");
        return;
    }

    Employe e = m_employes.at(m_ligneEmployeSelectionnee);
    auto reponse = QMessageBox::question(this, "Confirmer",
                                         "Supprimer l'employé « " + e.prenom() + " " + e.nom() + " » ?");
    if (reponse != QMessageBox::Yes)
        return;

    QString erreur;
    if (!e.supprimer(&erreur)) {
        QMessageBox::warning(this, "Supprimer", "Échec de la suppression : " + erreur);
        return;
    }

    chargerEmployesDepuisBase();
    rafraichirPostesTravail(); // un employe supprime libere son poste
    viderFormulaireEmploye();
}

void MainWindow::onEnregistrerEmployeClicked()
{
    // Normalement le bouton est desactive tant que la fiche est invalide ; on revalide quand
    // meme (touche Entree, etat change entre-temps) et on place le curseur sur le premier
    // champ en erreur.
    if (!m_validationEmploye->revalider()) {
        if (QWidget *fautif = m_validationEmploye->premierEnErreur())
            fautif->setFocus();
        return;
    }

    const QString nom = ui->champNom->text().trimmed();
    const QString prenom = ui->champPrenom->text().trimmed();
    const QString poste = ui->comboPoste->currentData().toString();
    const QString statut = ui->comboStatut->currentData().toString();
    const QString email = ui->champEmail->text().trimmed();
    const QString telephone = ui->champTelephone->text().trimmed();
    const QString motDePasse = ui->champMotDePasse->text(); // en clair : Employe se charge du hachage
    const QString posteTravail = ui->comboPosteTravail->currentData().toString(); // "" = "Aucun"
    const int idService = m_comboServiceEmploye->currentData().toString().toInt();       // 0 = rien de choisi

    QString erreur;
    QString champ;   // champ concerne par l'erreur, quand le Modele le sait
    bool succes;

    if (m_modeModificationEmploye && m_ligneEmployeSelectionnee >= 0 && m_ligneEmployeSelectionnee < m_employes.size()) {
        // --- MODIFICATION d'un employe existant (UPDATE) ---
        int id = m_employes.at(m_ligneEmployeSelectionnee).id();
        Employe e(id, nom, prenom, poste, email, telephone, statut, motDePasse, posteTravail, idService);
        succes = e.modifier(&erreur, &champ);
    } else {
        // --- AJOUT d'un nouvel employe (INSERT), l'ID vient de la sequence Oracle ---
        Employe e(-1, nom, prenom, poste, email, telephone, statut, motDePasse, posteTravail, idService);
        succes = e.ajouter(&erreur, &champ);
    }

    if (!succes) {
        afficherErreurModeleEmploye(champ, erreur);
        return;
    }

    chargerEmployesDepuisBase(); // on recharge tout depuis Oracle pour rester synchronise
    rafraichirPostesTravail();   // l'affectation a pu changer le plan
    viderFormulaireEmploye();
}

void MainWindow::afficherErreurModeleEmploye(const QString &champ, const QString &message)
{
    // Le Modele renvoie un message lisible (jamais d'erreur Oracle brute) et, s'il le sait, le
    // champ concerne : on l'affiche sous ce champ plutot que dans une fenetre.
    QWidget *cible = nullptr;
    if (champ == "nom")               cible = ui->champNom;
    else if (champ == "prenom")       cible = ui->champPrenom;
    else if (champ == "poste")        cible = ui->comboPoste;
    else if (champ == "statut")       cible = ui->comboStatut;
    else if (champ == "email")        cible = ui->champEmail;
    else if (champ == "telephone")    cible = ui->champTelephone;
    else if (champ == "posteTravail") cible = ui->comboPosteTravail;
    else if (champ == "service")      cible = m_comboServiceEmploye;
    else if (champ == "motDePasse")   cible = ui->champMotDePasse;

    if (cible) {
        m_validationEmploye->afficherErreur(cible, message);
        cible->setFocus();
    } else {
        QMessageBox::warning(this, "Enregistrer", message);
    }
}

void MainWindow::configurerFormulaireEmploye()
{
    // Longueurs alignees sur les colonnes de la table EMPLOYE
    ui->champNom->setMaxLength(100);
    ui->champPrenom->setMaxLength(100);
    ui->champEmail->setMaxLength(100);
    ui->champTelephone->setMaxLength(8);
    ui->champMotDePasse->setMaxLength(100);

    // Listes : valeurs exactement egales aux contraintes CHECK de la base ; rien n'est
    // choisi au depart ("Choisir...").
    ui->comboPoste->clear();
    for (const QString &poste : Employe::postes())
        ui->comboPoste->addItem(poste, poste);
    ui->comboStatut->clear();
    ui->comboStatut->addItem("Actif", "Actif");
    ui->comboStatut->addItem("Inactif", "Inactif");
    ui->comboStatut->addItem("Congé", "Conge");   // affiche avec accent, enregistre sans (comme le CHECK)
    ui->comboPoste->setPlaceholderText("Choisir...");
    ui->comboStatut->setPlaceholderText("Choisir...");

    m_validationEmploye = new ValidationFormulaire(this);

    // Les regles vivent dans le Modele (Employe::...Valide) : la Vue les branche seulement.
    m_validationEmploye->attacherChamp(ui->champNom, [](const QString &v, QString *e) { return Employe::nomValide(v, e); });
    m_validationEmploye->attacherChamp(ui->champPrenom, [](const QString &v, QString *e) { return Employe::prenomValide(v, e); });

    // chaque liste a sa propre cellule : son message se place juste sous elle
    QLabel *erreurPoste = ValidationFormulaire::creerEtiquette(ui->groupFiche);
    QLabel *erreurStatut = ValidationFormulaire::creerEtiquette(ui->groupFiche);
    ValidationFormulaire::placerSous(ui->comboPoste, erreurPoste);
    ValidationFormulaire::placerSous(ui->comboStatut, erreurStatut);
    m_validationEmploye->attacherCombo(ui->comboPoste, [](const QString &v, QString *e) { return Employe::posteValide(v, e); }, erreurPoste);
    m_validationEmploye->attacherCombo(ui->comboStatut, [](const QString &v, QString *e) { return Employe::statutValide(v, e); }, erreurStatut);

    m_validationEmploye->attacherChamp(ui->champEmail, [](const QString &v, QString *e) { return Employe::emailValide(v, e); });
    m_validationEmploye->attacherChamp(ui->champTelephone, [](const QString &v, QString *e) { return Employe::telephoneValide(v, e); });

    QLabel *erreurService = ValidationFormulaire::creerEtiquette(ui->groupFiche);
    ValidationFormulaire::placerSous(m_comboServiceEmploye, erreurService);
    m_comboServiceEmploye->setPlaceholderText("Choisir...");
    remplirComboServicesEmploye();
    m_validationEmploye->attacherCombo(m_comboServiceEmploye, [](const QString &v, QString *e) { return Employe::serviceValide(v, e); }, erreurService);

    QLabel *erreurPosteTravail = ValidationFormulaire::creerEtiquette(ui->groupFiche);
    ValidationFormulaire::placerSous(ui->comboPosteTravail, erreurPosteTravail);
    m_validationEmploye->attacherCombo(ui->comboPosteTravail, [](const QString &v, QString *e) { return Employe::posteTravailValide(v, e); }, erreurPosteTravail);

    // en modification, un mot de passe vide est accepte (on garde l'ancien)
    m_validationEmploye->attacherChamp(ui->champMotDePasse, [this](const QString &v, QString *e) {
        return Employe::motDePasseValide(v, e, !m_modeModificationEmploye);
    });

    // Enregistrer n'est actif que lorsque tous les champs sont valides
    connect(m_validationEmploye, &ValidationFormulaire::etatChange, ui->btnEnregistrerEmploye, &QPushButton::setEnabled);
    viderFormulaireEmploye();
}

void MainWindow::onAnnulerEmployeClicked()
{
    viderFormulaireEmploye();
}

void MainWindow::onRechercheEmployeChanged(const QString &texte)
{
    QString type = ui->comboTriEmploye->currentText(); // "Nom" ou "Poste"
    int colonne = (type == "Poste") ? 2 : 0;
    for (int row = 0; row < ui->tableEmployes->rowCount(); ++row) {
        auto *item = ui->tableEmployes->item(row, colonne);
        bool correspond = texte.isEmpty() || (item && item->text().contains(texte, Qt::CaseInsensitive));
        ui->tableEmployes->setRowHidden(row, !correspond);
    }
}

// --- Plan de disponibilite des postes de travail ---------------------------------
//
// Nouvelle fonctionnalite : chaque poste de travail (table POSTE_TRAVAIL) est libre
// ou occupe selon qu'un employe a ce moment-la son EMPLOYE.POSTE_TRAVAIL egal a ce poste
// dessus (contrainte UNIQUE cote base : un poste ne peut avoir qu'un seul occupant a
// la fois). Reutilise l'emplacement et le groupBox de l'ancienne "File d'attente
// intelligente" (retiree sur demande de la prof, jugee trop basique).

void MainWindow::chargerPostesTravailDepuisBase()
{
    // Les 6 postes sont une liste fixe (PosteTravail::tous()) ; seule la table EMPLOYE dit
    // qui occupe quoi : un poste absent de ce resultat est libre.
    QString erreur;
    m_occupants = Employe::occupantsParPoste(&erreur);
    if (!erreur.isEmpty() && !m_erreurPostesSignalee) {
        m_erreurPostesSignalee = true; // le timer rappelle cette methode : un seul avertissement
        QMessageBox::warning(this, "Postes de travail",
                              "Impossible de lire les postes de travail : " + erreur);
    }

    // Le poste de l'employe connecte a pu etre change ailleurs (par le gerant, ou depuis
    // une autre instance de l'application) : on le relit pour que les boutons "Prendre /
    // Liberer mon poste" restent justes.
    QString monPoste;
    for (auto it = m_occupants.constBegin(); it != m_occupants.constEnd(); ++it) {
        if (it.value().id() == m_employeConnecte.id()) {
            monPoste = it.key();
            break;
        }
    }
    m_employeConnecte.setPosteTravail(monPoste);
}

void MainWindow::remplirComboServicesEmploye()
{
    // La liste vient de la table SERVICE, via le Modele : "ID - TYPE_SERVICE" s'affiche, l'ID est memorise.
    const QString selection = m_comboServiceEmploye->currentData().toString();
    QString erreur;
    QSqlQueryModel *services = Employe::listerServices(&erreur);
    m_comboServiceEmploye->clear();
    for (int i = 0; i < services->rowCount(); ++i)
    {
        const QString id = services->data(services->index(i, 0)).toString();
        const QString type = services->data(services->index(i, 1)).toString();
        m_comboServiceEmploye->addItem(id + " - " + type, id);   // affiche "1 - Nettoyage", memorise l'ID
    }
    delete services;
    m_comboServiceEmploye->setCurrentIndex(m_comboServiceEmploye->findData(selection));   // -1 = "Choisir..."
    if (!erreur.isEmpty())
        QMessageBox::warning(this, "Services", "Impossible de lire les services : " + erreur);
}

void MainWindow::remplirComboPostesTravail()
{
    // Conserve la selection en cours (le plan se rafraichit pendant qu'on edite le formulaire).
    const QString selection = ui->comboPosteTravail->currentData().toString();

    ui->comboPosteTravail->clear();
    ui->comboPosteTravail->addItem("Aucun", QString());
    for (const QString &poste : PosteTravail::tous()) {
        const bool occupe = m_occupants.contains(poste);
        const Employe occupant = m_occupants.value(poste);
        ui->comboPosteTravail->addItem(
            PosteTravail::libelle(poste)
                + (occupe ? " (occupé : " + occupant.prenom() + " " + occupant.nom() + ")" : QString()),
            poste);
    }

    const int index = ui->comboPosteTravail->findData(selection);
    ui->comboPosteTravail->setCurrentIndex(index >= 0 ? index : 0);
}

void MainWindow::rafraichirPostesTravail()
{
    chargerPostesTravailDepuisBase();
    remplirComboPostesTravail();
    construirePlanDisponibilitePostes();
}

void MainWindow::construirePlanDisponibilitePostes()
{
    // Les deux tableaux de bord (gerant / employe) recoivent les memes donnees ; PlanPostes
    // ne reconstruit ses cartes que si quelque chose a change.
    m_planPostesGerant->setDonnees(PosteTravail::tous(), m_occupants, QString());
    m_planPostesEmploye->setDonnees(PosteTravail::tous(), m_occupants, m_employeConnecte.posteTravail());
}

// Carte de section : fond creme arrondi, titre en Playfair suivi d'un filet dore, puis le contenu.
// Libelle + champ empiles dans une cellule (le message de validation vient se glisser dessous).
static QWidget *creerCellule(QLabel *libelle, QWidget *champ)
{
    auto *cellule = new QWidget();
    auto *v = new QVBoxLayout(cellule);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(4);
    v->addWidget(libelle);
    v->addWidget(champ);
    return cellule;
}

// "compact" : marges et titre reduits, pour les cartes qui doivent rester basses.
// "icone" : pictogramme optionnel affiche a gauche du titre.
static QFrame *creerCarteSection(const QString &titre, QWidget *contenu, bool compact = false,
                                 const Icone *icone = nullptr)
{
    auto *carte = new QFrame();
    carte->setObjectName("carteSection");
    carte->setStyleSheet("QFrame#carteSection { background-color: #FFFDF9; border: 1px solid #EADFD0; border-radius: 14px; }");
    auto *colonne = new QVBoxLayout(carte);
    colonne->setContentsMargins(20, compact ? 10 : 16, 20, compact ? 12 : 18);
    colonne->setSpacing(compact ? 6 : 12);

    auto *entete = new QHBoxLayout();
    entete->setSpacing(14);
    auto *libelle = new QLabel(titre);
    libelle->setStyleSheet(QString("font-family: 'Playfair Display'; font-weight: bold; font-size: %1pt; "
                           "color: #1F3A4D; background: transparent; border: none;").arg(compact ? 13 : 14));
    auto *filet = new QFrame();
    filet->setFixedHeight(1);
    filet->setStyleSheet("background-color: #E2D5BF; border: none;");
    if (icone) {
        auto *pastille = new QLabel();
        pastille->setFixedSize(30, 30);
        pastille->setAlignment(Qt::AlignCenter);
        pastille->setPixmap(pixmapIcone(*icone, QColor("#8B1E2F"), 18));
        pastille->setStyleSheet("background-color: #F6EDE4; border: none; border-radius: 15px;");
        entete->setSpacing(10);
        entete->addWidget(pastille);
    }
    entete->addWidget(libelle);
    entete->addWidget(filet, 1, Qt::AlignVCenter);
    colonne->addLayout(entete);
    colonne->addWidget(contenu, 1);
    return carte;
}

void MainWindow::adapterMiseEnPageEmploye()
{
    // Le .ui place tout en positions absolues pour une fenetre de 1499 x 872 : trop grand
    // pour beaucoup d'ecrans (contenu coupe en bas) et avec des vides en plein ecran. On
    // remplace ces positions par des layouts qui suivent la taille de la fenetre.
    // (taille minimale de la fenetre : geree par MainWindow)
    ui->menubar->hide();

    QWidget *barreRecherche = ui->champRecherche->parentWidget();
    QWidget *navigation = ui->navArticlesEmpl->parentWidget();

    // --- barre laterale : logo, navigation, puis utilisateur et deconnexion en bas ---
    ui->sidebarEmpl->setFixedWidth(281);
    auto *lateral = new QVBoxLayout(ui->sidebarEmpl);
    lateral->setContentsMargins(25, 28, 25, 28);
    lateral->setSpacing(18);
    ui->labelLogoEmpl->setFixedSize(231, 101);
    lateral->addWidget(ui->labelLogoEmpl, 0, Qt::AlignHCenter);
    lateral->addSpacing(10);
    navigation->setFixedHeight(360);   // 6 boutons (au lieu de 4 : + Evenements, Matieres) ; 60 px par bouton pour tenir sur un ecran 768 px
    lateral->addWidget(navigation);
    lateral->addStretch();
    lateral->addWidget(ui->labelUtilisateurEmpl);
    lateral->addWidget(ui->btnDeconnexionEmpl);

    // --- fenetre : barre laterale a gauche, titre + pages a droite ---
    auto *principal = new QHBoxLayout(ui->PEmpl);
    principal->setContentsMargins(0, 0, 0, 0);
    principal->setSpacing(0);
    auto *zoneDroite = new QWidget(ui->PEmpl);
    auto *droite = new QVBoxLayout(zoneDroite);
    droite->setContentsMargins(28, 22, 28, 18);
    droite->setSpacing(10);
    ui->labelTitreEmpl->setMinimumHeight(44);
    droite->addWidget(ui->labelTitreEmpl);
    droite->addWidget(ui->SMEmpl, 1);
    principal->addWidget(ui->sidebarEmpl);
    principal->addWidget(zoneDroite, 1);

    // --- page Gerant : cartes "Gestion des employes" / "Statistiques" a gauche, fiche a droite,
    // plan de l'atelier compact en bas ; la page defile verticalement si l'ecran est petit ---
    auto *defilement = new QScrollArea(ui->pageGestionEmpl);
    defilement->setWidgetResizable(true);
    defilement->setFrameShape(QFrame::NoFrame);
    defilement->setStyleSheet("QScrollArea { background: transparent; }");
    auto *contenu = new QWidget();
    contenu->setObjectName("contenuGerant");
    contenu->setStyleSheet("QWidget#contenuGerant { background: transparent; }");
    defilement->setWidget(contenu);
    auto *lPage = new QVBoxLayout(ui->pageGestionEmpl);
    lPage->setContentsMargins(0, 0, 0, 0);
    lPage->addWidget(defilement);

    // loupe dans le champ de recherche
    ui->champRecherche->addAction(QIcon(pixmapIcone(Icone::Loupe, QColor("#5F707D"), 18)), QLineEdit::LeadingPosition);

    // carte 1 : recherche + boutons + tableau
    auto *contenuEmployes = new QWidget();
    auto *lEmployes = new QVBoxLayout(contenuEmployes);
    lEmployes->setContentsMargins(0, 0, 0, 0);
    lEmployes->setSpacing(10);
    lEmployes->addWidget(barreRecherche);
    lEmployes->addWidget(ui->tableEmployes, 1);
    ui->tableEmployes->setMinimumHeight(170);
    ui->tableEmployes->verticalHeader()->setDefaultSectionSize(34);
    QFrame *carteEmployes = creerCarteSection("Gestion des employés", contenuEmployes);

    // carte 2 : repartition par poste (a la place de l'ancien histogramme)
    m_repartition = new RepartitionPostes();
    ui->groupStatistiques->hide();
    const Icone iconeStatistiques = Icone::Repartition;
    QFrame *carteStatistiques = creerCarteSection("Statistiques des employés", m_repartition, true, &iconeStatistiques);

    // carte 3 : fiche employe sur deux colonnes. Chaque champ est range avec son libelle dans
    // une cellule ; le message de validation vient se glisser sous le champ, dans la cellule.
    auto cellule = [](QLabel *libelle, QWidget *champ) {
        auto *c = new QWidget();
        auto *v = new QVBoxLayout(c);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(4);
        v->addWidget(libelle);
        v->addWidget(champ);
        return c;
    };
    delete ui->groupFiche->layout();   // l'ancienne pile verticale ; les widgets restent dans le groupe
    ui->groupFiche->setTitle(QString());
    ui->groupFiche->setStyleSheet("QGroupBox { border: none; margin: 0; padding: 0; background: transparent; }");
    auto *fiche = new QGridLayout(ui->groupFiche);
    fiche->setContentsMargins(0, 0, 0, 0);
    fiche->setHorizontalSpacing(14);
    fiche->setVerticalSpacing(12);
    fiche->addWidget(cellule(ui->labelNomEmpl, ui->champNom), 0, 0);
    fiche->addWidget(cellule(ui->labelPrenomEmpl, ui->champPrenom), 0, 1);
    fiche->addWidget(cellule(ui->labelPosteEmpl, ui->comboPoste), 1, 0);
    fiche->addWidget(cellule(ui->labelStatutEmpl, ui->comboStatut), 1, 1);
    fiche->addWidget(cellule(ui->labelEmailEmpl, ui->champEmail), 2, 0);
    fiche->addWidget(cellule(ui->labelTelephoneEmpl, ui->champTelephone), 2, 1);
    auto *labelService = new QLabel("Service", ui->groupFiche);
    labelService->setStyleSheet(ui->labelPosteEmpl->styleSheet());
    labelService->setFont(ui->labelPosteEmpl->font());
    m_comboServiceEmploye = new QComboBox(ui->groupFiche);
    fiche->addWidget(cellule(ui->labelPosteTravail, ui->comboPosteTravail), 3, 0);
    fiche->addWidget(cellule(labelService, m_comboServiceEmploye), 3, 1);
    fiche->addWidget(cellule(ui->labelMotDePasseEmpl, ui->champMotDePasse), 4, 0, 1, 2);
    auto *boutons = new QHBoxLayout();
    boutons->setSpacing(14);
    ui->btnEnregistrerEmploye->setMinimumHeight(40);
    ui->btnAnnulerEmploye->setMinimumHeight(40);
    boutons->addWidget(ui->btnEnregistrerEmploye, 1);
    boutons->addWidget(ui->btnAnnulerEmploye, 1);
    // l'espace libre se repartit entre les lignes de champs ; les boutons restent en bas
    for (int ligne = 0; ligne < 5; ++ligne)
        fiche->setRowStretch(ligne, 1);
    fiche->addLayout(boutons, 5, 0, 1, 2);
    for (QWidget *champ : std::initializer_list<QWidget *>{
             ui->champNom, ui->champPrenom, ui->comboPoste, ui->comboStatut, ui->champEmail,
             ui->champTelephone, ui->comboPosteTravail, m_comboServiceEmploye, ui->champMotDePasse})
        champ->setMinimumHeight(38);
    QFrame *carteFiche = creerCarteSection("Fiche employé", ui->groupFiche);

    // plan de l'atelier : version compacte, sur toute la largeur
    m_planPostesGerant->setCompact(true);

    auto *grille = new QGridLayout(contenu);
    grille->setContentsMargins(0, 4, 8, 8);
    grille->setHorizontalSpacing(18);
    grille->setVerticalSpacing(16);
    grille->addWidget(carteEmployes, 0, 0);
    grille->addWidget(carteStatistiques, 1, 0);
    grille->addWidget(carteFiche, 0, 1, 2, 1);
    grille->addWidget(m_planPostesGerant, 2, 0, 1, 2);
    grille->setColumnStretch(0, 3);
    grille->setColumnStretch(1, 2);
    grille->setColumnMinimumWidth(1, 440);
    grille->setRowStretch(0, 1);   // le tableau prend toute la place libre, les statistiques restent basses
    grille->setRowStretch(1, 0);
    construireGraphiqueStatistiquesEmploye();

    // --- page employe : tout tient a l'ecran, sans defilement. Le plan est compact (une seule
    // ligne de cartes) et les trois blocs du bas rangent leurs champs par layouts : ils
    // s'adaptent a la largeur de la fenetre au lieu d'avoir une taille fixe. ---
    ui->groupFileAttenteEmpl->hide();   // ancienne "File d'attente intelligente", remplacee par le plan
    m_planPostesEmploye->setCompact(true);

    auto neutraliser = [](QGroupBox *groupe) {
        groupe->setTitle(QString());   // la carte porte le titre
        groupe->setStyleSheet("QGroupBox { border: none; margin: 0; padding: 0; background: transparent; }");
    };
    auto grilleDe = [](QWidget *parent) {
        auto *g = new QGridLayout(parent);
        g->setContentsMargins(0, 0, 0, 0);
        g->setHorizontalSpacing(12);
        g->setVerticalSpacing(8);
        return g;
    };

    // Mon profil : 2 x 2 champs en lecture seule
    neutraliser(ui->groupProfilEmpl);
    auto *profil = grilleDe(ui->groupProfilEmpl);
    profil->addWidget(creerCellule(ui->labelProfilNomEmpl, ui->lineProfilNomEmpl), 0, 0);
    profil->addWidget(creerCellule(ui->labelProfilPosteEmpl, ui->comboProfilPosteEmpl), 0, 1);
    profil->addWidget(creerCellule(ui->labelProfilEmailEmpl, ui->lineProfilEmailEmpl), 1, 0, 1, 2);
    profil->addWidget(creerCellule(ui->labelProfilTelEmpl, ui->lineProfilTelEmpl), 2, 0, 1, 2);

    // Mes documents RH : liste, dates, motif, bouton
    neutraliser(ui->groupDocumentsRHEmpl);
    ui->labelDocumentRHEmpl->hide();   // "Document pre-rempli..." : la liste se suffit a elle-meme
    auto *documents = grilleDe(ui->groupDocumentsRHEmpl);
    documents->addWidget(ui->comboDocumentRHEmpl, 0, 0, 1, 2);
    documents->addWidget(creerCellule(ui->labelDebutConge, ui->dateDebutConge), 1, 0);
    documents->addWidget(creerCellule(ui->labelFinConge, ui->dateFinConge), 1, 1);
    documents->addWidget(ui->champMotifConge, 2, 0, 1, 2);
    documents->addWidget(ui->btnExporterDocument, 3, 0, 1, 2);

    // Mes statistiques : le diagramme araignee remplit la carte
    neutraliser(ui->groupStatistiquesEmployeEmpl);
    if (ui->groupStatistiquesEmployeEmpl->layout())
        ui->groupStatistiquesEmployeEmpl->layout()->setContentsMargins(0, 0, 0, 0);

    for (QWidget *champ : std::initializer_list<QWidget *>{
             ui->lineProfilNomEmpl, ui->comboProfilPosteEmpl, ui->lineProfilEmailEmpl, ui->lineProfilTelEmpl, ui->comboDocumentRHEmpl,
             ui->dateDebutConge, ui->dateFinConge, ui->champMotifConge})
    {
        champ->setMinimumHeight(38);
        champ->setMaximumHeight(64);
        champ->setSizePolicy(champ->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    }
    ui->btnExporterDocument->setMinimumHeight(40);
    ui->btnExporterDocument->setMaximumHeight(64);
    ui->btnExporterDocument->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QFrame *carteProfil = creerCarteSection("Mon profil", ui->groupProfilEmpl, true);
    QFrame *carteDocuments = creerCarteSection("Mes documents RH", ui->groupDocumentsRHEmpl, true);
    QFrame *carteStatsEmploye = creerCarteSection("Mes statistiques", ui->groupStatistiquesEmployeEmpl, true);

    auto *lEmploye = new QGridLayout(ui->pageEspaceEmpl);
    lEmploye->setContentsMargins(0, 4, 8, 8);
    lEmploye->setHorizontalSpacing(16);
    lEmploye->setVerticalSpacing(14);
    lEmploye->addWidget(m_planPostesEmploye, 0, 0, 1, 3);
    lEmploye->addWidget(carteProfil, 1, 0);
    lEmploye->addWidget(carteDocuments, 1, 1);
    lEmploye->addWidget(carteStatsEmploye, 1, 2);
    lEmploye->setColumnStretch(0, 3);
    lEmploye->setColumnStretch(1, 3);
    lEmploye->setColumnStretch(2, 4);
    lEmploye->setRowStretch(0, 0);   // le plan prend sa hauteur naturelle
    lEmploye->setRowStretch(1, 1);   // les trois cartes se partagent le reste de la hauteur
}

void MainWindow::changerMonPosteTravail(const QString &poste)
{
    QString erreur;
    if (!m_employeConnecte.changerPosteTravail(poste, &erreur)) {
        QMessageBox::warning(this, "Poste de travail", erreur);
    }
    rafraichirPostesTravail();
}

void MainWindow::construireRadarStatistiques()
{
    QString erreur;
    const QMap<QString, int> parType = Employe::compterServicesParType(m_employeConnecte.id(), &erreur);
    if (!erreur.isEmpty()) {
        QMessageBox::warning(this, "Statistiques", "Impossible de charger les statistiques : " + erreur);
    }

    int total = 0;
    for (int n : parType)
        total += n;

    // Vide l'ancien contenu (au cas ou la methode est rappelee) avant d'ajouter le graphique.
    while (QLayoutItem *item = ui->layoutStatistiquesEmploye->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    auto *vue = new QChartView(creerDiagrammeAraignee(
        total == 0 ? "Aucun service pour l'instant" : "Mes services par type",
        Employe::typesService(), parType));
    vue->setRenderHint(QPainter::Antialiasing);
    vue->setStyleSheet("background: transparent;");
    vue->setMinimumHeight(150);
    ui->layoutStatistiquesEmploye->addWidget(vue);
}

void MainWindow::onExporterDocumentRHClicked()
{
    const QDate debut = ui->dateDebutConge->date();
    const QDate fin = ui->dateFinConge->date();
    if (fin < debut) {
        QMessageBox::warning(this, "Demande de congé",
                              "La date de fin doit être postérieure ou égale à la date de début.");
        ui->dateFinConge->setFocus();
        return;
    }

    const QString nomFichier = "Demande_conge_" + m_employeConnecte.nom() + "_"
                               + debut.toString("yyyyMMdd") + ".pdf";
    const QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter la demande de congé", QDir::home().filePath(nomFichier), "Documents PDF (*.pdf)");
    if (chemin.isEmpty())
        return; // annule par l'utilisateur

    QString erreur;
    if (!DocumentRH::exporterDemandeConge(m_employeConnecte, debut, fin,
                                          ui->champMotifConge->text(), chemin, &erreur)) {
        QMessageBox::warning(this, "Demande de congé", "Échec de l'export : " + erreur);
        return;
    }

    if (QMessageBox::question(this, "Demande de congé",
                              "Document exporté :\n" + chemin + "\n\nVoulez-vous l'ouvrir ?")
        == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(chemin));
    }
}
// === FIN MODULE EMPLOYE ===


// ====================================================================================
// === MODULE CLIENT ===
// ====================================================================================

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
        QTableWidgetItem *it = t->item(r, 5);
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

void MainWindow::initialiserModuleClient()
{
    connect(ui->btnDeconnexionClient, &QPushButton::clicked, this, &MainWindow::onDeconnexionClientClicked);

    QTableWidget *t = ui->cli_tableWidget;
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

    ui->cli_tableWidget->setColumnCount(7);
    ui->cli_tableWidget->setHorizontalHeaderLabels(
        {"id", "nom", "prénom", "email", "téléphone", "niveau", "statut"});
    ui->cli_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->cli_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ trierClients(); });

    ui->cli_tableWidget->setRowCount(0);

    ui->cli_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(ui->cli_lineEditRecherche, &QLineEdit::textChanged,
            this, &MainWindow::rechercherClients);

    QPixmap logo(":/logo_atelier.jpeg");
    ui->cli_label_13->setGeometry(10, 20, 111, 60);
    ui->cli_label_13->setAlignment(Qt::AlignCenter);
    ui->cli_label_13->setPixmap(logo.scaled(ui->cli_label_13->size(),
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation));
    ui->cli_label_13->raise();

    connect(ui->cli_pushButton_2, &QPushButton::clicked,
            this, &MainWindow::onAjouterClientClicked);

    connect(ui->cli_pushButton_3, &QPushButton::clicked,
            this, &MainWindow::onModifierClientClicked);

    // date et heure automatiques
    ui->cli_dateTimeEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    ui->cli_dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->cli_dateTimeEdit->setReadOnly(true);                             // l'utilisateur ne la modifie pas
    ui->cli_dateTimeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons); // enlève les flèches ^ v

    m_timerHorlogeClient = new QTimer(this);
    connect(m_timerHorlogeClient, &QTimer::timeout, this, [this]() {
        ui->cli_dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    });
    m_timerHorlogeClient->start(1000);   // mise à jour chaque seconde

    connect(ui->cli_pushButton_5, &QPushButton::clicked,
            this, &MainWindow::exporterListeClientsPdf);

    // m_diagrammeClient dans le cadre "Répartition"
    m_diagrammeClient = new DiagrammeWidget(ui->cli_groupBox_2);
    m_diagrammeClient->setGeometry(8, 22, ui->cli_groupBox_2->width() - 16,
                           ui->cli_groupBox_2->height() - 30);
    m_diagrammeClient->show();

    connect(ui->cli_tableWidget, &QTableWidget::currentCellChanged,
            this, [this](int, int, int, int){ majDiagrammeClient(); });

    // couleurs du menu définies directement ici
    m_styleMenuActifClient =
        "QPushButton { background:#8B1E2D; color:#FFFFFF;"
        " border:1px solid #8B1E2D; border-radius:4px; padding:4px; }";
    m_styleMenuNormalClient =
        "QPushButton { background:#2A4B63; color:#FFFFFF;"
        " border:1px solid #3C5A70; border-radius:4px; padding:4px; }"
        "QPushButton:hover { background:#355F7C; }";

    const QStringList titres = {"tableau de bord", "article", "service", "client", "employ",
                                 "événement", "matière"};
    const auto boutons = ui->PClient->findChildren<QPushButton*>();
    for (QPushButton *b : boutons) {
        const QString tx = b->text().trimmed().toLower();
        for (const QString &titre : titres) {
            if (tx.startsWith(titre)) { m_boutonsMenuClient.append(b); break; }
        }
    }

    qDebug() << "boutons du menu trouves :" << m_boutonsMenuClient.size();

    for (QPushButton *b : m_boutonsMenuClient)
        connect(b, &QPushButton::clicked, this, [this, b]() { activerMenuClient(b); });

    for (QPushButton *b : m_boutonsMenuClient)                       // "Client" actif au démarrage
        if (b->text().trimmed().toLower() == "client")
            activerMenuClient(b);

    // (les clients sont charges a chaque affichage de la page : voir connecterNavigation)

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
    nettoyer(ui->cli_groupBox, {ui->cli_lineEditNom, ui->cli_lineEditEmail, ui->cli_lineEditTel,
                            ui->cli_lineEditPrenom, ui->cli_pushButton_6, ui->cli_pushButtonOuvrir,
                            ui->cli_pushButtonEnregistrer, ui->cli_pushButtonAnnuler});
    nettoyer(ui->cli_groupBox_2, {m_diagrammeClient});
    delete ui->cli_groupBox->layout();
    delete ui->cli_groupBox_2->layout();

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
    ui->cli_label_13->setPixmap(logoRedim);
    ui->cli_label_13->setAlignment(Qt::AlignCenter);
    ui->cli_label_13->setFixedHeight(qMax(60, logoRedim.height() + 4));
    ls->addWidget(ui->cli_label_13);
    ls->addSpacing(14);

    QList<QPushButton*> ordre;
    for (const QString &nomMenu : titres)
        for (QPushButton *bm : m_boutonsMenuClient)
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
    ui->btnDeconnexionClient->setText("Déconnexion");
    ui->btnDeconnexionClient->setMinimumHeight(36);
    ui->btnDeconnexionClient->setStyleSheet(
        "QPushButton { background:transparent; color:#FFFFFF; border:1px solid #3C5A70;"
        "  border-radius:6px; padding:6px; }"
        "QPushButton:hover { background:#2A4B63; }");
    ls->addWidget(ui->btnDeconnexionClient);
    principal->addWidget(side);

    // styles du menu (texte aligné à gauche)
    m_styleMenuActifClient =
        "QPushButton { background:#8B1E2D; color:#FFFFFF; border:none; border-radius:6px;"
        "  padding:8px 14px; text-align:left; font-weight:bold; }";
    m_styleMenuNormalClient =
        "QPushButton { background:transparent; color:#E8E2D2; border:none; border-radius:6px;"
        "  padding:8px 14px; text-align:left; }"
        "QPushButton:hover { background:#2A4B63; }";
    for (QPushButton *bm : m_boutonsMenuClient)
        if (bm->text().trimmed().toLower() == "client")
            activerMenuClient(bm);

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
    ui->cli_dateTimeEdit->setFixedWidth(170);
    ui->cli_dateTimeEdit->setAlignment(Qt::AlignRight);
    ui->cli_dateTimeEdit->setStyleSheet("QDateTimeEdit { border:none; background:transparent;"
                                    " color:#1F3A4D; font-weight:bold; }");
    QLabel *lblSession = new QLabel("Session ouverte - Responsable clientèle");
    lblSession->setAlignment(Qt::AlignRight);
    lblSession->setStyleSheet("color:#5C6B77; background:transparent;");
    blocDate->addWidget(ui->cli_dateTimeEdit, 0, Qt::AlignRight);
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
    ui->cli_lineEditRecherche->setPlaceholderText("Rechercher par nom, prénom ou email...");
    ui->cli_lineEditRecherche->setStyleSheet(styleChamp);
    ui->cli_lineEditRecherche->setMinimumHeight(34);
    ui->cli_comboBox->setStyleSheet("QComboBox { background:#FFFFFF; border:1px solid #C9BFA8;"
                                " border-radius:6px; padding:6px 10px; color:#1F3A4D; }");
    ui->cli_comboBox->setMinimumHeight(34);
    ui->cli_pushButton_2->setText("+ Ajouter");
    ui->cli_pushButton_3->setText("Modifier");
    ui->cli_pushButton_4->setText("Supprimer");
    ui->cli_pushButton_5->setText("Export PDF");
    ui->cli_pushButton_2->setStyleSheet(btnPlein);
    ui->cli_pushButton_3->setStyleSheet(btnContour);
    ui->cli_pushButton_4->setStyleSheet(btnRouge);
    ui->cli_pushButton_5->setStyleSheet(btnMarine);
    lb->addWidget(ui->cli_lineEditRecherche, 1);
    lb->addWidget(ui->cli_comboBox);
    lb->addWidget(ui->cli_pushButton_2);
    lb->addWidget(ui->cli_pushButton_3);
    lb->addWidget(ui->cli_pushButton_4);
    lb->addWidget(ui->cli_pushButton_5);
    gauche->addWidget(barre);

    // tableau
    QFrame *cTable = carte();
    QVBoxLayout *lt = new QVBoxLayout(cTable);
    lt->setContentsMargins(10, 10, 10, 8);
    ui->cli_tableWidget->setMinimumHeight(150);
    ui->cli_tableWidget->verticalHeader()->setDefaultSectionSize(30);
    ui->cli_lblCount->setStyleSheet("color:#5C6B77; background:transparent;");
    lt->addWidget(ui->cli_tableWidget, 1);
    lt->addWidget(ui->cli_lblCount);
    gauche->addWidget(cTable, 3);

    // répartition + recommandations
    QHBoxLayout *bas = new QHBoxLayout;
    bas->setSpacing(12);

    ui->cli_groupBox_2->setTitle("Répartition par niveau");
    ui->cli_groupBox_2->setStyleSheet(styleGroupe);
    QVBoxLayout *lg2 = new QVBoxLayout(ui->cli_groupBox_2);
    lg2->setContentsMargins(10, 22, 10, 10);
    lg2->addWidget(m_diagrammeClient);
    bas->addWidget(ui->cli_groupBox_2, 1);

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
    ui->cli_groupBox->setStyleSheet(styleGroupe);
    ui->cli_groupBox->setMinimumWidth(300);
    ui->cli_groupBox->setMaximumWidth(430);
    QVBoxLayout *lf = new QVBoxLayout(ui->cli_groupBox);
    lf->setContentsMargins(16, 26, 16, 16);
    lf->setSpacing(4);

    ajouterChamp(lf, "Nom", ui->cli_lineEditNom, [](const QString &v) {
        static const QRegularExpression re(R"(^[\p{L} '\-]+$)");
        return re.match(v).hasMatch() ? QString()
                                      : QString("Le nom ne doit contenir que des lettres");
    });
    ajouterChamp(lf, "Prénom", ui->cli_lineEditPrenom, [](const QString &v) {
        static const QRegularExpression re(R"(^[\p{L} '\-]+$)");
        return re.match(v).hasMatch() ? QString()
                                      : QString("Le prénom ne doit contenir que des lettres");
    });
    ajouterChamp(lf, "Email", ui->cli_lineEditEmail, [this](const QString &v) {
        return emailClientValide(v) ? QString()
                              : QString("Email invalide (ex : nom@domaine.com)");
    });
    ajouterChamp(lf, "Téléphone", ui->cli_lineEditTel, [](const QString &v) {
        static const QRegularExpression re(R"(^[0-9 ]{8,12}$)");
        return re.match(v).hasMatch() ? QString()
                                      : QString("Le téléphone doit contenir 8 à 12 chiffres");
    });
    // niveau et statut : listes fermees (valeurs acceptees par la base)
    const QString styleListe =
        "QComboBox { background:#FFFFFF; border:1px solid #D9CFB8; border-radius:6px;"
        "  padding:6px 10px; color:#1F3A4D; }";
    auto ajouterListe = [lf, styleListe](const QString &libelle, QComboBox *&combo,
                                         const QStringList &valeurs) {
        QLabel *lbl = new QLabel(libelle);
        lbl->setStyleSheet("color:#1F3A4D; font-weight:bold; background:transparent;");
        combo = new QComboBox;
        combo->addItems(valeurs);
        combo->setStyleSheet(styleListe);
        combo->setMinimumHeight(34);
        lf->addWidget(lbl);
        lf->addWidget(combo);
    };
    ajouterListe("Niveau de fidélité", m_cmbNiveauClient, Client::niveauxAutorises());
    ajouterListe("Statut", m_cmbStatutClient, Client::statutsAutorises());
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
    ui->cli_pushButton_6->setText("Générer");
    ui->cli_pushButton_6->setStyleSheet(btnPlein);
    lrc->addWidget(ui->cli_pushButton_6);
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
    ui->cli_pushButtonOuvrir->setText("Ouvrir");
    ui->cli_pushButtonOuvrir->setStyleSheet(btnContour);
    lcc->addWidget(ui->cli_pushButtonOuvrir);
    lf->addWidget(cContrat);

    lf->addStretch();
    QHBoxLayout *lbtn = new QHBoxLayout;
    ui->cli_pushButtonEnregistrer->setText("Enregistrer");
    ui->cli_pushButtonAnnuler->setText("Annuler");
    ui->cli_pushButtonEnregistrer->setStyleSheet(btnPlein);
    ui->cli_pushButtonAnnuler->setStyleSheet(btnContour);
    ui->cli_pushButtonEnregistrer->setMinimumHeight(38);
    ui->cli_pushButtonAnnuler->setMinimumHeight(38);
    lbtn->addWidget(ui->cli_pushButtonEnregistrer, 1);
    lbtn->addWidget(ui->cli_pushButtonAnnuler, 1);
    lf->addLayout(lbtn);

    corps->addWidget(ui->cli_groupBox, 1);
    colonne->addLayout(corps, 1);

    // intégration : la nouvelle interface Clients remplit la page "PClient"
    const auto anciens = ui->PClient->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *w : anciens)
        w->hide();                                   // anciens widgets posés à la main
    QVBoxLayout *layClients = new QVBoxLayout(ui->PClient);
    layClients->setContentsMargins(0, 0, 0, 0);
    layClients->addWidget(racine);
}   // <-- fin du constructeur

bool MainWindow::emailClientValide(const QString &email) const
{
    static const QRegularExpression re(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    return re.match(email).hasMatch();
}

void MainWindow::on_cli_pushButtonEnregistrer_clicked()
{
    const bool modification = (m_modeModifClient && m_idModifClient > 0);
    const int id = modification ? m_idModifClient : 0;

    Client c(id, ui->cli_lineEditNom->text(), ui->cli_lineEditPrenom->text(),
             ui->cli_lineEditEmail->text(), ui->cli_lineEditTel->text(),
             m_cmbNiveauClient->currentText(), m_cmbStatutClient->currentText());

    // controle de saisie : fait par la classe Client (memes regles que les contraintes Oracle)
    QString message;
    if (!c.valider(&message)) {
        QMessageBox::warning(this, "Saisie invalide", message);
        return;
    }

    const bool ok = modification ? c.modifier() : c.ajouter();

    if (!ok) {
        const QString err = c.derniereErreur();
        if (err.contains("ORA-00001")) {
            QMessageBox::warning(this, "Doublon", "Cet email est déjà utilisé par un autre client.");
        } else if (err.contains("ORA-02290")) {
            QMessageBox::warning(this, "Données invalides",
                                 "Une valeur ne respecte pas les règles de la base (nom, prénom, email, téléphone, niveau...).");
        } else {
            QMessageBox::critical(this, "Erreur", err);
        }
        return;
    }

    chargerClients();
    QMessageBox::information(this, "Succès",
                             modification ? "Client modifié avec succès." : "Client enregistré avec succès.");
    viderChampsClient();
    m_modeAjoutClient = false;
    m_modeModifClient = false;
    m_ligneModifClient = -1;
    m_idModifClient = -1;
    ui->cli_groupBox->setTitle("Fiche client");
}

void MainWindow::on_cli_pushButtonAnnuler_clicked()
{
    m_modeAjoutClient = false;
    m_modeModifClient = false;
    m_ligneModifClient = -1;
    viderChampsClient();
    ui->cli_groupBox->setTitle("Fiche client");
    m_idModifClient = -1;
}

void MainWindow::viderChampsClient()
{
    ui->cli_lineEditNom->clear();
    ui->cli_lineEditEmail->clear();
    ui->cli_lineEditTel->clear();
    ui->cli_lineEditPrenom->clear();
    if (m_cmbNiveauClient) m_cmbNiveauClient->setCurrentIndex(0);
    if (m_cmbStatutClient) m_cmbStatutClient->setCurrentIndex(0);
    ui->cli_lineEditNom->setFocus();
}

void MainWindow::on_cli_pushButton_4_clicked()
{
    const int row = ui->cli_tableWidget->currentRow();

    if (row < 0) {
        QMessageBox::warning(this, "Aucune sélection",
                             "Veuillez sélectionner un client dans le tableau.");
        return;
    }

    const QString nom = ui->cli_tableWidget->item(row, 1)->text() + " "
                        + ui->cli_tableWidget->item(row, 2)->text();

    QMessageBox::StandardButton rep = QMessageBox::question(
        this, "Confirmation",
        "Voulez-vous vraiment supprimer le client « " + nom + " » ?",
        QMessageBox::Yes | QMessageBox::No);

    if (rep != QMessageBox::Yes)
        return;

    const int id = ui->cli_tableWidget->item(row, 0)->text().mid(1).toInt();

    Client c(id);
    if (!c.supprimer()) {
        const QString err = c.derniereErreur();
        if (err.contains("ORA-02292"))
            QMessageBox::warning(this, "Suppression impossible",
                                 "Ce client est lié à un service (table PASSER).");
        else
            QMessageBox::critical(this, "Erreur", err);
        return;
    }

    chargerClients();                   // recharge aussi le m_diagrammeClient
    QMessageBox::information(this, "Succès", "Client supprimé.");
}

void MainWindow::rechercherClients(const QString &texte)
{
    const QString recherche = texte.trimmed().toLower();
    int visibles = 0;
    for (int r = 0; r < ui->cli_tableWidget->rowCount(); ++r) {
        bool trouve = recherche.isEmpty();   // champ vide : tout afficher

        // colonnes : 1 = nom, 2 = prénom, 3 = email
        const int colonnes[] = {1, 2, 3};
        for (int c : colonnes) {
            QTableWidgetItem *it = ui->cli_tableWidget->item(r, c);
            if (it && it->text().toLower().contains(recherche)) {
                trouve = true;
                break;
            }
        }

        ui->cli_tableWidget->setRowHidden(r, !trouve);
        if (trouve) ++visibles;
    }

    ui->cli_lblCount->setText(QString("%1 clients affichés sur %2")
                              .arg(visibles).arg(ui->cli_tableWidget->rowCount()));
}

void MainWindow::on_cli_pushButtonOuvrir_clicked()
{
    // infos du formulaire, sinon du client sélectionné dans le tableau
    QString nom   = ui->cli_lineEditNom->text().trimmed();
    QString email = ui->cli_lineEditEmail->text().trimmed();
    QString tel   = ui->cli_lineEditTel->text().trimmed();
    QString prenom = ui->cli_lineEditPrenom->text().trimmed();
    QString ref = (m_idModifClient > 0)                         // numero de client (ex. C005)
                      ? QString("C%1").arg(m_idModifClient, 3, 10, QChar('0'))
                      : QString("à attribuer");

    const int row = ui->cli_tableWidget->currentRow();
    if (nom.isEmpty() && row >= 0) {
        auto texte = [&](int c) {
            QTableWidgetItem *it = ui->cli_tableWidget->item(row, c);
            return it ? it->text() : QString();
        };
        nom    = texte(1);
        prenom = texte(2);
        email  = texte(3);
        tel    = texte(4);
        ref    = texte(0);
    }
    nom = (nom + " " + prenom).trimmed();

    if (nom.isEmpty()) {
        QMessageBox::warning(this, "Contrat",
                             "Remplissez le formulaire ou sélectionnez un client dans le tableau.");
        return;
    }

    const QString fichier = QFileDialog::getSaveFileName(
        this, "Enregistrer le contrat",
        "contrat_" + (ref.startsWith('C') ? ref : QString("nouveau")) + ".pdf", "Fichiers PDF (*.pdf)");
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
    N° client : <b>%5</b>
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
    5. <b>Programme de fidélité :</b> Le niveau de fidélité (Bronze, Argent, Or) est strictement personnel et non cessible à un tiers.
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
                                  tel.toHtmlEscaped(), ref.toHtmlEscaped());

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

void MainWindow::on_cli_pushButton_6_clicked()
{
    // infos du formulaire, sinon du client sélectionné dans le tableau
    QString nom = ui->cli_lineEditNom->text().trimmed();
    QString prenom = ui->cli_lineEditPrenom->text().trimmed();
    QString ref = (m_idModifClient > 0)                         // numero de client (ex. C005)
                      ? QString("C%1").arg(m_idModifClient, 3, 10, QChar('0'))
                      : QString("à attribuer");

    const int row = ui->cli_tableWidget->currentRow();
    if (nom.isEmpty() && row >= 0) {
        QTableWidgetItem *n = ui->cli_tableWidget->item(row, 1);
        QTableWidgetItem *p = ui->cli_tableWidget->item(row, 2);
        QTableWidgetItem *r = ui->cli_tableWidget->item(row, 0);
        nom    = n ? n->text() : QString();
        prenom = p ? p->text() : QString();
        ref    = r ? r->text() : ref;
    }
    nom = (nom + " " + prenom).trimmed();

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
        "recu_" + (ref.startsWith('C') ? ref : QString("nouveau")) + ".pdf", "Fichiers PDF (*.pdf)");
    if (fichier.isEmpty())
        return;

    const QString numero = "RC-" + QDate::currentDate().toString("yyyyMMdd")
                           + "-" + (ref.startsWith('C') ? ref : QString("000"));

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
    N° client : <b>%4</b>
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
                                  ref.toHtmlEscaped(),
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

void MainWindow::onDeconnexionClientClicked()
{
    QMessageBox::StandardButton rep = QMessageBox::question(
        this, "Déconnexion",
        "Voulez-vous vraiment vous déconnecter ?",
        QMessageBox::Yes | QMessageBox::No);

    if (rep == QMessageBox::Yes)
        emit deconnexionDemandee();      // main.cpp ferme la fenêtre et rouvre l'écran de connexion
}

void MainWindow::onAjouterClientClicked()
{
    m_modeModifClient = false;
    m_ligneModifClient = -1;
    m_modeAjoutClient = true;
    viderChampsClient();                       // vide les champs + focus sur "nom et prenom"
    ui->cli_tableWidget->clearSelection();   // désélectionne la ligne du tableau

    ui->cli_groupBox->setTitle("Fiche client  (nouveau)");
    m_idModifClient = -1;
}

void MainWindow::onModifierClientClicked()
{
    const int row = ui->cli_tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Aucune sélection",
                             "Veuillez sélectionner un client dans le tableau.");
        return;
    }

    auto texte = [&](int c) {
        QTableWidgetItem *it = ui->cli_tableWidget->item(row, c);
        return it ? it->text() : QString();
    };

    // remplir la fiche avec les coordonnées du client
    ui->cli_lineEditNom->setText(texte(1));
    ui->cli_lineEditPrenom->setText(texte(2));
    ui->cli_lineEditEmail->setText(texte(3));
    ui->cli_lineEditTel->setText(texte(4));
    const int iNiveau = m_cmbNiveauClient->findText(texte(5), Qt::MatchFixedString);
    const int iStatut = m_cmbStatutClient->findText(texte(6), Qt::MatchFixedString);
    m_cmbNiveauClient->setCurrentIndex(iNiveau >= 0 ? iNiveau : 0);
    m_cmbStatutClient->setCurrentIndex(iStatut >= 0 ? iStatut : 0);

    m_modeAjoutClient = false;
    m_modeModifClient = true;
    m_ligneModifClient = row;
    m_idModifClient = texte(0).mid(1).toInt();   // l'id du client, ex. "C005" -> 5

    ui->cli_groupBox->setTitle("Fiche client  " + texte(0) + "  (modification)");
    ui->cli_lineEditNom->setFocus();
}

void MainWindow::majDiagrammeClient()
{
    const QStringList niveaux = Client::niveauxAutorises();      // Bronze, Argent, Or
    QVector<int> comptes(niveaux.size(), 0);
    for (int r = 0; r < ui->cli_tableWidget->rowCount(); ++r) {
        QTableWidgetItem *it = ui->cli_tableWidget->item(r, 5);
        if (it)
            comptes[rangNiveau(it->text()) - 1]++;
    }

    int sel = -1;
    const int row = ui->cli_tableWidget->currentRow();
    if (row >= 0 && ui->cli_tableWidget->item(row, 5))
        sel = rangNiveau(ui->cli_tableWidget->item(row, 5)->text()) - 1;

    m_diagrammeClient->setDonnees(comptes, sel, niveaux);
}

void DiagrammeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QStringList etiquettes = noms;                      // noms des barres (ex. Bronze, Argent, Or)
    while (etiquettes.size() < comptes.size())
        etiquettes << QString::number(etiquettes.size() + 1);
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
                   Qt::AlignCenter, etiquettes[i]);
    }
}

void MainWindow::trierClients()
{
    for (int r = 0; r < ui->cli_tableWidget->rowCount(); ++r) {
        QTableWidgetItem *in = ui->cli_tableWidget->item(r, 5);
        if (!in) continue;

        ItemTri *n = new ItemTri(in->text());
        n->setData(Qt::UserRole, rangNiveau(in->text()));
        ui->cli_tableWidget->setItem(r, 5, n);
    }

    const bool parNiveau = ui->cli_comboBox->currentText().toLower().contains("niveau");
    if (parNiveau)
        ui->cli_tableWidget->sortItems(5, Qt::DescendingOrder);   // Or, Argent, Bronze
    else
        ui->cli_tableWidget->sortItems(1, Qt::AscendingOrder);    // par nom
    ui->cli_tableWidget->scrollToTop();
    colorerNiveaux(ui->cli_tableWidget);
}

void MainWindow::chargerClients()
{
    ui->cli_tableWidget->setRowCount(0);

    QSqlQuery q;
    if (!Client::afficher(q))  {
        QMessageBox::critical(this, "Base de données", q.lastError().text());
        return;
    }

    while (q.next()) {
        const int r = ui->cli_tableWidget->rowCount();
        ui->cli_tableWidget->insertRow(r);

        ui->cli_tableWidget->setItem(r, 0, new QTableWidgetItem(
                                           QString("C%1").arg(q.value(0).toInt(), 3, 10, QChar('0'))));
        ui->cli_tableWidget->setItem(r, 1, new QTableWidgetItem(q.value(1).toString()));
        ui->cli_tableWidget->setItem(r, 2, new QTableWidgetItem(q.value(2).toString()));
        ui->cli_tableWidget->setItem(r, 3, new QTableWidgetItem(q.value(3).toString()));
        ui->cli_tableWidget->setItem(r, 4, new QTableWidgetItem(q.value(4).toString()));

        const QString niv = q.value(5).toString();
        auto *in = new ItemTri(niv);
        in->setData(Qt::UserRole, rangNiveau(niv));
        ui->cli_tableWidget->setItem(r, 5, in);

        const QString statut = q.value(6).toString();
        auto *is = new QTableWidgetItem(statut);
        is->setForeground(QColor(statut.compare("Actif", Qt::CaseInsensitive) == 0 ? "#2E7D4F" : "#7A8791"));
        ui->cli_tableWidget->setItem(r, 6, is);
    }
    colorerNiveaux(ui->cli_tableWidget);
    const int total = ui->cli_tableWidget->rowCount();
    ui->cli_lblCount->setText(QString("%1 clients affichés sur %2").arg(total).arg(total));
    //trierClients();                                           // réapplique le tri choisi
    //rechercherClients(ui->cli_lineEditRecherche->text());         // réapplique le filtre
    majDiagrammeClient();
}

void MainWindow::exporterListeClientsPdf()
{
    // lignes visibles du tableau (respecte la recherche et le tri)
    QString lignes;
    int nb = 0;
    for (int r = 0; r < ui->cli_tableWidget->rowCount(); ++r) {
        if (ui->cli_tableWidget->isRowHidden(r))
            continue;

        auto txt = [&](int c) {
            QTableWidgetItem *it = ui->cli_tableWidget->item(r, c);
            return it ? it->text().toHtmlEscaped() : QString();
        };

        const QString niv = txt(5).toLower();
        QString fond = "#F4D9C2", couleur = "#8A4B1F";            // bronze
        if (niv == "or")          { fond = "#F7E3A1"; couleur = "#8A6D1F"; }
        else if (niv == "argent") { fond = "#E4E7EB"; couleur = "#4A5560"; }

        const QString fondLigne = (nb % 2 == 0) ? "#FFFFFF" : "#FAF6EC";

        lignes += "<tr bgcolor=\"" + fondLigne + "\">";
        for (int c = 0; c < 5; ++c)
            lignes += "<td>" + txt(c) + "</td>";
        lignes += "<td align=\"center\" bgcolor=\"" + fond + "\">"
                  "<span style=\"color:" + couleur + ";\"><b>" + txt(5) + "</b></span></td>";
        lignes += "<td>" + txt(6) + "</td></tr>";
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
      <td><span style="color:#F5F1E8;"><b>Nom</b></span></td>
      <td><span style="color:#F5F1E8;"><b>Prénom</b></span></td>
      <td><span style="color:#F5F1E8;"><b>Email</b></span></td>
      <td><span style="color:#F5F1E8;"><b>Téléphone</b></span></td>
      <td align="center"><span style="color:#F5F1E8;"><b>Niveau</b></span></td>
      <td><span style="color:#F5F1E8;"><b>Statut</b></span></td>
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

void MainWindow::activerMenuClient(QPushButton *actif)
{
    for (QPushButton *b : m_boutonsMenuClient)
        b->setStyleSheet(b == actif ? m_styleMenuActifClient : m_styleMenuNormalClient);
}

void MainWindow::collecterGeometriesClient(QWidget *parent)
{
    for (QObject *o : parent->children()) {
        QWidget *w = qobject_cast<QWidget*>(o);
        if (!w || w->isWindow())
            continue;

        m_geomOrigClient.insert(w, w->geometry());

        // on descend seulement dans les simples conteneurs
        const QString classe = w->metaObject()->className();
        if (classe == "QWidget" || classe == "QFrame" || classe == "QGroupBox")
            collecterGeometriesClient(w);
    }
}

// UNE SEULE version de appliquerEchelleClient (géométrie + polices)
void MainWindow::appliquerEchelleClient()
{
    static QHash<QWidget*, qreal> fontOrig;
    if (m_baseSizeClient.isEmpty() || m_geomOrigClient.isEmpty())
        return;

    const QSize s = ui->PClient->size();
    const qreal sx = qreal(s.width())  / m_baseSizeClient.width();
    const qreal sy = qreal(s.height()) / m_baseSizeClient.height();
    const qreal sf = qMin(sx, sy);                       // échelle des polices

    for (auto it = m_geomOrigClient.cbegin(); it != m_geomOrigClient.cend(); ++it) {
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
// === FIN MODULE CLIENT ===


// ====================================================================================
// === MODULE ARTICLE ===   (Aziz)
// ====================================================================================
// Donnees : table Oracle ARTICLE, via la classe entite Article (Articles/article.h/.cpp).
// Cette zone ne contient AUCUNE requete SQL. L'interface (onglets Catalogue / Statistiques /
// Estimation et certificat) est le widget Art::ModuleArticles du dossier Articles/ : il est
// place dans la page PArticle, a cote du menu lateral dessine dans Qt Designer.
void MainWindow::initialiserModuleArticle()
{
    ui->lblUserNameArt->setText(m_employeConnecte.prenom() + " " + m_employeConnecte.nom());
    ui->lblUserRoleArt->setText(m_employeConnecte.poste());
    const QPixmap logo(":/images/logo.png");
    ui->lblLogoArt->setPixmap(logo.scaled(ui->lblLogoArt->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    // le widget du module est cree au premier affichage de la page (voir ouvrirModuleArticle)
}

void MainWindow::ouvrirModuleArticle()
{
    if (!m_moduleArticles) {
        m_moduleArticles = new Art::ModuleArticles(ui->contentFrameArt);
        ui->contentLayoutArt->addWidget(m_moduleArticles);
        m_moduleArticles->charger();      // premiere lecture de la table Oracle
        return;
    }
    m_moduleArticles->rafraichir();       // relit la table (modifications faites depuis un autre poste)
}

// Deconnexion (bouton du menu lateral) : confirmation puis retour a l'ecran de connexion
void MainWindow::onDeconnexionArticleClicked()
{
    QMessageBox::StandardButton rep = QMessageBox::question(
        this, "Déconnexion",
        "Voulez-vous vraiment vous déconnecter ?",
        QMessageBox::Yes | QMessageBox::No);

    if (rep == QMessageBox::Yes)
        emit deconnexionDemandee();      // main.cpp ferme la fenêtre et rouvre l'écran de connexion
}
// === FIN MODULE ARTICLE ===


// ====================================================================================
// === MODULE MATIERE PREMIERE ===   (Ahmed)
// ====================================================================================
// Donnees : table Oracle MATIERE_PREMIERE, via la classe entite Matiere (Matieres/matiere.h/.cpp).
// Cette zone ne contient AUCUNE requete SQL. L'interface (onglets Tableau de bord / Matieres
// premieres / Mouvements de stock) est le widget Mat::ModuleMatieres du dossier Matieres/ : il est
// place dans la page PMatiere, a cote du menu lateral dessine dans Qt Designer.
void MainWindow::initialiserModuleMatiere()
{
    ui->lblUserNameMat->setText(m_employeConnecte.prenom() + " " + m_employeConnecte.nom());
    ui->lblUserRoleMat->setText(m_employeConnecte.poste());
    const QPixmap logo(":/images/logo.png");
    ui->lblLogoMat->setPixmap(logo.scaled(ui->lblLogoMat->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    // le widget du module est cree au premier affichage de la page (voir ouvrirModuleMatiere)
}

void MainWindow::ouvrirModuleMatiere()
{
    if (!m_moduleMatieres) {
        m_moduleMatieres = new Mat::ModuleMatieres(ui->contentFrameMat);
        ui->contentLayoutMat->addWidget(m_moduleMatieres);
        m_moduleMatieres->charger();      // premiere lecture de la table Oracle
        return;
    }
    m_moduleMatieres->rafraichir();       // relit la table (modifications faites depuis un autre poste)
}

// Deconnexion (bouton du menu lateral) : confirmation puis retour a l'ecran de connexion
void MainWindow::onDeconnexionMatiereClicked()
{
    QMessageBox::StandardButton rep = QMessageBox::question(
        this, "Déconnexion",
        "Voulez-vous vraiment vous déconnecter ?",
        QMessageBox::Yes | QMessageBox::No);

    if (rep == QMessageBox::Yes)
        emit deconnexionDemandee();      // main.cpp ferme la fenêtre et rouvre l'écran de connexion
}
// === FIN MODULE MATIERE PREMIERE ===


// ====================================================================================
// === MODULE EVENEMENT ===   (Melek : ex-projet GEvents)
// ====================================================================================
// Donnees : table Oracle EVENEMENT, via la classe Evenement (Evenements/evenement.h/.cpp).
// Cette zone ne contient AUCUNE requete SQL : elle affiche et saisit, la classe fait le reste.
namespace {

const QStringList kTypesEvenement = Evenement::typesAutorises();
const char *kFormatDateEvenement = "yyyy-MM-dd";   // affichage de la date dans le tableau

// clé utilisée par le QSS : QLabel[badge="programme"] { ... }
QString cleBadgeEvenement(const QString &statut)
{
    static const QHash<QString, QString> map = {
        { "Planifié", "planifie" }, { "Programmé", "programme" },
        { "Terminé", "termine" },   { "Annulé", "annule" }
    };
    return map.value(statut, "termine");
}

} // namespace

// ------------------------------------------------------------------------------------
// Initialisation du module (ex-constructeur de la fenetre GEvents de Melek)
// ------------------------------------------------------------------------------------
void MainWindow::initialiserModuleEvenement()
{
    // Reapplique la feuille de style de la page : certains widgets (dans la zone de
    // defilement) sont "polis" avant d'avoir un parent, donc sans style.
    {
        const QString qss = ui->PEvenement->styleSheet();
        ui->PEvenement->setStyleSheet(QString());
        ui->PEvenement->setStyleSheet(qss);
    }

    // Utilisateur connecte dans la barre laterale (remplace le nom fige du .ui)
    ui->lblUserNameEvt->setText(m_employeConnecte.prenom() + " " + m_employeConnecte.nom());
    ui->lblUserRoleEvt->setText(m_employeConnecte.poste());

    const QLocale fr(QLocale::French, QLocale::France);
    ui->calendarEvt->setLocale(fr);
    ui->dateEditEvt->setLocale(fr);

    // Connexions vers les slots de MainWindow (celles entre widgets restent dans le .ui)
    connect(ui->btnEnregistrerEvt,  &QPushButton::clicked, this, &MainWindow::enregistrerEvenement);
    connect(ui->btnSupprimerEvt,    &QPushButton::clicked, this, &MainWindow::supprimerEvenement);
    connect(ui->btnModifierEvt,     &QPushButton::clicked, this, &MainWindow::modifierEvenement);
    connect(ui->btnChoisirImageEvt, &QPushButton::clicked, this, &MainWindow::choisirImageEvenement);
    connect(ui->lineSearchEvt, &QLineEdit::textChanged, this, &MainWindow::rechercherEvenements);
    connect(ui->comboTriEvt, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::trierEvenements);
    for (QComboBox *c : { ui->comboStatutEvt, ui->comboTypeEvt, ui->comboLieuEvt })
        connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::filtrerEvenements);

    connect(ui->btnAjouterEvt,       &QPushButton::clicked, this, &MainWindow::onAjouterEvenementClicked);
    connect(ui->btnAnnulerEvt,       &QPushButton::clicked, this, &MainWindow::onAnnulerEvenementClicked);
    connect(ui->btnReinitialiserEvt, &QPushButton::clicked, this, &MainWindow::onReinitialiserEvenementClicked);
    connect(ui->tableEventsEvt,      &QTableWidget::itemSelectionChanged, this, &MainWindow::onSelectionEvenementChanged);
    connect(ui->calendarEvt,         &QCalendarWidget::clicked,           this, &MainWindow::onCalendrierEvenementClicked);
    connect(ui->calendarEvt,         &QCalendarWidget::selectionChanged,  this, &MainWindow::majListeJourEvenement);

    // Navigation interne : menu lateral "Evenements" (tableau de bord) / bouton "Calendrier" (sous la recherche)
    connect(ui->btnNavEvenementsEvt, &QPushButton::clicked, this, &MainWindow::afficherListeEvenements);
    connect(ui->btnCalendrierEvt,    &QPushButton::clicked, this, &MainWindow::afficherCalendrierEvenements);
    // Page "Calendrier" : ouvrir un evenement du jour dans la fiche
    connect(ui->listJourEvt,         &QListWidget::itemDoubleClicked, this, &MainWindow::onJourEvenementActivated);
    connect(ui->btnOuvrirFicheEvt,   &QPushButton::clicked,           this, &MainWindow::ouvrirEvenementDuJour);

    // A chaque retour sur cette page : menu lateral sur "Evenements" et liste (pas le calendrier)
    connect(ui->SWSmartStitch, &QStackedWidget::currentChanged, this, [this](int index) {
        if (index == PageEvenement) {
            ui->btnNavEvenementsEvt->setChecked(true);
            ui->stackPagesEvt->setCurrentWidget(ui->pageEvenementsEvt);
        }
    });

    chargerEvenements();
    ui->calendarEvt->setSelectedDate(dateInitialeCalendrierEvenement());
    viderFicheEvenement();
    rafraichirEvenements();
    if (ui->tableEventsEvt->rowCount() > 0)
        ui->tableEventsEvt->selectRow(0);
}

// ======================================================================
//  Données : table Oracle EVENEMENT (via la classe Evenement)
// ======================================================================
// Dossier de l'application : copie locale des affiches (images) choisies dans la fiche.
QString MainWindow::dossierDonneesEvenement() const
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/evenements";
    if (d.isEmpty())
        d = QDir::homePath() + "/.smartstitch_evenements";
    QDir().mkpath(d);
    return d;
}

void MainWindow::chargerEvenements()
{
    m_evenements = Evenement::afficher();
    if (m_evenements.isEmpty() && !Evenement::derniereErreur().isEmpty()) {
        qWarning() << "Evenements : lecture impossible :" << Evenement::derniereErreur();
        QMessageBox::warning(this, "Événements",
                             "Impossible de lire les événements dans la base de données.\n"
                             "Vérifiez que le script Oracle v5 a bien été exécuté (table EVENEMENT).");
    }
}

const Evenement *MainWindow::trouverEvenementParId(int id) const
{
    for (const Evenement &e : m_evenements)
        if (e.id() == id)
            return &e;
    return nullptr;
}

// ======================================================================
//  Rafraîchissement de l'affichage
// ======================================================================
void MainWindow::rafraichirEvenements(int selectId)
{
    majCombosFiltreEvenement();
    remplirTableEvenements(selectId >= 0 ? selectId : m_idEvenementCourant);
    majCartesEvenement();
    majBarresEvenement();
    majLieuxEvenement();
    majMarquesCalendrierEvenement();
    majListeJourEvenement();
}

QVector<Evenement> MainWindow::evenementsFiltresTries() const
{
    const QString q = ui->lineSearchEvt->text().trimmed();
    QVector<Evenement> res;
    for (const Evenement &e : m_evenements) {
        if (!q.isEmpty()) {
            const QString hay = e.nom() + " " + e.type() + " " + e.lieu() + " " + e.statut();
            if (!hay.contains(q, Qt::CaseInsensitive))
                continue;
        }
        if (ui->comboStatutEvt->currentIndex() > 0 && e.statut() != ui->comboStatutEvt->currentText()) continue;
        if (ui->comboTypeEvt->currentIndex() > 0   && e.type()   != ui->comboTypeEvt->currentText())   continue;
        if (ui->comboLieuEvt->currentIndex() > 0   && e.lieu()   != ui->comboLieuEvt->currentText())   continue;
        res.push_back(e);
    }

    // ----- tri -----
    const int mode = ui->comboTriEvt->currentIndex();
    const QDate today = QDate::currentDate();
    QHash<QString, int> perLieu;                       // = panneau « Événements par lieu »
    for (const Evenement &e : m_evenements)
        ++perLieu[e.lieu()];
    QCollator col{ QLocale(QLocale::French) };         // départage les lieux ex æquo (é = e)
    col.setCaseSensitivity(Qt::CaseInsensitive);

    auto dist = [&](const Evenement &e) {
        const QDate d = e.date();
        return d.isValid() ? int(qAbs(today.daysTo(d))) : INT_MAX / 2;
    };
    auto less = [&](const Evenement &a, const Evenement &b) -> bool {
        switch (mode) {
        case 2:                                         // Lieu : le plus d'événements d'abord
            if (perLieu[a.lieu()] != perLieu[b.lieu()]) return perLieu[a.lieu()] > perLieu[b.lieu()];
            if (a.lieu() != b.lieu()) return col.compare(a.lieu(), b.lieu()) < 0;
            return a.date() < b.date();
        default:                                        // Date (0 : proche → lointaine, 1 : inverse)
            if (dist(a) != dist(b)) return dist(a) < dist(b);
            if (a.date() != b.date())   return a.date() < b.date();
            return a.heure() < b.heure();
        }
    };
    const bool reverse = (mode == 1);                   // Date : la plus lointaine → la plus proche
    std::stable_sort(res.begin(), res.end(), [&](const Evenement &a, const Evenement &b) {
        return reverse ? less(b, a) : less(a, b);
    });
    return res;
}

QWidget *MainWindow::creerBadgeEvenement(const QString &statut)
{
    auto *lbl = new QLabel(statut);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setProperty("badge", cleBadgeEvenement(statut));       // stylé par le QSS du .ui
    auto *wrap = new QWidget;
    wrap->setObjectName("badgeWrapEvt");
    wrap->setStyleSheet("QWidget#badgeWrapEvt { background: transparent; }");
    auto *lay = new QHBoxLayout(wrap);
    lay->setContentsMargins(4, 8, 4, 8);
    lay->addWidget(lbl);
    return wrap;
}

void MainWindow::remplirTableEvenements(int selectId)
{
    QTableWidget *t = ui->tableEventsEvt;
    m_evenementsVisibles = evenementsFiltresTries();

    const bool old = t->blockSignals(true);
    t->clearSelection();
    t->setRowCount(0);
    t->setRowCount(m_evenementsVisibles.size());
    int target = -1;
    for (int r = 0; r < m_evenementsVisibles.size(); ++r) {
        const Evenement &e = m_evenementsVisibles[r];
        const QStringList cells = { QString::number(e.id()), e.nom(), e.type(),
                                    e.date().toString(kFormatDateEvenement), e.lieu() };
        for (int c = 0; c < cells.size(); ++c)
            t->setItem(r, c, new QTableWidgetItem(cells[c]));
        t->setCellWidget(r, 5, creerBadgeEvenement(e.statut()));
        if (e.id() == selectId)
            target = r;
    }
    if (target >= 0)
        t->selectRow(target);
    t->blockSignals(old);
}

void MainWindow::majCombosFiltreEvenement()
{
    // liste des lieux : valeurs distinctes presentes dans la base (premier item = "Tous les lieux")
    QComboBox *combo = ui->comboLieuEvt;
    const QString first = combo->count() ? combo->itemText(0) : QString();
    const QString cur = combo->currentText();
    QStringList values;
    for (const Evenement &e : m_evenements)
        if (!e.lieu().isEmpty() && !values.contains(e.lieu()))
            values << e.lieu();
    values.sort(Qt::CaseInsensitive);

    const bool old = combo->blockSignals(true);
    combo->clear();
    combo->addItem(first);
    combo->addItems(values);
    combo->setCurrentIndex(qMax(0, combo->findText(cur)));
    combo->blockSignals(old);
}

void MainWindow::majCartesEvenement()
{
    // "A venir" = date a partir d'aujourd'hui, hors evenements termines ou annules
    int aVenir = 0, programme = 0, planifie = 0;
    const QDate today = QDate::currentDate();
    for (const Evenement &e : m_evenements) {
        if (e.statut() == "Programmé") ++programme;
        else if (e.statut() == "Planifié") ++planifie;
        if (e.date() >= today && e.statut() != "Terminé" && e.statut() != "Annulé") ++aVenir;
    }
    ui->lblTotalValueEvt->setText(QString::number(m_evenements.size()));
    ui->lblAVenirValueEvt->setText(QString::number(aVenir));
    ui->lblProgrammeValueEvt->setText(QString::number(programme));
    ui->lblPlanifieValueEvt->setText(QString::number(planifie));
}

void MainWindow::majBarresEvenement()
{
    // --- événements par type ---
    QHash<QString, int> byType;
    int maxT = 1;
    for (const Evenement &e : m_evenements)
        maxT = std::max(maxT, ++byType[e.type()]);
    for (int i = 0; i < kTypesEvenement.size(); ++i) {
        auto *bar = findChild<QProgressBar *>(QString("barT%1Evt").arg(i));
        auto *val = findChild<QLabel *>(QString("lblValT%1Evt").arg(i));
        const int n = byType.value(kTypesEvenement[i], 0);
        if (bar) { bar->setMaximum(maxT + 1); bar->setValue(n); }
        if (val) val->setText(QString::number(n));
    }
}

void MainWindow::majLieuxEvenement()
{
    QStringList order;
    QHash<QString, int> n;
    for (const Evenement &e : m_evenements) {
        if (e.lieu().isEmpty()) continue;
        if (!n.contains(e.lieu())) order << e.lieu();
        ++n[e.lieu()];
    }
    std::stable_sort(order.begin(), order.end(), [&](const QString &a, const QString &b) { return n[a] > n[b]; });
    ui->listLieuxEvt->clear();
    for (const QString &l : order)
        ui->listLieuxEvt->addItem(QString("📍  %1   ·   %2 événement%3").arg(l).arg(n[l]).arg(n[l] > 1 ? "s" : ""));
}

QDate MainWindow::dateInitialeCalendrierEvenement() const
{
    QList<QDate> dates;
    for (const Evenement &e : m_evenements) {
        const QDate d = e.date();
        if (d.isValid()) dates << d;
    }
    if (dates.isEmpty()) return QDate::currentDate();
    std::sort(dates.begin(), dates.end());
    for (const QDate &d : std::as_const(dates))
        if (d >= QDate::currentDate()) return d;
    return dates.last();
}

void MainWindow::majMarquesCalendrierEvenement()
{
    ui->calendarEvt->setDateTextFormat(QDate(), QTextCharFormat());   // efface tout
    QTextCharFormat fmt;
    fmt.setBackground(QColor("#dce9f7"));
    fmt.setForeground(QColor("#17375e"));
    fmt.setFontWeight(QFont::Bold);
    for (const Evenement &e : m_evenements) {
        const QDate d = e.date();
        if (d.isValid())
            ui->calendarEvt->setDateTextFormat(d, fmt);
    }
}

// ======================================================================
//  Fiche événement
// ======================================================================
int MainWindow::idEvenementSelectionne() const
{
    const int row = ui->tableEventsEvt->currentRow();
    if (row >= 0 && row < m_evenementsVisibles.size() && !ui->tableEventsEvt->selectedItems().isEmpty())
        return m_evenementsVisibles[row].id();
    return -1;
}

void MainWindow::viderFicheEvenement()
{
    m_idEvenementCourant = -1;
    m_imageEvenementCourante.clear();
    ui->lineNomEvt->clear();
    ui->comboTypeFormEvt->setCurrentIndex(0);
    ui->comboStatutFormEvt->setCurrentIndex(0);
    ui->dateEditEvt->setDate(QDate::currentDate());
    ui->timeEditEvt->setTime(QTime(18, 0));
    ui->lineLieuEvt->clear();
    ui->spinCapaciteEvt->setValue(50);
    ui->textDescriptionEvt->clear();
    definirApercuEvenement(QString());
    ui->lblFicheTitleEvt->setText("📅  Fiche événement");
}

void MainWindow::remplirFicheEvenement(const Evenement &e)
{
    ui->lineNomEvt->setText(e.nom());
    ui->comboTypeFormEvt->setCurrentText(e.type());
    ui->comboStatutFormEvt->setCurrentText(e.statut());
    const QDate d = e.date();
    ui->dateEditEvt->setDate(d.isValid() ? d : QDate::currentDate());
    const QTime t = QTime::fromString(e.heure(), "HH:mm");
    ui->timeEditEvt->setTime(t.isValid() ? t : QTime(18, 0));
    ui->lineLieuEvt->setText(e.lieu());
    ui->spinCapaciteEvt->setValue(e.capacite() > 0 ? e.capacite() : 50);
    ui->textDescriptionEvt->setPlainText(e.description());
    m_imageEvenementCourante = e.imageAffiche();
    definirApercuEvenement(m_imageEvenementCourante);
    ui->lblFicheTitleEvt->setText(QString("📅  Fiche événement  #%1").arg(e.id()));
    if (d.isValid())
        ui->calendarEvt->setSelectedDate(d);
}

Evenement MainWindow::donneesFicheEvenement()
{
    Evenement e;
    e.setNom(ui->lineNomEvt->text().trimmed());
    e.setType(ui->comboTypeFormEvt->currentText());
    e.setStatut(ui->comboStatutFormEvt->currentText());
    e.setDate(ui->dateEditEvt->date());
    e.setHeure(ui->timeEditEvt->time().toString("HH:mm"));
    e.setLieu(ui->lineLieuEvt->text().trimmed());
    e.setCapacite(ui->spinCapaciteEvt->value());
    e.setDescription(ui->textDescriptionEvt->toPlainText().trimmed());
    e.setImageAffiche(copierImageEvenement(m_imageEvenementCourante));
    return e;
}

void MainWindow::definirApercuEvenement(const QString &path)
{
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        ui->lblImagePreviewEvt->setPixmap(
            QPixmap(path).scaled(56, 56, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        ui->btnChoisirImageEvt->setText(QFileInfo(path).fileName());
    } else {
        ui->lblImagePreviewEvt->setPixmap(QPixmap());
        ui->lblImagePreviewEvt->setText("🖼");
        ui->btnChoisirImageEvt->setText("Choisir une image…");
    }
}

QString MainWindow::copierImageEvenement(const QString &path) const
{
    // copie l'affiche dans le dossier de données pour que le chemin reste valide
    if (path.isEmpty() || !QFileInfo::exists(path))
        return QString();
    const QString dir = dossierDonneesEvenement() + "/images";
    QDir().mkpath(dir);
    const QString dest = dir + "/" + QFileInfo(path).fileName();
    if (QFileInfo(path).absoluteFilePath() != QFileInfo(dest).absoluteFilePath()) {
        QFile::remove(dest);
        QFile::copy(path, dest);
    }
    return dest;
}

// ======================================================================
//  Slots : boutons et widgets
// ======================================================================
void MainWindow::onSelectionEvenementChanged()
{
    const int id = idEvenementSelectionne();
    if (id < 0) return;
    if (const Evenement *e = trouverEvenementParId(id)) {
        m_idEvenementCourant = id;
        remplirFicheEvenement(*e);
    }
}

void MainWindow::onCalendrierEvenementClicked(const QDate &date)
{
    // (le .ui met déjà à jour le champ Date) – sélectionne l'événement de ce jour s'il existe
    for (int r = 0; r < m_evenementsVisibles.size(); ++r) {
        if (m_evenementsVisibles[r].date() == date) {
            ui->tableEventsEvt->selectRow(r);
            return;
        }
    }
}

// ======================================================================
//  Navigation et page « Calendrier »
// ======================================================================
void MainWindow::afficherListeEvenements()
{
    ui->stackPagesEvt->setCurrentWidget(ui->pageEvenementsEvt);
}

void MainWindow::afficherCalendrierEvenements()
{
    ui->stackPagesEvt->setCurrentWidget(ui->pageCalendrierEvt);
    majListeJourEvenement();
}

void MainWindow::majListeJourEvenement()
{
    const QDate d = ui->calendarEvt->selectedDate();
    ui->lblJourDateEvt->setText(QLocale(QLocale::French, QLocale::France).toString(d, "dddd d MMMM yyyy"));

    QVector<Evenement> day;
    for (const Evenement &e : m_evenements)
        if (e.date() == d)
            day.push_back(e);
    std::sort(day.begin(), day.end(), [](const Evenement &a, const Evenement &b) { return a.heure() < b.heure(); });

    ui->listJourEvt->clear();
    for (const Evenement &e : day) {
        auto *it = new QListWidgetItem(QString("%1  ·  %2\n📍 %3   —   %4").arg(e.heure(), e.nom(), e.lieu(), e.statut()));
        it->setData(Qt::UserRole, e.id());
        ui->listJourEvt->addItem(it);
    }
    if (day.isEmpty()) {
        auto *it = new QListWidgetItem("Aucun événement à cette date.");
        it->setFlags(Qt::NoItemFlags);
        ui->listJourEvt->addItem(it);
    } else {
        ui->listJourEvt->setCurrentRow(0);
    }
    ui->btnOuvrirFicheEvt->setEnabled(!day.isEmpty());
}

void MainWindow::ouvrirEvenementDuJour()
{
    if (QListWidgetItem *it = ui->listJourEvt->currentItem())
        onJourEvenementActivated(it);
}

void MainWindow::onJourEvenementActivated(QListWidgetItem *item)
{
    if (!item || !item->data(Qt::UserRole).isValid())
        return;
    ouvrirEvenementDansFiche(item->data(Qt::UserRole).toInt());
}

void MainWindow::ouvrirEvenementDansFiche(int id)
{
    const Evenement *e = trouverEvenementParId(id);
    if (!e)
        return;
    bool visible = false;
    for (const Evenement &v : std::as_const(m_evenementsVisibles))
        if (v.id() == id) { visible = true; break; }
    if (!visible)
        onReinitialiserEvenementClicked();                      // un filtre ou une recherche masque l'événement

    ui->btnNavEvenementsEvt->setChecked(true);     // retour au tableau de bord
    ui->stackPagesEvt->setCurrentWidget(ui->pageEvenementsEvt);
    for (int r = 0; r < m_evenementsVisibles.size(); ++r) {
        if (m_evenementsVisibles[r].id() == id) {
            ui->tableEventsEvt->selectRow(r);
            break;
        }
    }
    m_idEvenementCourant = id;
    remplirFicheEvenement(*e);                               // la fiche affiche l'événement choisi
}

void MainWindow::onAjouterEvenementClicked()
{
    ui->tableEventsEvt->clearSelection();
    viderFicheEvenement();
    ui->lblFicheTitleEvt->setText("📅  Nouvel événement");
    ui->lineNomEvt->setFocus();
}

void MainWindow::onAnnulerEvenementClicked()
{
    // le .ui vide d'abord les champs ; on restaure ensuite l'événement courant
    if (const Evenement *e = trouverEvenementParId(m_idEvenementCourant))
        remplirFicheEvenement(*e);
    else
        viderFicheEvenement();
}

void MainWindow::onReinitialiserEvenementClicked()
{
    for (QComboBox *c : { ui->comboStatutEvt, ui->comboTypeEvt, ui->comboLieuEvt, ui->comboTriEvt }) {
        const bool old = c->blockSignals(true);
        c->setCurrentIndex(0);
        c->blockSignals(old);
    }
    ui->lineSearchEvt->clear();
    remplirTableEvenements(m_idEvenementCourant);
}

void MainWindow::modifierEvenement()
{
    if (idEvenementSelectionne() < 0 && m_idEvenementCourant < 0) {
        QMessageBox::information(this, "Modifier", "Sélectionnez d'abord un événement dans le tableau.");
        return;
    }
    ui->lblFicheTitleEvt->setText(QString("✏️  Modification  #%1").arg(m_idEvenementCourant));
}

void MainWindow::supprimerEvenement()
{
    const int id = idEvenementSelectionne();
    const Evenement *e = trouverEvenementParId(id);
    if (!e) {
        QMessageBox::information(this, "Supprimer", "Sélectionnez d'abord un événement dans le tableau.");
        return;
    }
    QMessageBox box(QMessageBox::Question, "Supprimer",
                    QString("Voulez-vous vraiment supprimer « %1 » ?").arg(e->nom()),
                    QMessageBox::Yes | QMessageBox::No, this);
    box.button(QMessageBox::Yes)->setText("Oui");
    box.button(QMessageBox::No)->setText("Non");
    if (box.exec() != QMessageBox::Yes)
        return;

    if (!Evenement::supprimer(id)) {
        qWarning() << "Evenements : suppression impossible :" << Evenement::derniereErreur();
        QMessageBox::critical(this, "Supprimer", "La suppression a échoué.\n" + Evenement::derniereErreur());
        return;
    }
    chargerEvenements();
    viderFicheEvenement();
    rafraichirEvenements(-1);
}

void MainWindow::enregistrerEvenement()
{
    Evenement e = donneesFicheEvenement();

    // controle de saisie (memes regles que les contraintes de la table, cf. Evenement::valider)
    if (e.nom().isEmpty()) {
        QMessageBox::warning(this, "Champ obligatoire", "Le nom de l'événement est obligatoire.");
        ui->lineNomEvt->setFocus();
        return;
    }
    if (e.lieu().isEmpty()) {
        QMessageBox::warning(this, "Champ obligatoire", "Le lieu est obligatoire.");
        ui->lineLieuEvt->setFocus();
        return;
    }
    QString erreur;
    if (!e.valider(&erreur)) {
        QMessageBox::warning(this, "Saisie invalide", erreur);
        return;
    }

    bool ok;
    if (m_idEvenementCourant < 0) {
        ok = e.ajouter();                       // INSERT : renseigne e.id()
    } else {
        e.setId(m_idEvenementCourant);
        ok = e.modifier();                      // UPDATE
    }
    if (!ok) {
        qWarning() << "Evenements : enregistrement impossible :" << Evenement::derniereErreur();
        QMessageBox::critical(this, "Enregistrer", "L'enregistrement a échoué.\n" + Evenement::derniereErreur());
        return;
    }

    m_idEvenementCourant = e.id();
    chargerEvenements();
    rafraichirEvenements(e.id());
    ui->lblFicheTitleEvt->setText(QString("📅  Fiche événement  #%1").arg(e.id()));
    QMessageBox::information(this, "Enregistré", "L'événement a été enregistré avec succès.");
}

void MainWindow::choisirImageEvenement()
{
    const QString path = QFileDialog::getOpenFileName(this, "Choisir une image", QString(),
                                                      "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (path.isEmpty()) return;
    m_imageEvenementCourante = path;
    definirApercuEvenement(path);
}

void MainWindow::rechercherEvenements(const QString &)   { remplirTableEvenements(m_idEvenementCourant); }
void MainWindow::trierEvenements(int)          { remplirTableEvenements(m_idEvenementCourant); }
void MainWindow::filtrerEvenements(int)        { remplirTableEvenements(m_idEvenementCourant); }

// ------------------------------------------------------------------------------------
// Deconnexion (bouton du menu lateral) : confirmation puis retour a l'ecran de connexion
// ------------------------------------------------------------------------------------
void MainWindow::onDeconnexionEvenementClicked()
{
    QMessageBox::StandardButton rep = QMessageBox::question(
        this, "Déconnexion",
        "Voulez-vous vraiment vous déconnecter ?",
        QMessageBox::Yes | QMessageBox::No);

    if (rep == QMessageBox::Yes)
        emit deconnexionDemandee();      // main.cpp ferme la fenêtre et rouvre l'écran de connexion
}
// === FIN MODULE EVENEMENT ===
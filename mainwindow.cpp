#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "service.h"
#include "client.h"
#include <QRegularExpression>
#include <QTableWidgetItem>
#include <QString>
#include <QPageSize>
#include <QDesktopServices>
#include <QUrl>
#include <QDate>
#include <QLineEdit>
#include <QTextFrame>
#include <QColor>
#include <QInputDialog>
#include <QDateTime>
#include <QWidget>
#include <QGroupBox>
#include <QEvent>
#include <QTimer>
#include <QFont>
#include <QStatusBar>
#include <functional>
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

// ---------- Construction ----------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Atelier Vintage Couture - Gestion des services");
    setMinimumSize(640, 480);
    const QRect avail = screen()->availableGeometry();
    resize(qMin(1600, avail.width()), qMin(860, avail.height()));

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

    // Chargement via la classe Service
    m_services = Service::chargerDepuisDb();
    ui->tableServices->setRowCount(m_services.size());
    for (int r = 0; r < m_services.size(); ++r) fillRow(r);

    updateChart();
    updateBanner();
    updateCount();
    if (ui->tableServices->rowCount() > 0)
        ui->tableServices->selectRow(0);

    initModuleClients();   // === MODULE CLIENT ===
    setupNavigation();     // === SLOTS GÉNÉRAUX ===
}

MainWindow::~MainWindow() { delete ui; }

// =====================================================================
// === SLOTS GÉNÉRAUX (NAVIGATION ENTRE PAGES) ==========================
// =====================================================================
void MainWindow::setupNavigation()
{
    // menu latéral du module Service : bouton "Clients"
    connect(ui->pushButton_4, &QPushButton::clicked, this, [this]() {
        ui->stackedWidgetMain->setCurrentWidget(ui->pageClients);
    });
    // menu latéral du module Client : bouton "Service"
    connect(ui->cli_pushButton_11, &QPushButton::clicked, this, [this]() {
        ui->stackedWidgetMain->setCurrentWidget(ui->pageServices);
    });
    ui->stackedWidgetMain->setCurrentWidget(ui->pageServices);   // page de départ
}

// =====================================================================
// === MODULE SERVICE (Rawen) : fonctions de la fenêtre des services ====
// =====================================================================

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

    const QList<QPushButton *> nav = {ui->pushButton, ui->pushButton_2, ui->pushButton_3,
                                      ui->pushButton_4, ui->pushButton_5};
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
    connect(logout, &QPushButton::clicked, this, &QWidget::close);
}

// ---------- En-tête : création des widgets (placés par setupLayout) ----------
void MainWindow::setupHeader()
{
    lblSousTitre = new QLabel("Location, restauration, nettoyage, authentification — "
                              "planning intelligent et alertes par e-mail",
                              ui->pageServices);
    lblSousTitre->setWordWrap(true);
    lblSousTitre->setStyleSheet("color:#6B7A88; font-size:11px; background:transparent;");

    ui->txtRecherche->setPlaceholderText("Rechercher par client ou tarif...");
    ui->txtRecherche->setMinimumHeight(32);
    ui->cmbTri->setMinimumHeight(32);
    ui->cmbTri->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    ui->cmbTri->setMinimumContentsLength(10);

    btnExportPDF = new QPushButton("Export PDF", ui->pageServices);
    btnExportPDF->setMinimumSize(130, 32);
    btnExportPDF->setStyleSheet("QPushButton{background-color:#1F3A4D; color:white; border-radius:6px;}"
                                "QPushButton:hover{background-color:#8B1E2D;}");
    connect(btnExportPDF, &QPushButton::clicked, this, &MainWindow::exportPdf);

    btnRappels = new QPushButton("✉ Envoyer les rappels", ui->pageServices);
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
    cmbEmploye->addItems({"Mouna Ayari", "Sami Guesmi", "Karim Fekih",
                          "Amine Sassi", "Salma Trabelsi"});

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
    t->setHorizontalHeaderLabels({"ID", "Article", "Client", "Type", "Employé",
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

    lblCount = new QLabel(ui->pageServices);
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
    const QStringList v = { s.id, s.article, s.client.isEmpty() ? "—" : s.client, s.type,
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
    ui->frameDetails->setTitle("Fiche service · " + s.id);
    setCombo(ui->cmbArticle, s.article);
    setCombo(ui->cmbClient,  s.client);
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
            lignes << QString("• %1 (%2 → %3, %4)").arg(o.id, o.debut.toString("dd/MM/yyyy"),
                                                        o.fin.toString("dd/MM/yyyy"), o.type);
            libre = qMax(libre, o.fin.addDays(1));
        }
        QMessageBox::warning(this, "Conflit de dates",
                             QString("L'article « %1 » est déjà engagé dans :\n%2\n\n"
                                     "Prochaine disponibilité : %3.\nChangez les dates ou l'article.")
                                 .arg(s.article, lignes.join("\n"), libre.toString("dd/MM/yyyy")));
        return;
    }

    m_services[m_current] = s;
    fillRow(m_current);
    ui->tableServices->selectRow(m_current);
    updateChart();
    updateBanner();
    statusBar()->showMessage("Service " + s.id + " enregistré", 3000);

    // Enregistrement via la classe Service (INSERT si nouveau, UPDATE sinon)
    bool ok = false;
    if (s.id.isEmpty()) {
        ok = s.ajouter();
    } else {
        QSqlQuery check;
        check.prepare("select count(*) from service where id = :id");
        check.bindValue(":id", s.id);
        if (check.exec() && check.next() && check.value(0).toInt() > 0)
            ok = s.modifier();
        else
            ok = s.ajouter();
    }

    if (!ok) {
        QMessageBox::critical(this, "Oracle",
                              "Échec de l'enregistrement dans la base.");
        return;
    }
}

void MainWindow::onCancel() { fillForm(m_current); }

void MainWindow::onSearch(const QString &text)
{
    auto *t = ui->tableServices;
    for (int r = 0; r < t->rowCount(); ++r) {
        bool ok = text.isEmpty();
        for (int c : {1, 2, 6})
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
        if (s.statut == "En retard") ids << s.id;
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
    auto *mid = new QWidget(ui->pageServices);
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
    scrollDetails = new QScrollArea(ui->pageServices);
    scrollDetails->setFrameShape(QFrame::NoFrame);
    scrollDetails->setWidgetResizable(true);
    scrollDetails->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollDetails->setWidget(ui->frameDetails);

    // zone de contenu : horizontale en grand, verticale en très petit (cf. applyResponsive)
    m_content = new QBoxLayout(QBoxLayout::LeftToRight);
    m_content->setSpacing(0);
    m_content->addWidget(mid, 3);
    m_content->addWidget(scrollDetails, 1);

    auto *main = new QHBoxLayout(ui->pageServices);
    main->setContentsMargins(0, 0, 10, 0);
    main->setSpacing(0);
    main->addWidget(ui->frameSidebar);
    main->addLayout(m_content, 1);

    // (le contenu de cette page est affiché dans le stackedWidgetMain)

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
    for (int i : c) { ids << m_services[i].id; libre = qMax(libre, m_services[i].fin.addDays(1)); }
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
    QDialog dlg(this);
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
                           .arg(a.id, b.id, a.article,
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
                                               .arg(conf ? "⚠ " : "", sv.id, sv.article, sv.statut));
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
    QString file = QFileDialog::getSaveFileName(this, "Reçu de service", "recu_" + s.id + ".pdf", "PDF (*.pdf)");
    if (file.isEmpty()) return;

    QString html = QString("<h2>Atelier Vintage Couture</h2><h3>Reçu de service %1</h3>"
                           "<p>Article : %2<br>Client : %3<br>Type : %4<br>Fin prévue : %5</p>"
                           "<table border='1' cellspacing='0' cellpadding='6'>"
                           "<tr><td>Tarif</td><td>%6</td></tr>"
                           "<tr><td>Acompte versé</td><td>%7</td></tr>"
                           "<tr><td><b>Solde restant</b></td><td><b>%8</b></td></tr></table>")
                       .arg(s.id, s.article, s.client.isEmpty() ? "—" : s.client, s.type,
                            s.fin.toString("dd/MM/yyyy"), dt(s.tarif), dt(s.acompte), dt(s.tarif - s.acompte));

    QPdfWriter w(file);
    w.setPageSize(QPageSize(QPageSize::A5));
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&w);
    txtAlertes->append("● Reçu généré pour " + s.id);
}

void MainWindow::envoyerRappels()
{
    int n = 0;
    for (const Service &s : std::as_const(m_services))
        if (s.statut == "En retard") {
            txtAlertes->append(QString("● Rappel envoyé à %1 — %2 en retard.").arg(s.employe, s.id));
            ++n;
        }
    // TODO : envoi réel par e-mail (SMTP) ici
    QMessageBox::information(this, "Rappels", QString("%1 rappel(s) envoyé(s).").arg(n));
}

// ---------- Ajouter / Modifier / Supprimer ----------
void MainWindow::onAjouter()
{
    int n = 0;
    for (const Service &x : std::as_const(m_services))
        n = qMax(n, x.id.mid(2).toInt());

    Service s;
    s.id      = QString("SV%1").arg(n + 1, 3, 10, QChar('0'));
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
    statusBar()->showMessage("Nouveau service " + s.id +
                                 " : remplissez la fiche puis cliquez sur Enregistrer", 6000);
    // TODO : INSERT dans Oracle au moment de Enregistrer
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
    const QString id = m_services[m_current].id;
    if (QMessageBox::question(this, "Supprimer", "Supprimer le service " + id + " ?")
        != QMessageBox::Yes)
        return;

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
    statusBar()->showMessage("Service " + id + " supprimé", 4000);

    // Suppression via la classe Service
    Service svc;
    if (!svc.supprimer(id)) {
        QMessageBox::critical(this, "Oracle",
                              "Échec de la suppression dans la base.");
        return;
    }
}

// =====================================================================
// === MODULE CLIENT (Maysam) : travailler ici uniquement ===============
// =====================================================================

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

void MainWindow::initModuleClients()
{

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
        {"id", "nom et prenom", "email", "telephone", "cin", "points", "niveau"});
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
            this, &MainWindow::onAjouterClicked);

    connect(ui->cli_pushButton_3, &QPushButton::clicked,
            this, &MainWindow::onModifierClicked);

    // date et heure automatiques
    ui->cli_dateTimeEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    ui->cli_dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->cli_dateTimeEdit->setReadOnly(true);                             // l'utilisateur ne la modifie pas
    ui->cli_dateTimeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons); // enlève les flèches ^ v

    timerHorloge = new QTimer(this);
    connect(timerHorloge, &QTimer::timeout, this, [this]() {
        ui->cli_dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    });
    timerHorloge->start(1000);   // mise à jour chaque seconde

    connect(ui->cli_pushButton_5, &QPushButton::clicked,
            this, &MainWindow::exporterListePdf);

    // diagramme dans le cadre "Répartition"
    diagramme = new DiagrammeWidget(ui->cli_groupBox_2);
    diagramme->setGeometry(8, 22, ui->cli_groupBox_2->width() - 16,
                           ui->cli_groupBox_2->height() - 30);
    diagramme->show();

    connect(ui->cli_tableWidget, &QTableWidget::currentCellChanged,
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
    const auto boutons = ui->pageClients->findChildren<QPushButton*>();
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
    nettoyer(ui->cli_groupBox, {ui->cli_lineEditNom, ui->cli_lineEditEmail, ui->cli_lineEditTel,
                            ui->cli_lineEditCin, ui->cli_pushButton_6, ui->cli_pushButtonOuvrir,
                            ui->cli_pushButtonEnregistrer, ui->cli_pushButtonAnnuler});
    nettoyer(ui->cli_groupBox_2, {diagramme});
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
    ui->cli_pushButton_13->setText("Déconnexion");
    ui->cli_pushButton_13->setMinimumHeight(36);
    ui->cli_pushButton_13->setStyleSheet(
        "QPushButton { background:transparent; color:#FFFFFF; border:1px solid #3C5A70;"
        "  border-radius:6px; padding:6px; }"
        "QPushButton:hover { background:#2A4B63; }");
    ls->addWidget(ui->cli_pushButton_13);
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
    ui->cli_lineEditRecherche->setPlaceholderText("Rechercher par nom, email ou CIN...");
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

    ui->cli_groupBox_2->setTitle("Répartition par points");
    ui->cli_groupBox_2->setStyleSheet(styleGroupe);
    QVBoxLayout *lg2 = new QVBoxLayout(ui->cli_groupBox_2);
    lg2->setContentsMargins(10, 22, 10, 10);
    lg2->addWidget(diagramme);
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

    ajouterChamp(lf, "Nom et prénom", ui->cli_lineEditNom, [](const QString &v) {
        static const QRegularExpression re(R"(^[\p{L} '\-]+$)");
        return re.match(v).hasMatch() ? QString()
                                      : QString("Le nom ne doit contenir que des lettres");
    });
    ajouterChamp(lf, "Email", ui->cli_lineEditEmail, [this](const QString &v) {
        return emailValide(v) ? QString()
                              : QString("Email invalide (ex : nom@domaine.com)");
    });
    ajouterChamp(lf, "Téléphone", ui->cli_lineEditTel, [](const QString &v) {
        static const QRegularExpression re(R"(^[0-9 ]{8,12}$)");
        return re.match(v).hasMatch() ? QString()
                                      : QString("Le téléphone doit contenir 8 à 12 chiffres");
    });
    ajouterChamp(lf, "CIN", ui->cli_lineEditCin, [](const QString &v) {
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

    // intégration : la nouvelle interface Clients remplit la page "pageClients"
    const auto anciens = ui->pageClients->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *w : anciens)
        w->hide();                                   // anciens widgets posés à la main
    QVBoxLayout *layClients = new QVBoxLayout(ui->pageClients);
    layClients->setContentsMargins(0, 0, 0, 0);
    layClients->addWidget(racine);
}   // <-- fin du constructeur

bool MainWindow::emailValide(const QString &email) const
{
    static const QRegularExpression re(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    return re.match(email).hasMatch();
}

void MainWindow::on_cli_pushButtonEnregistrer_clicked()
{
    const QString nom   = ui->cli_lineEditNom->text().trimmed();
    const QString email = ui->cli_lineEditEmail->text().trimmed();
    const QString tel   = ui->cli_lineEditTel->text().trimmed();
    const QString cin   = ui->cli_lineEditCin->text().trimmed();

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
    ui->cli_groupBox->setTitle("Fiche client");
}

void MainWindow::on_cli_pushButtonAnnuler_clicked()
{
    modeAjout = false;
    modeModif = false;
    ligneModif = -1;
    viderChamps();
    ui->cli_groupBox->setTitle("Fiche client");
    idModif = -1;
}

void MainWindow::viderChamps()
{
    ui->cli_lineEditNom->clear();
    ui->cli_lineEditEmail->clear();
    ui->cli_lineEditTel->clear();
    ui->cli_lineEditCin->clear();
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

    const QString nom = ui->cli_tableWidget->item(row, 1)->text();

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
    for (int r = 0; r < ui->cli_tableWidget->rowCount(); ++r) {
        bool trouve = recherche.isEmpty();   // champ vide : tout afficher

        // colonnes : 1 = nom, 2 = email, 4 = cin
        const int colonnes[] = {1, 2, 4};
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
    QString cin   = ui->cli_lineEditCin->text().trimmed();

    const int row = ui->cli_tableWidget->currentRow();
    if (nom.isEmpty() && row >= 0) {
        auto texte = [&](int c) {
            QTableWidgetItem *it = ui->cli_tableWidget->item(row, c);
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

void MainWindow::on_cli_pushButton_6_clicked()
{
    // infos du formulaire, sinon du client sélectionné dans le tableau
    QString nom = ui->cli_lineEditNom->text().trimmed();
    QString cin = ui->cli_lineEditCin->text().trimmed();

    const int row = ui->cli_tableWidget->currentRow();
    if (nom.isEmpty() && row >= 0) {
        QTableWidgetItem *n = ui->cli_tableWidget->item(row, 1);
        QTableWidgetItem *c = ui->cli_tableWidget->item(row, 4);
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

void MainWindow::on_cli_pushButton_13_clicked()
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
    ui->cli_tableWidget->clearSelection();   // désélectionne la ligne du tableau

    ui->cli_groupBox->setTitle("Fiche client  (nouveau)");
    idModif = -1;
}

void MainWindow::onModifierClicked()
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
    ui->cli_lineEditEmail->setText(texte(2));
    ui->cli_lineEditTel->setText(texte(3));
    ui->cli_lineEditCin->setText(texte(4));

    modeAjout = false;
    modeModif = true;
    ligneModif = row;
    idModif = texte(0).mid(1).toInt();   // l'id du client, ex. "C005" -> 5

    ui->cli_groupBox->setTitle("Fiche client  " + texte(0) + "  (modification)");
    ui->cli_lineEditNom->setFocus();
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
    for (int r = 0; r < ui->cli_tableWidget->rowCount(); ++r) {
        QTableWidgetItem *it = ui->cli_tableWidget->item(r, 5);
        if (it)
            comptes[tranche(it->text().toInt())]++;
    }

    int sel = -1;
    const int row = ui->cli_tableWidget->currentRow();
    if (row >= 0 && ui->cli_tableWidget->item(row, 5))
        sel = tranche(ui->cli_tableWidget->item(row, 5)->text().toInt());

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
    for (int r = 0; r < ui->cli_tableWidget->rowCount(); ++r) {
        QTableWidgetItem *ip = ui->cli_tableWidget->item(r, 5);
        QTableWidgetItem *in = ui->cli_tableWidget->item(r, 6);
        if (!ip || !in) continue;

        const QString pts = ip->text();
        const QString niv = in->text();

        ItemTri *p = new ItemTri(pts);
        p->setData(Qt::UserRole, pts.toInt());
        ui->cli_tableWidget->setItem(r, 5, p);

        ItemTri *n = new ItemTri(niv);
        n->setData(Qt::UserRole, rangNiveau(niv));
        ui->cli_tableWidget->setItem(r, 6, n);
    }

    const bool parNiveau = ui->cli_comboBox->currentText().toLower().contains("niveau");
    ui->cli_tableWidget->sortItems(parNiveau ? 6 : 5, Qt::DescendingOrder);
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

        const int pts = q.value(5).toInt();
        auto *ip = new ItemTri(QString::number(pts));
        ip->setData(Qt::UserRole, pts);
        ui->cli_tableWidget->setItem(r, 5, ip);

        const QString niv = q.value(6).toString();
        auto *in = new ItemTri(niv);
        in->setData(Qt::UserRole, rangNiveau(niv));
        ui->cli_tableWidget->setItem(r, 6, in);
    }
    colorerNiveaux(ui->cli_tableWidget);
    const int total = ui->cli_tableWidget->rowCount();
    ui->cli_lblCount->setText(QString("%1 clients affichés sur %2").arg(total).arg(total));
    //trierClients();                                           // réapplique le tri choisi
    //rechercherClients(ui->cli_lineEditRecherche->text());         // réapplique le filtre
    majDiagramme();
}

void MainWindow::exporterListePdf()
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

    const QSize s = ui->pageClients->size();
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



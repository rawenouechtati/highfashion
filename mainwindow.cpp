#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "notificationsdialog.h"
#include "service.h"
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDirIterator>
#include <QFileDialog>
#include <QFrame>
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
    // fenêtre adaptée à l'écran, avec barres de défilement si l'écran est petit
    QWidget *cw = takeCentralWidget();
    cw->setFixedSize(1740, 790);
    auto *scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(cw);
    setCentralWidget(scroll);
    const QRect avail = screen()->availableGeometry();
    resize(qMin(1760, avail.width()), qMin(860, avail.height()));

    // ouvrir la fenêtre de notifications
    connect(ui->btnNotifications, &QPushButton::clicked, this, [this]() {
        NotificationsDialog dialog(this);
        dialog.exec();
    });

    setupSidebar();
    setupHeader();
    setupFiche();
    setupTable();
    setupChart();
    setupStatusBar();

    // Chargement via la classe Service
    m_services = Service::chargerDepuisDb();
    ui->tableServices->setRowCount(m_services.size());
    for (int r = 0; r < m_services.size(); ++r) fillRow(r);

    updateChart();
    updateBanner();
    updateCount();
    if (ui->tableServices->rowCount() > 0)
        ui->tableServices->selectRow(0);
}

MainWindow::~MainWindow() { delete ui; }

// ---------- Menu latéral : logo + profil + déconnexion ----------
void MainWindow::setupSidebar()
{
    ui->frameSidebar->setGeometry(0, -10, 201, 800);
    for (QPushButton *b : ui->frameSidebar->findChildren<QPushButton *>())
        b->move(b->x(), b->y() + 60);

    auto *logo = new QLabel(ui->frameSidebar);
    logo->setGeometry(10, 20, 181, 72);
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
        logo->setPixmap(pm.scaled(logo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    auto *avatar = new QLabel("RS", ui->frameSidebar);
    avatar->setGeometry(12, 700, 36, 36);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("background:#B39B6A; color:white; font-weight:bold; border-radius:18px;");

    auto *profil = new QLabel("<b>r.services</b><br><small>Responsable des services</small>", ui->frameSidebar);
    profil->setGeometry(54, 698, 145, 40);
    profil->setStyleSheet("background:transparent; color:white;");

    auto *logout = new QPushButton("Déconnexion", ui->frameSidebar);
    logout->setGeometry(8, 748, 185, 30);
    logout->setFlat(true);
    logout->setStyleSheet("QPushButton{background:transparent; color:#D8CFBC; text-align:left; padding-left:8px;}"
                          "QPushButton:hover{color:white; background:transparent;}");
    connect(logout, &QPushButton::clicked, this, &QWidget::close);
}

// ---------- En-tête : sous-titre, Export PDF, bannière, boutons ----------
void MainWindow::setupHeader()
{
    auto *sous = new QLabel("Location, restauration, nettoyage, authentification — "
                            "notifications système et alertes par e-mail",
                            ui->centralwidget);
    sous->setGeometry(225, 36, 780, 16);
    sous->setStyleSheet("color:#6B7A88; font-size:11px; background:transparent;");
    sous->show();

    ui->txtRecherche->move(ui->txtRecherche->x(), 62);
    ui->cmbTri->move(ui->cmbTri->x(), 62);
    ui->txtRecherche->setPlaceholderText("Rechercher par client ou tarif...");

    btnExportPDF = new QPushButton("Export PDF", ui->centralwidget);
    btnExportPDF->setGeometry(1040, 62, 151, 29);
    btnExportPDF->setStyleSheet("QPushButton{background-color:#1F3A4D; color:white; border-radius:6px;}"
                                "QPushButton:hover{background-color:#8B1E2D;}");
    btnExportPDF->show();
    connect(btnExportPDF, &QPushButton::clicked, this, &MainWindow::exportPdf);

    btnRappels = new QPushButton("✉ Envoyer les rappels", ui->centralwidget);
    btnRappels->setGeometry(1010, 105, 181, 31);
    btnRappels->setStyleSheet("QPushButton{background-color:#8B1E2D; color:white;}");
    btnRappels->show();
    connect(btnRappels, &QPushButton::clicked, this, &MainWindow::envoyerRappels);

    ui->lblAlerte->setGeometry(340, 100, 660, 41);

    connect(ui->btnAjouter,   &QPushButton::clicked, this, &MainWindow::onAjouter);
    connect(ui->btnModifier,  &QPushButton::clicked, this, &MainWindow::onModifier);
    connect(ui->btnSupprimer, &QPushButton::clicked, this, &MainWindow::onSupprimer);
    connect(ui->txtRecherche, &QLineEdit::textChanged, this, &MainWindow::onSearch);
}

// ---------- Fiche service ----------
void MainWindow::setupFiche()
{
    QWidget *f = ui->frameDetails;
    auto *lblEmp = new QLabel("Employé assigné", f);
    lblEmp->setFont(ui->lblType->font());
    lblEmp->setGeometry(270, 130, 211, 31);
    lblEmp->show();

    cmbEmploye = new QComboBox(f);
    cmbEmploye->setGeometry(270, 170, 211, 28);
    cmbEmploye->addItems({"Mouna Ayari", "Sami Guesmi", "Karim Fekih",
                          "Amine Sassi", "Salma Trabelsi"});
    cmbEmploye->show();

    for (QWidget *w : {(QWidget *)ui->lblSoldeTitre, (QWidget *)ui->lblSolde,
                       (QWidget *)ui->lblBenefice, (QWidget *)ui->lblValeurBenefice})
        w->hide();

    auto makeCard = [f](const QString &name, const QString &titre, QLabel *&valeur,
                        const QRect &r, const QString &bg, const QString &fg) {
        auto *card = new QFrame(f);
        card->setGeometry(r);
        card->setObjectName(name);
        card->setStyleSheet(QString("QFrame#%1{background:%2; border:1px solid %3; border-radius:6px;}"
                                    "QLabel{background:transparent; border:none; color:%3;}")
                                .arg(name, bg, fg));
        auto *t = new QLabel(titre.toUpper(), card);
        t->setGeometry(12, 6, r.width() - 24, 16);
        t->setStyleSheet(QString("font-size:10px; font-weight:bold; color:%1;").arg(fg));
        valeur = new QLabel("0 DT", card);
        valeur->setGeometry(12, 24, r.width() - 24, 28);
        valeur->setStyleSheet(QString("font-size:18px; font-weight:bold; color:%1;").arg(fg));
        card->show();
    };

    makeCard("cardSoldeNew", "Solde restant", lblSoldeVal, QRect(10, 455, 235, 60), "#FBE9EC", "#8B1E2D");
    makeCard("cardBenefNew", "Bénéfice",      lblBenefVal, QRect(255, 455, 235, 60), "#FFF4DC", "#9A6B00");

    connect(ui->spinTarif,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::recalc);
    connect(ui->spinCout,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::recalc);
    connect(ui->spinAcompte, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::recalc);

    auto *recu = new QFrame(f);
    recu->setObjectName("cardRecu");
    recu->setGeometry(10, 525, 480, 55);
    recu->setStyleSheet("QFrame#cardRecu{background:white; border:1px solid #E2D9C6; border-radius:8px;}"
                        "QLabel{background:transparent; border:none;}");
    auto *rt = new QLabel("<b>Reçu de service</b><br><small>Ticket : tarif, acompte, solde</small>", recu);
    rt->setGeometry(12, 8, 300, 40);

    btnGenerer = new QPushButton("Générer", recu);
    btnGenerer->setGeometry(370, 12, 100, 31);
    btnGenerer->setStyleSheet("QPushButton{background-color:#8B1E2D; color:white; font-weight:bold;}");
    recu->show();
    connect(btnGenerer, &QPushButton::clicked, this, &MainWindow::genererRecu);

    ui->chkEcheance->hide();
    ui->chkRetard->hide();
    ui->frameAlertesEmail->setTitle("Alertes du service · système et e-mail");
    ui->frameAlertesEmail->setGeometry(10, 592, 480, 112);

    txtAlertes = new QTextEdit(ui->frameAlertesEmail);
    txtAlertes->setGeometry(10, 24, 460, 48);
    txtAlertes->setReadOnly(true);
    txtAlertes->setStyleSheet("QTextEdit{background:#FFF8F0; border:1px solid #E2D9C6; border-radius:6px;}");
    txtAlertes->setPlainText("● Rappel envoyé par e-mail à Mouna Ayari — échéance dans 2 jours.");
    txtAlertes->show();

    ui->btnConfigurerAlertes->setGeometry(300, 78, 170, 29);

    btnEnregistrer = new QPushButton("Enregistrer", f);
    btnEnregistrer->setGeometry(10, 718, 235, 38);
    btnEnregistrer->setStyleSheet("QPushButton{background-color:#8B1E2D; color:white; font-weight:bold;}");
    btnEnregistrer->show();

    btnAnnuler = new QPushButton("Annuler", f);
    btnAnnuler->setGeometry(255, 718, 235, 38);
    btnAnnuler->setStyleSheet("QPushButton{background-color:white; color:#1F3A4D; border:1px solid #B39B6A;}"
                              "QPushButton:hover{background-color:#F5F1E8;}");
    btnAnnuler->show();

    connect(btnEnregistrer, &QPushButton::clicked, this, &MainWindow::onSave);
    connect(btnAnnuler,     &QPushButton::clicked, this, &MainWindow::onCancel);
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
    t->verticalHeader()->hide();
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setHighlightSections(false);

    int w[] = {70, 210, 130, 140, 130, 100, 70};
    for (int i = 0; i < 7; ++i) t->setColumnWidth(i, w[i]);
    t->verticalHeader()->setDefaultSectionSize(27);

    ui->frameStats->move(210, 458);
    t->setItemDelegateForColumn(3, new BadgeDelegate(t));
    t->setItemDelegateForColumn(7, new BadgeDelegate(t));
    t->setStyleSheet(
        "QTableWidget{background:white; alternate-background-color:#FAF7F1; border:1px solid #E2D9C6; border-radius:8px;}"
        "QHeaderView::section{background:#1F3A4D; color:white; padding:6px; border:none; font-weight:bold;}"
        "QTableWidget::item:selected{background:#F3D9DD; color:#1F3A4D;}");

    lblCount = new QLabel(ui->centralwidget);
    lblCount->setGeometry(210, 442, 990, 14);
    lblCount->setStyleSheet("color:#6B7A88; font-size:10px; background:transparent;");
    lblCount->show();

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
}

void MainWindow::recalc()
{
    lblSoldeVal->setText(dt(ui->spinTarif->value() - ui->spinAcompte->value()));
    lblBenefVal->setText(dt(ui->spinTarif->value() - ui->spinCout->value()));
}

void MainWindow::onSave()
{
    if (m_current < 0) return;
    Service &s = m_services[m_current];
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

    auto makeCard = [this](const QString &prefix, const QString &titre,
                           const QString &main, const QString &tint, int x) {
        auto *card = new QFrame(ui->frameStats);
        card->setObjectName(prefix + "_card");
        card->setProperty("main", main);
        card->setProperty("tint", tint);
        card->setGeometry(x, 34, 480, 246);
        card->setStyleSheet(QString("QFrame#%1_card{background:white; border:1px solid #E2D9C6; border-radius:10px;}"
                                    "QLabel{background:transparent; border:none;}").arg(prefix));
        auto *accent = new QFrame(card);
        accent->setGeometry(1, 1, 478, 5);
        accent->setStyleSheet(QString("background:%1; border:none; border-top-left-radius:9px; border-top-right-radius:9px;").arg(main));

        auto *t = new QLabel(titre.toUpper(), card);
        t->setGeometry(18, 18, 444, 16);
        QFont ft = t->font();
        ft.setBold(true);
        ft.setPixelSize(11);
        ft.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        t->setFont(ft);
        t->setStyleSheet("color:#6B7A88;");

        auto *name = new QLabel("—", card);
        name->setObjectName(prefix + "_name");
        name->setGeometry(18, 40, 444, 36);
        name->setStyleSheet(QString("font-size:26px; font-weight:bold; color:%1;").arg(main));

        auto *sub = new QLabel("", card);
        sub->setObjectName(prefix + "_sub");
        sub->setGeometry(18, 78, 444, 18);
        sub->setStyleSheet("font-size:12px; color:#6B7A88;");

        auto *line = new QFrame(card);
        line->setGeometry(18, 106, 444, 1);
        line->setStyleSheet("background:#EFE7D6; border:none;");

        for (int i = 0; i < 4; ++i) {
            const int y = 118 + i * 31;
            auto *lbl = new QLabel("", card);
            lbl->setObjectName(QString("%1_lbl_%2").arg(prefix).arg(i));
            lbl->setGeometry(18, y, 130, 20);
            lbl->setStyleSheet("font-size:12px; color:#1F3A4D;");

            auto *track = new QFrame(card);
            track->setObjectName(QString("%1_track_%2").arg(prefix).arg(i));
            track->setGeometry(150, y + 3, 230, 14);
            track->setStyleSheet("background:#F1ECE0; border:none; border-radius:7px;");

            auto *fill = new QFrame(track);
            fill->setObjectName(QString("%1_fill_%2").arg(prefix).arg(i));
            fill->setGeometry(0, 0, 0, 14);

            auto *val = new QLabel("", card);
            val->setObjectName(QString("%1_val_%2").arg(prefix).arg(i));
            val->setGeometry(388, y, 74, 20);
            val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            val->setStyleSheet("font-size:12px; font-weight:bold; color:#1F3A4D;");
        }
    };

    makeCard("dem", "Service le plus demandé",  "#8B1E2D", "#D9A9B0", 10);
    makeCard("ren", "Service le plus rentable", "#1F3A4D", "#A9B8C4", 501);
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

            auto *track = card->findChild<QFrame *>(QString("%1_track_%2").arg(prefix).arg(i));
            auto *fill  = card->findChild<QFrame *>(QString("%1_fill_%2").arg(prefix).arg(i));
            int w = (mx > 0 && rows[i].val > 0)
                        ? qMax(8, int(track->width() * rows[i].val / mx)) : 0;
            fill->setGeometry(0, 0, w, 14);
            fill->setVisible(w > 0);
            fill->setStyleSheet(QString("background:%1; border:none; border-radius:7px;")
                                    .arg(i == 0 ? main : tint));
        }
    };

    refresh("dem", false);
    refresh("ren", true);
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
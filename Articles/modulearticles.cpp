#include "modulearticles.h"
#include <QAbstractItemView>
#include "codeqr.h"
#include <QDesktopServices>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QFileInfo>
#include <QLocale>
#include <QMarginsF>
#include <QPageLayout>
#include <QStyle>
#include <QUrl>
#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QComboBox>
#include <QDateEdit>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPageSize>
#include <QPdfWriter>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QSortFilterProxyModel>
#include <QSqlQueryModel>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableView>
#include <QTextDocument>
#include <QTimer>
#include <QValueAxis>
#include <QVBoxLayout>
#include <algorithm>

namespace Art {

// Thème du module (feuille de style portée par ce widget : elle ne touche pas les autres modules)
static const char *kStyle = R"QSS(
QWidget#artRoot, QWidget#artPage, QWidget#artErreur, QScrollArea#artScroll, QWidget#artScrollInner { background: #f5f3ef; }
* { font-family: "Lato", "Segoe UI", "Noto Sans", "DejaVu Sans", sans-serif; font-size: 9pt; color: #1d2b36; }
QScrollArea { background: transparent; border: 0; }
QToolTip { background: #fffdf6; color: #1d2b36; border: 1px solid #1e2f47; }
QLabel#h1 { font-size: 17pt; font-weight: 700; color: #1e2f47; }
QLabel#panelTitle { font-size: 12pt; font-weight: 700; color: #1e2f47; }
QLabel#hint { color: #5d6b75; font-size: 8.5pt; }
QLabel#fieldLabel { font-weight: 700; }
QLabel#err { color: #b3261e; font-size: 8pt; }
QFrame#panel { background: #ffffff; border: 1px solid #e3e0da; border-radius: 4px; }
QFrame#panel QLabel { background: transparent; }
QFrame#kpi { background: #ffffff; border: 1px solid #1e2f47; }
QLabel#kpiHead { background: #1e2f47; color: white; padding: 4px 8px; font-weight: 600; }
QLabel#kpiValue { font-size: 16pt; font-weight: 700; padding: 6px 8px 6px 8px; background: transparent; }
QLabel#resultat { background: #ffffff; border: 1px solid #e3e0da; padding: 12px; }
QPushButton { background: #1e2f47; color: white; border: 0; border-radius: 3px; padding: 6px 14px; min-height: 16px; }
QPushButton:hover { background: #31496b; }
QPushButton:pressed { background: #14202f; }
QPushButton:disabled { background: #b8b3a8; }
QPushButton[variant="red"] { background: #8b2635; }
QPushButton[variant="red"]:hover { background: #721e2b; }
QPushButton[variant="light"] { background: #ece8e0; color: #1d2b36; border: 1px solid #d9d5ce; }
QPushButton[variant="light"]:hover { background: #e2ddd2; }
QLineEdit, QComboBox, QDateEdit { background: white; border: 1px solid #b8b3a8; border-radius: 2px; padding: 2px 6px; min-height: 20px;
    selection-background-color: #1e2f47; selection-color: white; }
QLineEdit:focus, QComboBox:focus, QDateEdit:focus { border: 1px solid #8b2635; }
QLineEdit[invalid="true"], QComboBox[invalid="true"], QDateEdit[invalid="true"] { border: 1px solid #b3261e; background: #fff3f1; }
QComboBox QAbstractItemView { background: white; selection-background-color: #f1dfe2; selection-color: #1d2b36; }
QComboBox::drop-down, QDateEdit::drop-down { border: 0; width: 20px; subcontrol-origin: padding; subcontrol-position: center right; }
QTableView { font-size: 8.5pt; background: #ffffff; border: 1px solid #1e2f47; gridline-color: #ece8e0; alternate-background-color: #faf8f4;
    selection-background-color: #f1dfe2; selection-color: #1d2b36; }
QTableView::item { padding: 2px 4px; border-bottom: 1px solid #ece8e0; }
QTableView::item:hover { background: #f7eef0; }
QHeaderView { background: #1e2f47; }
QHeaderView::section { font-size: 8.5pt; background: #1e2f47; color: white; padding: 5px 6px; border: 0;
    border-right: 1px solid #3a4f6b; font-weight: 600; }
QTableCornerButton::section { background: #1e2f47; border: 0; }
QScrollBar:vertical { background: #ece8e0; width: 12px; margin: 0; }
QScrollBar::handle:vertical { background: #c9c4bb; min-height: 24px; border-radius: 3px; margin: 2px; }
QScrollBar:horizontal { background: #ece8e0; height: 12px; margin: 0; }
QScrollBar::handle:horizontal { background: #c9c4bb; min-width: 24px; border-radius: 3px; margin: 2px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QLabel#toast { color: white; padding: 9px 14px; border-radius: 3px; }
QLabel#toast[type="ok"] { background: #2e7d32; }
QLabel#toast[type="err"] { background: #b3261e; }
QLabel#toast[type="warn"] { background: #b86e00; }
QLabel#toast[type="info"] { background: #1e2f47; }
QMessageBox, QDialog { background: #f5f3ef; }
QMessageBox QLabel, QDialog QLabel { background: transparent; }
QTabWidget#sections::pane { border: 0; background: #f5f3ef; }
QTabBar { background: #f5f3ef; }
QTabBar::tab { background: transparent; color: #1e2f47; padding: 9px 16px; margin-right: 2px; border: 0;
               border-bottom: 3px solid transparent; font-weight: 600; }
QTabBar::tab:hover { background: #ece8e0; }
QTabBar::tab:selected { color: #8b2635; border-bottom: 3px solid #8b2635; }
)QSS";

static QPushButton *bouton(const QString &texte, const char *variante = nullptr)
{
    auto *b = new QPushButton(texte);
    b->setCursor(Qt::PointingHandCursor);
    if (variante) b->setProperty("variant", variante);
    return b;
}

static void marquer(QWidget *w, bool invalide)
{
    w->setProperty("invalid", invalide);
    w->style()->unpolish(w);
    w->style()->polish(w);
}

static QString formatPrix(double v) { return QLocale(QLocale::French).toString(v, 'f', 3) + " DT"; }

ModuleArticles::ModuleArticles(QWidget *parent) : QWidget(parent)
{
    setObjectName("artRoot");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QString::fromUtf8(kStyle));
    setMinimumSize(640, 420);
    construire();
}

void ModuleArticles::construire()
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    stack_ = new QStackedWidget;
    lay->addWidget(stack_);

    // page d'erreur : table ARTICLE absente ou colonnes différentes
    erreurPage_ = new QWidget;
    erreurPage_->setObjectName("artErreur");
    erreurPage_->setAttribute(Qt::WA_StyledBackground, true);
    auto *el = new QVBoxLayout(erreurPage_);
    el->setContentsMargins(40, 40, 40, 40);
    el->addStretch();
    auto *titre = new QLabel("Articles : base de données inaccessible");
    titre->setObjectName("h1");
    erreurTexte_ = new QLabel;
    erreurTexte_->setWordWrap(true);
    erreurTexte_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *retry = bouton("Réessayer", "red");
    el->addWidget(titre);
    el->addWidget(erreurTexte_);
    el->addWidget(retry, 0, Qt::AlignLeft);
    el->addStretch();
    connect(retry, &QPushButton::clicked, this, [this] { charger(); });
    stack_->addWidget(erreurPage_);

    // message flottant (coin inférieur droit)
    toast_ = new QLabel(this);
    toast_->setObjectName("toast");
    toast_->setWordWrap(true);
    toast_->hide();
    toastTimer_ = new QTimer(this);
    toastTimer_->setSingleShot(true);
    connect(toastTimer_, &QTimer::timeout, toast_, &QLabel::hide);
}

bool ModuleArticles::charger()
{
    Article::relireContraintes();     // listes (état, statut, catégorie) = valeurs acceptées par les CHECK de la table
    QString err;
    if (!Article::tableAccessible(&err)) {
        erreurTexte_->setText("La table ARTICLE n'est pas accessible dans Oracle.\n\n" + err +
                              "\n\nVérifiez que le script de la base (v5) a été exécuté, puis cliquez sur « Réessayer ».");
        stack_->setCurrentWidget(erreurPage_);
        return false;
    }
    if (!construit_) {
        auto *page = new QWidget;
        page->setObjectName("artPage");
        page->setAttribute(Qt::WA_StyledBackground, true);
        auto *pl = new QVBoxLayout(page);
        pl->setContentsMargins(20, 14, 20, 12);
        pl->setSpacing(6);
        auto *h1 = new QLabel("Gestion des articles");
        h1->setObjectName("h1");
        auto *sous = new QLabel("Catalogue de l'atelier : fiches, statistiques, estimation du prix et certificats d'authenticité");
        sous->setObjectName("hint");
        pl->addWidget(h1);
        pl->addWidget(sous);
        tabs_ = new QTabWidget;
        tabs_->setObjectName("sections");
        tabs_->setDocumentMode(true);
        tabs_->addTab(construireCatalogue(), "Catalogue");
        tabs_->addTab(construireStatistiques(), "Statistiques");
        tabs_->addTab(construireEstimation(), "Estimation et certificat");
        pl->addWidget(tabs_, 1);
        stack_->addWidget(page);
        construit_ = true;
    }
    stack_->setCurrentIndex(1);
    recharger();
    return true;
}

void ModuleArticles::rafraichir()
{
    if (!construit_) { charger(); return; }
    QString err;
    if (!Article::tableAccessible(&err)) { charger(); return; }
    stack_->setCurrentIndex(1);
    recharger(idCourant_);
}

// ----------------------------------------------------------------------------------------------------------
// Catalogue
// ----------------------------------------------------------------------------------------------------------

QWidget *ModuleArticles::construireCatalogue()
{
    auto *w = new QWidget;
    auto *root = new QHBoxLayout(w);
    root->setContentsMargins(0, 8, 0, 0);
    root->setSpacing(14);

    // ----- colonne gauche : recherche, tri, boutons, tableau -----
    auto *gauche = new QVBoxLayout;
    gauche->setSpacing(8);
    auto *barre = new QHBoxLayout;
    recherche_ = new QLineEdit;
    recherche_->setPlaceholderText("🔍  Rechercher par nom, maison, catégorie...");
    recherche_->setClearButtonEnabled(true);
    tri_ = new QComboBox;
    tri_->addItems({"Trier par : ID", "Trier par : Nom (A → Z)", "Trier par : Maison", "Trier par : Décennie",
                    "Trier par : Prix croissant", "Trier par : Prix décroissant"});
    barre->addWidget(recherche_, 1);
    barre->addWidget(tri_);
    gauche->addLayout(barre);

    auto *actions = new QHBoxLayout;
    auto *btnAjouter = bouton("+ Ajouter", "red");
    btnModifier_ = bouton("Modifier", "light");
    btnSupprimer_ = bouton("Supprimer", "light");
    compte_ = new QLabel;
    compte_->setObjectName("hint");
    actions->addWidget(btnAjouter);
    actions->addWidget(btnModifier_);
    actions->addWidget(btnSupprimer_);
    actions->addStretch();
    actions->addWidget(compte_);
    gauche->addLayout(actions);

    table_ = new QTableView;
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setSortingEnabled(false);
    table_->verticalHeader()->hide();
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->setWordWrap(false);
    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_->setFilterKeyColumn(-1);
    proxy_->setSortCaseSensitivity(Qt::CaseInsensitive);
    table_->setModel(proxy_);
    gauche->addWidget(table_, 1);
    auto *astuce = new QLabel("Cliquez sur une ligne pour ouvrir la fiche de l'article.");
    astuce->setObjectName("hint");
    gauche->addWidget(astuce);
    root->addLayout(gauche, 3);

    // ----- colonne droite : fiche article (formulaire avec contrôle de saisie en direct) -----
    auto *panel = new QFrame;
    panel->setObjectName("panel");
    panel->setMinimumWidth(360);
    panel->setMaximumWidth(430);
    auto *pv = new QVBoxLayout(panel);
    pv->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea;
    scroll->setObjectName("artScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *inner = new QWidget;
    inner->setObjectName("artScrollInner");
    auto *grid = new QGridLayout(inner);
    grid->setContentsMargins(16, 14, 16, 14);
    grid->setHorizontalSpacing(12);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setVerticalSpacing(2);
    ficheTitre_ = new QLabel("Fiche article");
    ficheTitre_->setObjectName("panelTitle");
    grid->addWidget(ficheTitre_, 0, 0, 1, 2);

    auto champ = [&](int ligne, int col, int span, const QString &cle, const QString &libelle, QWidget *editeur) {
        auto *cell = new QWidget;
        cell->setAttribute(Qt::WA_StyledBackground, false);
        auto *cv = new QVBoxLayout(cell);
        cv->setContentsMargins(0, 4, 0, 0);
        cv->setSpacing(2);
        auto *l = new QLabel(libelle);
        l->setObjectName("fieldLabel");
        auto *e = new QLabel;
        e->setObjectName("err");
        e->setWordWrap(true);
        e->setMinimumHeight(14);
        erreurs_[cle] = e;
        cv->addWidget(l);
        cv->addWidget(editeur);
        cv->addWidget(e);
        grid->addWidget(cell, ligne, col, 1, span);
    };
    nom_ = new QLineEdit;       nom_->setMaxLength(80);
    maison_ = new QComboBox;    maison_->setEditable(true);  maison_->lineEdit()->setMaxLength(60);
    decennie_ = new QComboBox;
    categorie_ = new QComboBox; categorie_->setEditable(true); categorie_->lineEdit()->setMaxLength(40);
    etat_ = new QComboBox;
    statut_ = new QComboBox;
    prix_ = new QLineEdit;
    prix_->setPlaceholderText("ex. 4500,500");
    prix_->setValidator(new QRegularExpressionValidator(QRegularExpression(R"(^\d{0,7}([.,]\d{0,3})?$)"), prix_));
    date_ = new QDateEdit(QDate::currentDate());
    date_->setCalendarPopup(true);
    date_->setDisplayFormat("dd/MM/yyyy");
    date_->setMaximumDate(QDate::currentDate());
    service_ = new QComboBox;
    origine_ = new QLineEdit;   origine_->setMaxLength(100);
    origine_->setPlaceholderText("Provenance et historique");
    for (QComboBox *cb : {maison_, decennie_, categorie_, etat_, statut_, service_}) {   // colonnes de même largeur
        cb->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        cb->setMinimumContentsLength(6);
    }
    champ(1, 0, 2, "nom", "Nom de l'article *", nom_);
    champ(2, 0, 2, "maison", "Maison de couture *", maison_);
    champ(3, 0, 1, "decennie", "Décennie *", decennie_);
    champ(3, 1, 1, "categorie", "Catégorie *", categorie_);
    champ(4, 0, 1, "etat", "État *", etat_);
    champ(4, 1, 1, "statut", "Statut *", statut_);
    champ(5, 0, 1, "prix", "Prix (DT) *", prix_);
    champ(5, 1, 1, "date", "Date d'entrée *", date_);
    champ(6, 0, 2, "service", "Service associé *", service_);
    champ(7, 0, 2, "origine", "Origine *", origine_);
    auto *boutons = new QHBoxLayout;
    btnEnregistrer_ = bouton("Enregistrer", "red");
    auto *btnAnnuler = bouton("Annuler", "light");
    boutons->addWidget(btnEnregistrer_, 1);
    boutons->addWidget(btnAnnuler, 1);
    grid->addLayout(boutons, 8, 0, 1, 2);
    grid->setRowStretch(9, 1);
    scroll->setWidget(inner);
    pv->addWidget(scroll);
    root->addWidget(panel, 2);

    // ----- connexions -----
    connect(btnAjouter, &QPushButton::clicked, this, &ModuleArticles::onAjouter);
    connect(btnModifier_, &QPushButton::clicked, this, &ModuleArticles::onModifier);
    connect(btnSupprimer_, &QPushButton::clicked, this, &ModuleArticles::onSupprimer);
    connect(btnEnregistrer_, &QPushButton::clicked, this, &ModuleArticles::onEnregistrer);
    connect(btnAnnuler, &QPushButton::clicked, this, &ModuleArticles::onAnnuler);
    connect(recherche_, &QLineEdit::textChanged, this, &ModuleArticles::onRecherche);
    connect(tri_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModuleArticles::onTri);
    connect(table_->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this] { onLigneChoisie(); });
    connect(nom_, &QLineEdit::textEdited, this, [this] { validerChamp("nom", true); });
    connect(maison_, &QComboBox::editTextChanged, this, [this] { if (maison_->hasFocus() || maison_->lineEdit()->hasFocus()) validerChamp("maison", true); });
    connect(categorie_, &QComboBox::editTextChanged, this, [this] { if (categorie_->lineEdit() && (categorie_->hasFocus() || categorie_->lineEdit()->hasFocus())) validerChamp("categorie", true); });
    connect(categorie_, QOverload<int>::of(&QComboBox::activated), this, [this] { validerChamp("categorie", true); });
    connect(decennie_, QOverload<int>::of(&QComboBox::activated), this, [this] { validerChamp("decennie", true); });
    connect(etat_, QOverload<int>::of(&QComboBox::activated), this, [this] { validerChamp("etat", true); });
    connect(statut_, QOverload<int>::of(&QComboBox::activated), this, [this] { validerChamp("statut", true); });
    connect(service_, QOverload<int>::of(&QComboBox::activated), this, [this] { validerChamp("service", true); });
    connect(prix_, &QLineEdit::textEdited, this, [this] { validerChamp("prix", true); });
    connect(origine_, &QLineEdit::textEdited, this, [this] { validerChamp("origine", true); });
    connect(date_, &QDateEdit::dateChanged, this, [this] { validerChamp("date", true); });
    return w;
}

void ModuleArticles::remplirListes()
{
    auto garder = [](QComboBox *cb) { return cb->currentText(); };
    // listes éditables : valeurs proposées + celles déjà utilisées dans la table
    QStringList maisons = Article::maisons(), cats = Article::categories();
    for (const Article &a : cache_) {
        if (!maisons.contains(a.getMaison(), Qt::CaseInsensitive)) maisons << a.getMaison();
        if (!Article::categoriesFermees() && !cats.contains(a.getCategorie(), Qt::CaseInsensitive)) cats << a.getCategorie();
    }
    maisons.sort(Qt::CaseInsensitive);
    cats.sort(Qt::CaseInsensitive);
    categorie_->setEditable(!Article::categoriesFermees());   // liste imposée par un CHECK : pas de saisie libre
    const QString m = garder(maison_), c = garder(categorie_);
    maison_->blockSignals(true); maison_->clear(); maison_->addItems(maisons); maison_->setCurrentIndex(-1); maison_->setEditText(m); maison_->blockSignals(false);
    categorie_->blockSignals(true); categorie_->clear(); categorie_->addItems(cats); categorie_->setCurrentIndex(-1);
    if (categorie_->isEditable()) categorie_->setEditText(c); else categorie_->setCurrentIndex(categorie_->findText(c));
    categorie_->blockSignals(false);

    auto remplirSimple = [](QComboBox *cb, const QString &vide, const QStringList &items) {
        const QString cur = cb->currentData().toString();
        cb->blockSignals(true);
        cb->clear();
        cb->addItem(vide, "");
        for (const QString &i : items) cb->addItem(i, i);
        const int idx = cb->findData(cur);
        cb->setCurrentIndex(idx < 0 ? 0 : idx);
        cb->blockSignals(false);
    };
    remplirSimple(etat_, "— Choisir —", Article::etats());
    remplirSimple(statut_, "— Choisir —", Article::statuts());

    const QString dc = decennie_->currentData().toString();
    decennie_->blockSignals(true);
    decennie_->clear();
    decennie_->addItem("— Choisir —", "");
    for (int d : Article::decennies()) decennie_->addItem(QString::number(d), QString::number(d));
    decennie_->setCurrentIndex(std::max(0, decennie_->findData(dc)));
    decennie_->blockSignals(false);

    const int sv = service_->currentData().toInt();
    service_->blockSignals(true);
    service_->clear();
    service_->addItem("— Choisir —", 0);
    QString errSrv;
    for (int id : Article::services(&errSrv)) service_->addItem(QString("Service n° %1").arg(id), id);
    service_->setCurrentIndex(std::max(0, service_->findData(sv)));
    service_->blockSignals(false);
    service_->setToolTip(service_->count() <= 1 ? "Aucun service dans la base : créez d'abord un service (module Services)." : QString());
}

void ModuleArticles::recharger(int idASelectionner)
{
    QString err;
    cache_ = Article::tous(&err);
    Article modele;
    QSqlQueryModel *nouveau = modele.afficher();
    proxy_->setSourceModel(nouveau);
    delete model_;
    model_ = nouveau;
    model_->setParent(this);
    // colonnes visibles : ID, Nom, Maison, Décennie, État, Prix, Catégorie, Statut (Origine, Service et Date restent dans la fiche)
    for (int c : {7, 9, 10}) table_->setColumnHidden(c, true);
    onTri(tri_->currentIndex());
    remplirListes();
    compte_->setText(QString("%1 article(s)").arg(cache_.size()));
    mettreAJourStatistiques();

    // liste de l'onglet Estimation
    estArticle_->blockSignals(true);
    estArticle_->clear();
    estArticle_->addItem("— Choisir un article —", 0);
    for (const Article &a : cache_)
        estArticle_->addItem(QString("%1 — %2 (%3)").arg(a.getId()).arg(a.getNom(), a.getMaison()), a.getId());
    estArticle_->blockSignals(false);

    if (idASelectionner > 0 && trouver(idASelectionner)) {
        for (int r = 0; r < proxy_->rowCount(); ++r)
            if (proxy_->index(r, 0).data().toInt() == idASelectionner) { table_->selectRow(r); break; }
        if (const Article *a = trouver(idASelectionner)) remplirFiche(*a);   // fiche toujours à jour apres une relecture
        const int ie = estArticle_->findData(idASelectionner);
        if (ie > 0) estArticle_->setCurrentIndex(ie);
    } else {
        viderFiche();
    }
}

const Article *ModuleArticles::trouver(int id) const
{
    for (const Article &a : cache_) if (a.getId() == id) return &a;
    return nullptr;
}

void ModuleArticles::onTri(int index)
{
    static const int colonnes[] = {0, 1, 2, 3, 5, 5};
    static const Qt::SortOrder ordres[] = {Qt::AscendingOrder, Qt::AscendingOrder, Qt::AscendingOrder, Qt::AscendingOrder,
                                           Qt::AscendingOrder, Qt::DescendingOrder};
    if (index < 0 || index > 5) return;
    proxy_->sort(colonnes[index], ordres[index]);
}

void ModuleArticles::onRecherche(const QString &texte)
{
    proxy_->setFilterFixedString(texte.trimmed());
    compte_->setText(QString("%1 article(s) affiché(s) sur %2").arg(proxy_->rowCount()).arg(cache_.size()));
}

// ----------------------------------------------------------------------------------------------------------
// Fiche : remplissage, lecture, contrôle en direct
// ----------------------------------------------------------------------------------------------------------

void ModuleArticles::viderFiche()
{
    idCourant_ = 0;
    touches_.clear();
    ficheTitre_->setText("Nouvel article");
    nom_->clear();
    prix_->clear();
    origine_->clear();
    maison_->setCurrentIndex(-1); maison_->setEditText(QString());
    categorie_->setCurrentIndex(-1); if (categorie_->isEditable()) categorie_->setEditText(QString());
    decennie_->setCurrentIndex(0);
    etat_->setCurrentIndex(0);
    statut_->setCurrentIndex(0);
    service_->setCurrentIndex(0);
    date_->setDate(QDate::currentDate());
    for (auto it = erreurs_.begin(); it != erreurs_.end(); ++it) it.value()->clear();
    for (QWidget *w : QList<QWidget *>{nom_, prix_, origine_, maison_, categorie_, decennie_, etat_, statut_, service_, date_}) marquer(w, false);
    table_->clearSelection();
    table_->setCurrentIndex(QModelIndex());   // sinon un clic sur la meme ligne ne rouvrirait pas la fiche
    btnEnregistrer_->setText("Enregistrer");
}

void ModuleArticles::remplirFiche(const Article &a)
{
    idCourant_ = a.getId();
    touches_.clear();
    ficheTitre_->setText(QString("Fiche article n° %1").arg(a.getId()));
    nom_->setText(a.getNom());
    maison_->setEditText(a.getMaison());
    if (categorie_->isEditable()) categorie_->setEditText(a.getCategorie());
    else categorie_->setCurrentIndex(categorie_->findText(a.getCategorie()));
    decennie_->setCurrentIndex(std::max(0, decennie_->findData(QString::number(a.getDecennie()))));
    etat_->setCurrentIndex(std::max(0, etat_->findData(a.getEtat())));
    statut_->setCurrentIndex(std::max(0, statut_->findData(a.getStatut())));
    service_->setCurrentIndex(std::max(0, service_->findData(a.getIdService())));
    prix_->setText(QString::number(a.getPrix(), 'f', 3).replace('.', ','));
    date_->setDate(a.getDate().isValid() ? a.getDate() : QDate::currentDate());
    origine_->setText(a.getOrigine());
    for (auto it = erreurs_.begin(); it != erreurs_.end(); ++it) it.value()->clear();
    for (QWidget *w : QList<QWidget *>{nom_, prix_, origine_, maison_, categorie_, decennie_, etat_, statut_, service_, date_}) marquer(w, false);
    btnEnregistrer_->setText("Enregistrer les modifications");
}

Article::Saisie ModuleArticles::lireSaisie() const
{
    Article::Saisie s;
    s.nom = nom_->text();
    s.maison = maison_->currentText();
    s.decennie = decennie_->currentData().toString();
    s.etat = etat_->currentData().toString();
    s.prix = prix_->text();
    s.categorie = categorie_->currentText();
    s.origine = origine_->text();
    s.statut = statut_->currentData().toString();
    s.idService = service_->currentData().toInt();
    s.date = date_->date();
    return s;
}

void ModuleArticles::validerChamp(const QString &cle, bool force)
{
    if (force) touches_.insert(cle);
    QWidget *w = nullptr;
    if (cle == "nom") w = nom_; else if (cle == "maison") w = maison_; else if (cle == "decennie") w = decennie_;
    else if (cle == "etat") w = etat_; else if (cle == "prix") w = prix_; else if (cle == "categorie") w = categorie_;
    else if (cle == "origine") w = origine_; else if (cle == "statut") w = statut_; else if (cle == "service") w = service_;
    else if (cle == "date") w = date_;
    if (!w || !touches_.contains(cle)) return;
    const QString msg = Article::verifierChamp(cle, lireSaisie());
    erreurs_[cle]->setText(msg);
    marquer(w, !msg.isEmpty());
}

bool ModuleArticles::validerTout()
{
    for (const char *c : {"nom", "maison", "decennie", "categorie", "etat", "statut", "prix", "date", "service", "origine"})
        validerChamp(c, true);
    const Article::Saisie s = lireSaisie();
    return Article::verifier(s).isEmpty();
}

void ModuleArticles::onLigneChoisie()
{
    const QModelIndex cur = table_->currentIndex();
    if (!cur.isValid()) return;
    const int id = proxy_->index(cur.row(), 0).data().toInt();
    if (const Article *a = trouver(id)) {
        remplirFiche(*a);
        const int ie = estArticle_->findData(id);
        if (ie > 0) estArticle_->setCurrentIndex(ie);
    }
}

// ----------------------------------------------------------------------------------------------------------
// Actions CRUD (la page n'exécute aucune requête : tout passe par la classe Article)
// ----------------------------------------------------------------------------------------------------------

void ModuleArticles::onAjouter()
{
    viderFiche();
    tabs_->setCurrentIndex(0);
    nom_->setFocus();
}

void ModuleArticles::onModifier()
{
    if (idCourant_ <= 0) {
        montrerToast("Sélectionnez d'abord un article dans le tableau.", "warn");
        return;
    }
    nom_->setFocus();
    nom_->selectAll();
}

void ModuleArticles::onEnregistrer()
{
    if (!validerTout()) {
        montrerToast("Corrigez les champs en rouge avant d'enregistrer.", "warn");
        return;
    }
    Article a;
    a.setId(idCourant_);
    Article::verifier(lireSaisie(), &a);
    a.setId(idCourant_);
    const bool ajout = idCourant_ <= 0;
    if (!(ajout ? a.ajouter() : a.modifier())) {
        montrerToast(a.erreur(), "err");
        return;
    }
    montrerToast(ajout ? "Article ajouté." : "Article modifié.", "ok");
    recharger(a.getId());
}

void ModuleArticles::onAnnuler()
{
    if (idCourant_ > 0) {
        if (const Article *a = trouver(idCourant_)) { remplirFiche(*a); return; }
    }
    viderFiche();
}

void ModuleArticles::onSupprimer()
{
    if (idCourant_ <= 0) {
        montrerToast("Sélectionnez d'abord un article dans le tableau.", "warn");
        return;
    }
    const Article *cible = trouver(idCourant_);
    const QString nomA = cible ? cible->getNom() : QString::number(idCourant_);
    if (QMessageBox::question(this, "Confirmation",
                              QString("Supprimer l'article « %1 » ?\nCette action est définitive.").arg(nomA),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    Article a;
    if (!a.supprimer(idCourant_)) {
        montrerToast(a.erreur(), "err");
        return;
    }
    montrerToast("Article supprimé.", "ok");
    idCourant_ = 0;
    recharger();
}

// ----------------------------------------------------------------------------------------------------------
// Statistiques
// ----------------------------------------------------------------------------------------------------------

QWidget *ModuleArticles::construireStatistiques()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    v->setContentsMargins(0, 10, 0, 0);
    v->setSpacing(10);
    auto *ligne = new QHBoxLayout;
    auto carte = [&](const QString &titre, QLabel *&val) {
        auto *f = new QFrame;
        f->setObjectName("kpi");
        auto *fl = new QVBoxLayout(f);
        fl->setContentsMargins(0, 0, 0, 0);
        fl->setSpacing(0);
        auto *h = new QLabel(titre);
        h->setObjectName("kpiHead");
        val = new QLabel("0");
        val->setObjectName("kpiValue");
        fl->addWidget(h);
        fl->addWidget(val);
        ligne->addWidget(f, 1);
    };
    carte("Articles au catalogue", kpiTotal_);
    carte("Valeur du catalogue", kpiValeur_);
    carte("Prix moyen", kpiMoyen_);
    carte("Articles disponibles", kpiDispo_);
    v->addLayout(ligne);
    graphique_ = new QChartView;
    graphique_->setRenderHint(QPainter::Antialiasing);
    graphique_->setMinimumHeight(260);
    v->addWidget(graphique_, 1);
    return w;
}

void ModuleArticles::mettreAJourStatistiques()
{
    double valeur = 0;
    int dispo = 0;
    QMap<int, int> parDecennie;
    for (const Article &a : cache_) {
        valeur += a.getPrix();
        if (a.getStatut() == "Disponible") ++dispo;
        parDecennie[a.getDecennie()]++;
    }
    kpiTotal_->setText(QString::number(cache_.size()));
    kpiValeur_->setText(formatPrix(valeur));
    kpiMoyen_->setText(cache_.isEmpty() ? "—" : formatPrix(valeur / cache_.size()));
    kpiDispo_->setText(QString::number(dispo));

    auto *chart = new QChart;
    chart->setTitle("Articles par décennie");
    chart->setBackgroundBrush(QBrush(QColor("#f5f3ef")));
    chart->legend()->hide();
    auto *set = new QBarSet("Articles");
    set->setColor(QColor("#8b2635"));
    QStringList cats;
    int maxi = 1;
    for (int d : Article::decennies()) {
        set->append(parDecennie.value(d));
        cats << QString("%1s").arg(d);
        maxi = std::max(maxi, parDecennie.value(d));
    }
    auto *serie = new QBarSeries;
    serie->append(set);
    serie->setLabelsVisible(true);
    chart->addSeries(serie);
    auto *ax = new QBarCategoryAxis;
    ax->append(cats);
    chart->addAxis(ax, Qt::AlignBottom);
    serie->attachAxis(ax);
    auto *ay = new QValueAxis;
    ay->setRange(0, maxi + 1);
    ay->setLabelFormat("%d");
    ay->setTickCount(std::min(maxi + 2, 8));
    chart->addAxis(ay, Qt::AlignLeft);
    serie->attachAxis(ay);
    QChart *ancien = graphique_->chart();
    graphique_->setChart(chart);
    delete ancien;
}

// ----------------------------------------------------------------------------------------------------------
// Estimation du prix + certificat d'authenticité avec QR code (métier innovant)
// ----------------------------------------------------------------------------------------------------------

QWidget *ModuleArticles::construireEstimation()
{
    auto *w = new QWidget;
    auto *root = new QHBoxLayout(w);
    root->setContentsMargins(0, 10, 0, 0);
    root->setSpacing(16);

    // ----- colonne gauche : critères + estimation -----
    auto *panel = new QFrame;
    panel->setObjectName("panel");
    panel->setMaximumWidth(430);
    auto *pl = new QVBoxLayout(panel);
    pl->setContentsMargins(16, 14, 16, 14);
    pl->setSpacing(6);
    auto *t = new QLabel("Estimation et certificat");
    t->setObjectName("panelTitle");
    pl->addWidget(t);
    auto *aide = new QLabel("Choisissez un article du catalogue (ou modifiez les critères pour simuler un autre cas), "
                            "puis lancez l'estimation. Le prix est calculé selon la maison de couture, la décennie et l'état.");
    aide->setObjectName("hint");
    aide->setWordWrap(true);
    pl->addWidget(aide);
    auto etiquette = [&](const QString &texte, QWidget *e) {
        auto *l = new QLabel(texte);
        l->setObjectName("fieldLabel");
        pl->addWidget(l);
        pl->addWidget(e);
    };
    estArticle_ = new QComboBox;
    estMaison_ = new QComboBox;  estMaison_->setEditable(true);
    estDecennie_ = new QComboBox;
    estEtat_ = new QComboBox;
    for (const QString &m : Article::maisons()) estMaison_->addItem(m);
    estMaison_->setCurrentIndex(-1);
    for (int d : Article::decennies()) estDecennie_->addItem(QString::number(d), d);
    for (const QString &e : Article::etats()) estEtat_->addItem(e, e);
    etiquette("Article", estArticle_);
    etiquette("Maison de couture", estMaison_);
    etiquette("Décennie", estDecennie_);
    etiquette("État", estEtat_);
    auto *btnEstimer = bouton("Estimer le prix", "red");
    pl->addSpacing(6);
    pl->addWidget(btnEstimer);
    pl->addStretch();
    root->addWidget(panel, 2);

    // ----- colonne droite : résultat, QR code, boutons -----
    auto *droite = new QVBoxLayout;
    droite->setSpacing(8);
    estResultat_ = new QLabel("<span style='color:#5d6b75'>Aucune estimation pour le moment.</span>");
    estResultat_->setObjectName("resultat");
    estResultat_->setWordWrap(true);
    estResultat_->setTextFormat(Qt::RichText);
    estResultat_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    droite->addWidget(estResultat_, 1);

    // certificat : le QR code renvoie vers la fiche d'origine (PDF) de l'article
    auto *cadreQr = new QFrame;
    cadreQr->setObjectName("panel");
    cadreQr->setMinimumHeight(200);
    auto *ql = new QHBoxLayout(cadreQr);
    ql->setContentsMargins(14, 12, 14, 12);
    ql->setSpacing(14);
    estQr_ = new QLabel;
    estQr_->setFixedSize(150, 150);
    estQr_->setAlignment(Qt::AlignCenter);
    estQr_->setStyleSheet("background: #ffffff; border: 1px dashed #b8b3a8;");
    estQr_->setText("QR code");
    auto *colQr = new QVBoxLayout;
    auto *titreQr = new QLabel("Certificat d'authenticité");
    titreQr->setObjectName("panelTitle");
    auto *texteQr = new QLabel("Le certificat (PDF) porte un QR code. Il renvoie vers la fiche d'origine de l'article, "
                               "un second PDF : provenance, historique, état et prix.");
    texteQr->setObjectName("hint");
    texteQr->setWordWrap(true);
    estLien_ = new QLineEdit;
    estLien_->setPlaceholderText("Adresse en ligne des fiches (facultatif), ex. https://mon-site.tn/fiches");
    estLien_->setToolTip("Si vous publiez les PDF sur un site, saisissez l'adresse du dossier : le QR contiendra ce lien "
                         "et pourra être lu avec un téléphone. Sinon, le QR ouvre le fichier sur cet ordinateur.");
    estLienInfo_ = new QLabel;
    estLienInfo_->setObjectName("hint");
    estLienInfo_->setWordWrap(true);
    colQr->addWidget(titreQr);
    colQr->addWidget(texteQr);
    colQr->addWidget(estLien_);
    colQr->addWidget(estLienInfo_);
    colQr->addStretch();
    ql->addWidget(estQr_);
    ql->addLayout(colQr, 1);
    droite->addWidget(cadreQr);
    {
        QSettings reglages;
        estLien_->setText(reglages.value("articles/lienFiches").toString());
    }

    auto *bas = new QHBoxLayout;
    btnAppliquer_ = bouton("Appliquer ce prix à l'article", "light");
    btnCertificat_ = bouton("Générer le certificat + la fiche d'origine (PDF)");
    btnOuvrirFiche_ = bouton("Ouvrir la fiche d'origine", "light");
    btnAppliquer_->setEnabled(false);
    btnCertificat_->setEnabled(false);
    btnOuvrirFiche_->setEnabled(false);
    bas->addWidget(btnAppliquer_);
    bas->addWidget(btnCertificat_);
    bas->addWidget(btnOuvrirFiche_);
    bas->addStretch();
    droite->addLayout(bas);
    root->addLayout(droite, 3);

    connect(btnEstimer, &QPushButton::clicked, this, &ModuleArticles::onEstimer);
    connect(btnAppliquer_, &QPushButton::clicked, this, &ModuleArticles::onAppliquerPrix);
    connect(btnCertificat_, &QPushButton::clicked, this, &ModuleArticles::onCertificat);
    connect(btnOuvrirFiche_, &QPushButton::clicked, this, &ModuleArticles::onOuvrirFiche);
    connect(estArticle_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModuleArticles::onArticleEstimation);
    connect(estLien_, &QLineEdit::editingFinished, this, [this] {
        QSettings reglages;
        reglages.setValue("articles/lienFiches", estLien_->text().trimmed());
    });
    return w;
}

void ModuleArticles::onArticleEstimation(int)
{
    const int id = estArticle_->currentData().toInt();
    const Article *a = trouver(id);
    btnAppliquer_->setEnabled(false);
    btnCertificat_->setEnabled(a != nullptr);
    btnOuvrirFiche_->setEnabled(false);
    derniereFiche_.clear();
    estQr_->setPixmap(QPixmap());
    estQr_->setText("QR code");
    estLienInfo_->clear();
    if (!a) return;
    estMaison_->setEditText(a->getMaison());
    estDecennie_->setCurrentIndex(std::max(0, estDecennie_->findData(a->getDecennie())));
    estEtat_->setCurrentIndex(std::max(0, estEtat_->findData(a->getEtat())));
    derniereEstimation_ = 0;
    estResultat_->setText(QString("<span style='color:#5d6b75'>Article « %1 » — prix actuel : <b>%2</b>. "
                                  "Cliquez sur « Estimer le prix ».</span>").arg(a->getNom().toHtmlEscaped(), formatPrix(a->getPrix())));
}

void ModuleArticles::onEstimer()
{
    const QString maison = estMaison_->currentText().simplified();
    if (maison.isEmpty()) {
        montrerToast("Choisissez ou saisissez la maison de couture.", "warn");
        return;
    }
    const int dec = estDecennie_->currentData().toInt();
    const Article::Estimation e = Article::estimerPrix(maison, dec, estEtat_->currentData().toString());
    derniereEstimation_ = e.total;
    auto ligne = [](const QString &lib, double v) {
        return QString("<tr><td style='padding:2px 14px 2px 0'>%1</td><td align='right'>%2</td></tr>").arg(lib, formatPrix(v));
    };
    estResultat_->setText(QString("<p style='font-size:11pt'><b>Prix estimé : <span style='color:#8b2635'>%1</span></b></p>"
                                  "<table>%2%3%4%5</table>"
                                  "<p style='color:#5d6b75'>Estimation indicative : base de l'atelier + valeur de la maison, "
                                  "de la décennie et de l'état de conservation.</p>")
                              .arg(formatPrix(e.total), ligne("Base atelier", e.base), ligne("Maison de couture", e.maison),
                                   ligne("Décennie", e.decennie), ligne("État de conservation", e.etat)));
    btnAppliquer_->setEnabled(trouver(estArticle_->currentData().toInt()) != nullptr);
}

void ModuleArticles::onAppliquerPrix()
{
    const Article *src = trouver(estArticle_->currentData().toInt());
    if (!src || derniereEstimation_ <= 0) return;
    Article a = *src;
    a.setPrix(derniereEstimation_);
    if (!a.modifier()) {
        montrerToast(a.erreur(), "err");
        return;
    }
    montrerToast(QString("Prix de « %1 » mis à jour : %2.").arg(a.getNom(), formatPrix(a.getPrix())), "ok");
    recharger(a.getId());
}

void ModuleArticles::onCertificat()
{
    if (!trouver(estArticle_->currentData().toInt())) {
        montrerToast("Choisissez d'abord un article dans la liste.", "warn");
        return;
    }
    const QString dossier = QFileDialog::getExistingDirectory(this, "Dossier où enregistrer le certificat et la fiche d'origine",
                                                              QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (dossier.isEmpty()) return;
    genererDocuments(dossier);
}

// Écrit dans le dossier : la fiche d'origine (PDF) puis le certificat (PDF) dont le QR code renvoie vers cette fiche.
bool ModuleArticles::genererDocuments(const QString &dossier)
{
    const Article *a = trouver(estArticle_->currentData().toInt());
    if (!a) return false;
    const double prix = derniereEstimation_ > 0 ? derniereEstimation_ : a->getPrix();
    const QDir d(dossier);
    const QString cheminFiche = d.filePath(QString("Fiche_origine_%1.pdf").arg(a->getId(), 4, 10, QChar('0')));
    const QString cheminCert = d.filePath(QString("Certificat_%1.pdf").arg(a->getId(), 4, 10, QChar('0')));
    QString err;
    if (!ecrireFicheOrigine(*a, prix, cheminFiche, &err)) { montrerToast(err, "err"); return false; }
    const QString lien = lienFiche(cheminFiche, estLien_->text());
    if (!ecrireCertificat(*a, prix, cheminCert, lien, &err)) { montrerToast(err, "err"); return false; }

    CodeQr qr;
    if (CodeQr::encoder(lien.toUtf8(), qr)) {
        estQr_->setText(QString());
        estQr_->setPixmap(QPixmap::fromImage(qr.image(4, 2)).scaled(estQr_->size() - QSize(8, 8), Qt::KeepAspectRatio, Qt::FastTransformation));
    }
    derniereFiche_ = cheminFiche;
    btnOuvrirFiche_->setEnabled(true);
    estLienInfo_->setText(estLien_->text().trimmed().isEmpty()
                              ? "Le QR ouvre la fiche d'origine sur cet ordinateur : " + QDir::toNativeSeparators(cheminFiche)
                              : "Le QR renvoie vers : " + lien);
    montrerToast("Certificat et fiche d'origine enregistrés dans " + QDir::toNativeSeparators(dossier), "ok");
    return true;
}

void ModuleArticles::onOuvrirFiche()
{
    if (derniereFiche_.isEmpty() || !QFileInfo::exists(derniereFiche_)) {
        montrerToast("Générez d'abord le certificat.", "warn");
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(derniereFiche_));
}

// Adresse contenue dans le QR : adresse en ligne (si saisie) + nom du fichier, sinon fichier local (file:///...)
QString ModuleArticles::lienFiche(const QString &cheminFiche, const QString &base)
{
    QString b = base.trimmed();
    if (b.isEmpty()) return QUrl::fromLocalFile(QFileInfo(cheminFiche).absoluteFilePath()).toString();
    while (b.endsWith('/')) b.chop(1);
    return b + "/" + QFileInfo(cheminFiche).fileName();
}

// Écrit un document HTML dans un PDF A4 (résolution 96 ppp : les tailles en pt et px du HTML sont respectées)
static bool htmlVersPdf(const QString &html, const QMap<QString, QImage> &images, const QString &chemin,
                        const QString &titre, QString *erreur)
{
    QPdfWriter writer(chemin);
    writer.setResolution(96);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(20, 18, 20, 18), QPageLayout::Millimeter);
    writer.setTitle(titre);
    QTextDocument doc;
    for (auto it = images.begin(); it != images.end(); ++it)
        doc.addResource(QTextDocument::ImageResource, QUrl(it.key()), it.value());
    doc.setHtml(html);
    doc.setPageSize(QSizeF(writer.width(), writer.height()));
    doc.print(&writer);
    if (!QFileInfo::exists(chemin) || QFileInfo(chemin).size() == 0) {
        if (erreur) *erreur = "Impossible d'écrire le fichier PDF (dossier protégé ?) : " + chemin;
        return false;
    }
    return true;
}

static QString lignePdf(const QString &l, const QString &v)
{
    return QString("<tr><td style='padding:6px 24px 6px 0;color:#5d6b75'>%1</td><td style='padding:6px 0'><b>%2</b></td></tr>")
        .arg(l, v.toHtmlEscaped());
}

static QString enteteLogo(QMap<QString, QImage> &images)
{
    const QImage logo(":/images/logo.png");
    if (logo.isNull()) return QString();
    images["logo"] = logo.scaledToWidth(420, Qt::SmoothTransformation);
    return "<img src='logo'/><br/>";
}

static QString referencePdf(const Article &a)
{
    return QString("%1-%2").arg(a.getId(), 4, 10, QChar('0')).arg(QDate::currentDate().year());
}

bool ModuleArticles::ecrireFicheOrigine(const Article &a, double prixEstime, const QString &chemin, QString *erreur)
{
    QMap<QString, QImage> images;
    const QString html = QString(
        "<div align='center'>%1"
        "<h1 style='color:#1e2f47;font-size:24pt;margin-bottom:0'>Fiche d'origine</h1>"
        "<p style='color:#8b2635;font-size:12pt;margin-top:2px'>Atelier Vintage Couture — catalogue des articles</p></div>"
        "<hr/>"
        "<h2 style='color:#1e2f47;font-size:16pt'>%2</h2>"
        "<table style='font-size:11pt'>%3%4%5%6%7%8</table>"
        "<h3 style='color:#1e2f47;font-size:13pt;margin-top:18px'>Provenance et historique</h3>"
        "<p style='font-size:11pt;line-height:140%'>%9</p>"
        "<p style='font-size:11pt'>Prix estimé : <b style='color:#8b2635;font-size:13pt'>%10</b></p>"
        "<p style='font-size:9pt;color:#5d6b75'>Fiche n° %11 — éditée le %12 — document consulté depuis le QR code du certificat d'authenticité.</p>")
        .arg(enteteLogo(images), a.getNom().toHtmlEscaped(),
             lignePdf("Maison de couture", a.getMaison()), lignePdf("Décennie", QString::number(a.getDecennie()) + "s"),
             lignePdf("Catégorie", a.getCategorie()), lignePdf("État de conservation", a.getEtat()),
             lignePdf("Statut", a.getStatut()), lignePdf("Date d'entrée à l'atelier", a.getDate().toString("dd/MM/yyyy")))
        .arg(a.getOrigine().toHtmlEscaped(), formatPrix(prixEstime), referencePdf(a), QDate::currentDate().toString("dd/MM/yyyy"));
    return htmlVersPdf(html, images, chemin, QString("Fiche d'origine — %1").arg(a.getNom()), erreur);
}

bool ModuleArticles::ecrireCertificat(const Article &a, double prixEstime, const QString &chemin, const QString &lien,
                                      QString *erreur)
{
    CodeQr qr;
    if (!CodeQr::encoder(lien.toUtf8(), qr)) {
        if (erreur) *erreur = "Le lien est trop long pour être placé dans un QR code.";
        return false;
    }
    QMap<QString, QImage> images;
    const QString logo = enteteLogo(images);
    images["qr"] = qr.image(10, 4);          // 10 pixels par module : net à l'impression comme à l'écran
    const QString html = QString(
        "<div align='center'>%1"
        "<h1 style='color:#1e2f47;font-size:26pt;margin-bottom:0'>Certificat d'authenticité</h1>"
        "<p style='color:#8b2635;font-size:12pt;margin-top:2px'>Atelier Vintage Couture</p></div>"
        "<hr/><p style='font-size:11pt'>L'atelier certifie que l'article ci-dessous figure à son catalogue "
        "et a été examiné par ses soins.</p>"
        "<table style='font-size:11pt'>%2%3%4%5%6%7</table>"
        "<p style='font-size:11pt'>Prix estimé : <b style='color:#8b2635;font-size:14pt'>%8</b></p>"
        "<table style='margin-top:14px'><tr>"
        "<td style='padding-right:18px'><img src='qr' width='170' height='170'/></td>"
        "<td style='vertical-align:middle'><p style='font-size:12pt;color:#1e2f47'><b>Fiche d'origine</b></p>"
        "<p style='font-size:10pt'>Scannez ce QR code pour consulter la fiche d'origine de l'article "
        "(provenance et historique) au format PDF.</p>"
        "<p style='font-size:8pt;color:#5d6b75'>%9</p></td></tr></table>"
        "<p style='font-size:9pt;color:#5d6b75'>Référence du certificat : <b>CERT-%10</b> — délivré le %11</p>"
        "<br/><p style='font-size:10pt'>Signature de l'atelier : ______________________</p>")
        .arg(logo, lignePdf("Article", a.getNom()), lignePdf("Maison de couture", a.getMaison()),
             lignePdf("Décennie", QString::number(a.getDecennie()) + "s"), lignePdf("Catégorie", a.getCategorie()),
             lignePdf("État", a.getEtat()), lignePdf("Statut", a.getStatut()))
        .arg(formatPrix(prixEstime), lien.toHtmlEscaped(), referencePdf(a), QDate::currentDate().toString("dd/MM/yyyy"));
    return htmlVersPdf(html, images, chemin, QString("Certificat d'authenticité — %1").arg(a.getNom()), erreur);
}

// ----------------------------------------------------------------------------------------------------------
// Message flottant
// ----------------------------------------------------------------------------------------------------------

void ModuleArticles::montrerToast(const QString &texte, const QString &type)
{
    toast_->setProperty("type", type);
    toast_->style()->unpolish(toast_);
    toast_->style()->polish(toast_);
    toast_->setText(texte);
    toast_->setMaximumWidth(std::max(240, width() / 2));
    toast_->adjustSize();
    toast_->move(width() - toast_->width() - 20, height() - toast_->height() - 20);
    toast_->raise();
    toast_->show();
    toastTimer_->start(type == "err" ? 6000 : 3500);
}

void ModuleArticles::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (toast_ && toast_->isVisible())
        toast_->move(width() - toast_->width() - 20, height() - toast_->height() - 20);
}

}  // namespace Art

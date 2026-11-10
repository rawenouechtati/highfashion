// pagesmatieres.cpp — implémentation de toutes les pages, du graphique en barres et de l'export PDF.
#include "pagesmatieres.h"
#include "stockrules.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QCollator>
#include <QDate>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>

namespace Mat {


// Cellule de quantité d'un tableau (réception, bon d'achat) : n'accepte que des chiffres.
// Entiers seulement si la ligne porte Qt::UserRole + 1 = true (unité « pièce », « bobine »...).
namespace {
class QuantityDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const override
    {
        auto *e = new QLineEdit(parent);
        Ui2::numberRule(e, index.data(Qt::UserRole + 1).toBool() ? 0 : 3);
        return e;
    }
};
}  // namespace

// ======================= PieChart (graphique circulaire en anneau) =======================
PieChart::PieChart(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void PieChart::setItems(const QVector<BarItem> &items, const QString &centerTop, const QString &centerSub)
{
    items_ = items;
    centerTop_ = centerTop;
    centerSub_ = centerSub;
    updateGeometry();
    update();
}

QSize PieChart::sizeHint() const
{
    return QSize(300, std::max(168, 20 + int(items_.size()) * 26));
}

QSize PieChart::minimumSizeHint() const { return QSize(260, sizeHint().height()); }

void PieChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    double total = 0;
    int shown = 0;
    for (const BarItem &it : items_) {
        total += std::max(0.0, it.value);
        if (it.value > 0) ++shown;
    }
    if (items_.isEmpty() || total <= 0) {
        p.setPen(QColor("#6b7782"));
        p.drawText(rect().adjusted(10, 0, -10, 0), Qt::AlignCenter | Qt::TextWordWrap, empty_);
        return;
    }
    // --- anneau (fin, avec une piste claire derrière et de petits espaces entre les secteurs)
    const int d = std::min(height() - 16, 150);
    const int ring = std::max(12, d / 6);
    const QRectF box(12 + ring / 2.0, (height() - d) / 2.0 + ring / 2.0, d - ring, d - ring);
    QPen track(QColor("#e1e9f0"), ring);
    track.setCapStyle(Qt::FlatCap);
    p.setPen(track);
    p.drawEllipse(box);
    double start = 90.0 * 16;                       // départ en haut, sens horaire
    const double gap = shown > 1 ? 1.6 * 16 : 0;    // petit espace entre deux secteurs
    for (const BarItem &it : items_) {
        if (it.value <= 0) continue;
        const double full = -360.0 * 16 * (it.value / total);
        QPen pen(it.color.isValid() ? it.color : QColor("#1f3a4d"), ring);
        pen.setCapStyle(Qt::FlatCap);
        p.setPen(pen);
        p.drawArc(box, int(start - gap / 2), int(full + gap));
        start += full;
    }
    // --- texte au centre (réduit automatiquement pour tenir dans l'anneau)
    const QRectF inner = box.adjusted(ring / 2.0 + 2, ring / 2.0 + 2, -ring / 2.0 - 2, -ring / 2.0 - 2);
    QFont big = font();
    big.setBold(true);
    big.setPointSizeF(font().pointSizeF() * 1.5);
    while (big.pointSizeF() > 6 && QFontMetrics(big).horizontalAdvance(centerTop_) > inner.width() - 2)
        big.setPointSizeF(big.pointSizeF() - 0.5);
    p.setFont(big);
    p.setPen(QColor("#1f3a4d"));
    const double topShare = centerSub_.isEmpty() ? 1.0 : 0.58;
    p.drawText(QRectF(inner.left(), inner.top(), inner.width(), inner.height() * topShare),
               Qt::AlignHCenter | Qt::AlignBottom, centerTop_);
    if (!centerSub_.isEmpty()) {
        QFont small = font();
        small.setPointSizeF(font().pointSizeF() * 0.85);
        while (small.pointSizeF() > 5 && QFontMetrics(small).horizontalAdvance(centerSub_) > inner.width() - 2)
            small.setPointSizeF(small.pointSizeF() - 0.5);
        p.setFont(small);
        p.setPen(QColor("#6b7782"));
        p.drawText(QRectF(inner.left(), inner.top() + inner.height() * topShare, inner.width(), inner.height() * (1 - topShare)),
                   Qt::AlignHCenter | Qt::AlignTop, centerSub_);
    }
    // --- légende : pastille + nom à gauche, valeur alignée à droite
    const int lx = int(box.right() + ring / 2.0) + 24;
    const int rx = width() - 12;
    const int rowH = 26;
    int y = std::max(4, (height() - int(items_.size()) * rowH) / 2);
    QFont bold = font();
    bold.setBold(true);
    const QFontMetrics fmBold(bold), fm(font());
    for (const BarItem &it : items_) {
        p.setPen(Qt::NoPen);
        p.setBrush(it.color.isValid() ? it.color : QColor("#1f3a4d"));
        p.drawEllipse(QRectF(lx, y + rowH / 2.0 - 5, 10, 10));
        const int valW = fmBold.horizontalAdvance(it.valueText);
        const int labelW = std::max(20, rx - (lx + 18) - valW - 10);
        p.setFont(font());
        p.setPen(QColor("#2b3a45"));
        p.drawText(QRect(lx + 18, y, labelW, rowH), Qt::AlignLeft | Qt::AlignVCenter,
                   fm.elidedText(it.label, Qt::ElideRight, labelW));
        p.setFont(bold);
        p.setPen(QColor("#1f3a4d"));
        p.drawText(QRect(rx - valW, y, valW, rowH), Qt::AlignRight | Qt::AlignVCenter, it.valueText);
        y += rowH;
    }
}

// ======================= DashboardPage =======================
static void clearLayout(QLayout *l)
{
    while (QLayoutItem *it = l->takeAt(0)) {
        delete it->widget();
        delete it;
    }
}

DashboardPage::DashboardPage(StockDb *db, QWidget *parent) : PageBase(db, parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(14, 10, 14, 10);
    outer->setSpacing(8);

    auto *head = new QHBoxLayout;
    head->addWidget(Ui2::title("Tableau de bord"));
    head->addStretch();
    auto *bInv = Ui2::button("Voir l'inventaire", "navy");
    btnNotif_ = Ui2::button("Notifications", "navy");
    head->addWidget(bInv);
    head->addWidget(btnNotif_);
    outer->addLayout(head);
    connect(bInv, &QPushButton::clicked, this, [this] { emit navigate(PgMaterials); });
    connect(btnNotif_, &QPushButton::clicked, this, [this] { emit navigate(PgNotifications); });

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *inner = new QWidget;
    inner->setObjectName("scrollInner");
    auto *il = new QVBoxLayout(inner);
    il->setContentsMargins(0, 0, 6, 0);
    il->setSpacing(10);
    scroll->setWidget(inner);
    outer->addWidget(scroll, 1);

    kpiHost_ = new QWidget;
    auto *kl = new QHBoxLayout(kpiHost_);
    kl->setContentsMargins(0, 0, 0, 0);
    kl->setSpacing(8);
    kpiHost_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    il->addWidget(kpiHost_);

    chartRefs_ = new PieChart;
    chartValue_ = new PieChart;
    replenish_ = new PieChart;

    auto *charts = new QHBoxLayout;
    charts->setSpacing(10);
    charts->addWidget(Ui2::panel("Répartition du stock par catégorie", chartRefs_), 1, Qt::AlignTop);
    charts->addWidget(Ui2::panel("Valeur du stock par catégorie", chartValue_), 1, Qt::AlignTop);
    charts->addWidget(Ui2::panel("Matières à réapprovisionner", replenish_), 1, Qt::AlignTop);
    il->addLayout(charts);

    // --- statistiques : répartition du stock par catégorie et valeur totale (déplacées ici)
    //     + service « Prévision de rupture »
    topCatChart_ = new PieChart;
    topValueChart_ = new PieChart;
    totalLabel_ = new QLabel;
    auto *statsBody = new QWidget;
    auto *sv = new QVBoxLayout(statsBody);
    sv->setContentsMargins(0, 0, 0, 0);
    sv->setSpacing(4);
    auto *cols = new QHBoxLayout;
    cols->setSpacing(18);
    auto column = [&](const QString &title, PieChart *chart) {
        auto *col = new QVBoxLayout;
        col->setSpacing(2);
        auto *lab = new QLabel(title);
        lab->setObjectName("fieldLabel");
        col->addWidget(lab);
        col->addWidget(chart);
        cols->addLayout(col, 1);
    };
    column("Catégorie la plus représentée", topCatChart_);
    column("Catégorie de plus grande valeur", topValueChart_);
    sv->addLayout(cols);
    totalLabel_->setAlignment(Qt::AlignRight);
    sv->addWidget(totalLabel_);

    forecast_ = new PieChart;

    auto *statsRow = new QHBoxLayout;
    statsRow->setSpacing(10);
    statsRow->addWidget(Ui2::panel("Statistiques : répartition du stock par catégorie et valeur totale", statsBody), 2);
    statsRow->addWidget(Ui2::panel("Prévision de rupture (sorties des 30 derniers jours)", forecast_), 1);
    il->addLayout(statsRow);
    il->addStretch();

    connect(db_, &StockDb::changed, this, &DashboardPage::refresh);
    refresh();
}

QFrame *DashboardPage::kpi(const QString &head, const QString &big, const QString &sub, const QString &color) const
{
    auto *f = new QFrame;
    f->setObjectName("kpi");
    auto *l = new QVBoxLayout(f);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    auto *h = new QLabel(head);
    h->setObjectName("kpiHead");
    l->addWidget(h);
    if (!big.isEmpty()) {
        auto *v = new QLabel(big);
        v->setObjectName("kpiValue");
        if (!color.isEmpty()) v->setStyleSheet("color:" + color + ";");
        l->addWidget(v);
    }
    auto *s = new QLabel(sub);
    s->setObjectName("kpiSub");
    s->setTextFormat(Qt::RichText);
    s->setWordWrap(true);
    l->addWidget(s);
    l->addStretch();
    return f;
}

void DashboardPage::refresh()
{
    const Stats s = db_->stats();
    const int alerts = db_->alertCount();
    btnNotif_->setText(alerts ? QString("Notifications (%1)").arg(alerts) : QString("Notifications"));

    // --- indicateurs
    auto *kl = qobject_cast<QHBoxLayout *>(kpiHost_->layout());
    clearLayout(kl);
    QString units;
    for (auto it = s.byUnit.cbegin(); it != s.byUnit.cend(); ++it)
        units += QString("<b>%1</b> %2<br>").arg(Stock::fmtNum(it.value()), it.key());
    if (units.isEmpty()) units = "<span style='color:#5d6b75'>Aucun stock</span>";
    kl->addWidget(kpi("Matières premières", QString::number(s.refs),
                      QString("%1 catégorie(s) · %2 fournisseur(s)").arg(db_->categories().size()).arg(db_->supplierNames().size())), 1);
    kl->addWidget(kpi("Quantité en stock (par unité)", QString(), units), 1);
    kl->addWidget(kpi("Stock faible", QString::number(s.low), "à ou sous le seuil d'alerte", "#b86e00"), 1);
    kl->addWidget(kpi("En rupture", QString::number(s.out), "quantité à zéro", "#b3261e"), 1);
    kl->addWidget(kpi("Valeur totale du stock", Stock::fmtMoney(s.totalValue), "DT (quantité × prix unitaire)"), 1);

    // --- graphiques circulaires (un secteur par catégorie)
    static const QColor palette[] = {QColor("#2f6fb5"), QColor("#17a2a2"), QColor("#6a5acd"), QColor("#4fb3e8"),
                                     QColor("#1f3a4d"), QColor("#8e7cc3"), QColor("#5fc9b0"), QColor("#7a8ea3")};
    QVector<BarItem> refs, values;
    int idx = 0;
    for (const CategoryStat &c : s.byCategory) {
        if (c.refs == 0) continue;
        const QColor col = palette[idx++ % 8];
        refs.push_back({c.categorie, QString("%1 (%2 %)").arg(c.refs).arg(qRound(100.0 * c.refs / qMax(1, s.refs))),
                        QString(), double(c.refs), col});
        values.push_back({c.categorie, Stock::fmtMoney(c.value) + " DT", QString(), c.value, col});
    }
    chartRefs_->setItems(refs, QString::number(s.refs), "références");
    chartValue_->setItems(values, Stock::fmtMoney(s.totalValue), "DT");

    // --- statistiques (catégorie la plus représentée / de plus grande valeur / valeur totale)
    const CategoryStat *top = nullptr, *topV = nullptr;
    for (const CategoryStat &c : s.byCategory) {
        if (c.refs == 0) continue;
        if (!top || c.refs > top->refs) top = &c;
        if (!topV || c.value > topV->value) topV = &c;
    }
    if (!top) {
        topCatChart_->setItems({});
        topValueChart_->setItems({});
    } else {
        // anneau = part de la catégorie dans le total (le reste en gris clair)
        const QColor rest("#d6e0ea");
        const double pr = 100.0 * top->refs / qMax(1, s.refs);
        topCatChart_->setItems({{top->categorie, QString("%1 réf. sur %2").arg(top->refs).arg(s.refs), "", double(top->refs), QColor("#2f6fb5")},
                                {"Autres", QString::number(s.refs - top->refs) + " réf.", "", double(s.refs - top->refs), rest}},
                               QString("%1 %").arg(qRound(pr)), "des références");
        const double pv = s.totalValue > 0 ? 100.0 * topV->value / s.totalValue : 0;
        topValueChart_->setItems({{topV->categorie, Stock::fmtMoney(topV->value) + " DT", "", topV->value, QColor("#1f3a4d")},
                                  {"Autres", Stock::fmtMoney(s.totalValue - topV->value) + " DT", "", std::max(0.0, s.totalValue - topV->value), rest}},
                                 QString("%1 %").arg(qRound(pv)), "de la valeur");
    }
    totalLabel_->setText("<b>Valeur totale du stock :</b> " + Stock::fmtMoney(s.totalValue) + " DT");

    // --- service : prévision de rupture (autonomie = stock / consommation moyenne des 30 derniers jours)
    int already = 0, d7 = 0, d15 = 0, more = 0;
    for (const Material &m : db_->materials()) {
        const Estimate e = Stock::estimate(m, 30, false, 0, QString(), db_->consumedSum(m.nom, 30), 30);
        if (!e.ok) continue;
        if (m.quantite <= 0) ++already;
        else if (e.autonomy <= 7) ++d7;
        else if (e.autonomy <= 15) ++d15;
        else ++more;
    }
    const int followed = already + d7 + d15 + more;
    forecast_->setEmptyText("Pas assez d'historique : aucune sortie sur les 30 derniers jours.");
    if (followed == 0) {
        forecast_->setItems({});
    } else {
        forecast_->setItems({{"Déjà en rupture", QString::number(already), "", double(already), QColor("#b83a52")},
                             {"Autonomie ≤ 7 jours", QString::number(d7), "", double(d7), QColor("#ef8a98")},
                             {"8 à 15 jours", QString::number(d15), "", double(d15), QColor("#f2b134")},
                             {"Plus de 15 jours", QString::number(more), "", double(more), QColor("#2fb28a")}},
                            QString::number(followed), "suivies");
    }

    // --- matières à réapprovisionner : répartition disponible / stock faible / rupture
    const int toOrder = s.low + s.out;
    replenish_->setEmptyText("Aucune matière.");
    if (s.refs == 0) {
        replenish_->setItems({});
    } else {
        replenish_->setItems({{"En rupture", QString::number(s.out), "", double(s.out), QColor("#e05a6d")},
                              {"Stock faible", QString::number(s.low), "", double(s.low), QColor("#f2b134")},
                              {"Disponible", QString::number(s.refs - s.low - s.out), "", double(s.refs - s.low - s.out), QColor("#2fb28a")}},
                             QString::number(toOrder), "à commander");
    }
}

// ======================= MaterialsPage =======================
MaterialsPage::MaterialsPage(StockDb *db, QWidget *parent) : PageBase(db, parent)
{
    buildUi();
    connect(db_, &StockDb::changed, this, &MaterialsPage::refresh);
    refresh();
    loadForm(nullptr);
}

void MaterialsPage::buildUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(14, 10, 14, 10);
    root->setSpacing(14);
    auto *left = new QVBoxLayout;
    left->setSpacing(8);
    root->addLayout(left, 1);

    // --- titre + boutons d'action
    left->addWidget(Ui2::title("Gestion des matières premières"));
    auto *head = new QHBoxLayout;
    auto *bAdd = Ui2::button("+ Ajouter", "red");
    auto *bEdit = Ui2::button("Modifier", "navy");
    auto *bDel = Ui2::button("Supprimer", "navy");
    auto *bPdf = Ui2::button("Exporter PDF", "navy");
    bPdf->setToolTip("Bon d'achat des matières en rupture ou sous le seuil d'alerte");
    auto *bRecv = Ui2::button("Réception de commande", "navy");
    bRecv->setToolTip("Enregistrer d'un coup les quantités reçues pour les matières à réapprovisionner");
    btnNotif_ = Ui2::button("Notifications", "navy");
    for (auto *b : {bAdd, bEdit, bDel, bPdf, bRecv, btnNotif_}) head->addWidget(b);
    head->addStretch();
    left->addLayout(head);
    connect(bAdd, &QPushButton::clicked, this, &MaterialsPage::onAdd);
    connect(bEdit, &QPushButton::clicked, this, &MaterialsPage::onEdit);
    connect(bDel, &QPushButton::clicked, this, &MaterialsPage::onDelete);
    connect(bPdf, &QPushButton::clicked, this, &MaterialsPage::onExportPdf);
    connect(bRecv, &QPushButton::clicked, this, &MaterialsPage::onReceive);
    connect(btnNotif_, &QPushButton::clicked, this, [this] { emit navigate(PgNotifications); });

    // --- recherche et filtres
    search_ = new QLineEdit;
    search_->setPlaceholderText("Rechercher par nom ou catégorie…");
    search_->setClearButtonEnabled(true);
    fltCat_ = new QComboBox;
    fltSup_ = new QComboBox;
    fltStatus_ = new QComboBox;
    fltStatus_->addItem("Tous les statuts", "");
    for (const QString &s : {Stock::Disponible, Stock::StockFaible, Stock::Rupture}) fltStatus_->addItem(s, s);
    auto *row1 = new QHBoxLayout;
    row1->addWidget(search_, 2);
    row1->addWidget(fltCat_, 1);
    row1->addWidget(fltSup_, 1);
    row1->addWidget(fltStatus_, 1);
    left->addLayout(row1);

    sortCombo_ = new QComboBox;
    const QList<QPair<QString, QString>> sorts = {{"Nom", "nom"}, {"Catégorie", "categorie"},
        {"Fournisseur", "fournisseur"}, {"Quantité", "quantite"}, {"Seuil d'alerte", "seuil"},
        {"Prix unitaire", "prix"}, {"Statut", "statut"}};
    for (const auto &s : sorts) sortCombo_->addItem("Trier par : " + s.first, s.second);
    auto *bClear = Ui2::button("Effacer les filtres", "light");
    count_ = new QLabel;
    count_->setObjectName("muted");
    auto *row2 = new QHBoxLayout;
    row2->addWidget(sortCombo_);
    row2->addWidget(bClear);
    row2->addStretch();
    row2->addWidget(count_);
    left->addLayout(row2);

    connect(search_, &QLineEdit::textChanged, this, [this] { fillTable(); });
    for (QComboBox *c : {fltCat_, fltSup_, fltStatus_})
        connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { if (!loading_) fillTable(); });
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (loading_) return;
        sortKey_ = sortCombo_->currentData().toString();
        sortAsc_ = true;
        fillTable();
    });
    connect(bClear, &QPushButton::clicked, this, &MaterialsPage::clearFilters);

    // --- bannière d'alerte
    banner_ = new QPushButton;
    banner_->setObjectName("banner");
    banner_->setCursor(Qt::PointingHandCursor);
    banner_->hide();
    connect(banner_, &QPushButton::clicked, this, [this] { emit navigate(PgNotifications); });
    auto *bannerRow = new QHBoxLayout;
    bannerRow->addWidget(banner_);
    bannerRow->addStretch();
    left->addLayout(bannerRow);

    // --- tableau d'inventaire
    table_ = new QTableWidget;
    Ui2::setupTable(table_, {"Nom", "Catégorie", "Fournisseur", "Quantité", "Unité", "Seuil d'alerte", "Prix unitaire (DT)", "Statut"});
    table_->horizontalHeader()->setSectionsClickable(true);
    connect(table_->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int col) {
        static const QStringList keys = {"nom", "categorie", "fournisseur", "quantite", "unite", "seuil", "prix", "statut"};
        const QString k = keys.value(col);
        sortAsc_ = (sortKey_ == k) ? !sortAsc_ : true;
        sortKey_ = k;
        fillTable();
    });
    connect(table_, &QTableWidget::itemSelectionChanged, this, &MaterialsPage::onSelectionChanged);
    left->addWidget(table_, 1);

    // (les statistiques ont été déplacées dans le « Tableau de bord »)

    // --- fiche matière première (à droite)
    auto *form = new QFrame;
    form->setObjectName("panel");
    form->setFixedWidth(300);
    auto *fl = new QVBoxLayout(form);
    fl->setContentsMargins(14, 12, 14, 12);
    fl->setSpacing(6);
    auto *ft = new QLabel("Fiche matière première");
    ft->setObjectName("formTitle");
    formMode_ = new QLabel;
    formMode_->setObjectName("muted");
    fl->addWidget(ft);
    fl->addWidget(formMode_);

    leNom_ = new QLineEdit;          // le nom est la clé primaire
    leNom_->setMaxLength(80);
    Ui2::nameRule(leNom_);           // lettres uniquement : un chiffre ou un symbole ne s'écrit pas
    cbCat_ = new QComboBox;          // catégorie et fournisseur : attributs texte -> listes éditables
    cbSup_ = new QComboBox;
    for (QComboBox *cb : {cbCat_, cbSup_}) {
        cb->setEditable(true);
        cb->setInsertPolicy(QComboBox::NoInsert);
        cb->setMaxCount(500);
        cb->lineEdit()->setMaxLength(cb == cbCat_ ? 40 : 60);
        Ui2::nameRule(cb->lineEdit());   // lettres uniquement
        cb->lineEdit()->setPlaceholderText("Choisir / saisir");
    }
    leQty_ = new QLineEdit;
    leQty_->setPlaceholderText("0");
    Ui2::numberRule(leQty_, 3);      // chiffres seulement (entiers si l'unité est « pièce », « bobine »... : voir updateQtyRule)
    cbUnit_ = new QComboBox;
    leSeuil_ = new QLineEdit;
    leSeuil_->setPlaceholderText("0");
    Ui2::numberRule(leSeuil_, 3);
    lePrix_ = new QLineEdit;
    lePrix_->setPlaceholderText("0,000");
    Ui2::numberRule(lePrix_, 3);
    leStatut_ = new QLineEdit;
    leStatut_->setReadOnly(true);
    leValeur_ = new QLineEdit;
    leValeur_->setReadOnly(true);
    search_->setObjectName("fltQ");
    table_->setObjectName("matTable");
    leNom_->setObjectName("fNom"); leQty_->setObjectName("fQty");
    leSeuil_->setObjectName("fSeuil"); lePrix_->setObjectName("fPrix"); leStatut_->setObjectName("fStatut");
    leValeur_->setObjectName("fValeur"); cbCat_->setObjectName("fCat"); cbSup_->setObjectName("fSup"); cbUnit_->setObjectName("fUnit");

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);
    auto put = [&](const QString &key, const QString &label, QWidget *w, int r, int c, int span) {
        auto f = Ui2::field(label, w);
        grid->addWidget(f.container, r, c, 1, span, Qt::AlignTop);
        if (!key.isEmpty()) errors_.add(key, w, f.error);
    };
    put("", "Statut", leStatut_, 0, 0, 2);
    put("nom", "Nom", leNom_, 1, 0, 2);
    put("categorie", "Catégorie", cbCat_, 2, 0, 1);
    put("fournisseur", "Fournisseur", cbSup_, 2, 1, 1);
    put("quantite", "Quantité en stock", leQty_, 3, 0, 1);
    put("unite", "Unité", cbUnit_, 3, 1, 1);
    put("seuil", "Seuil d'alerte", leSeuil_, 4, 0, 1);
    put("prix", "Prix unitaire (DT)", lePrix_, 4, 1, 1);
    put("", "Valeur du stock (quantité × prix)", leValeur_, 5, 0, 2);
    fl->addLayout(grid);

    auto *actions = new QHBoxLayout;
    auto *bSave = Ui2::button("Enregistrer", "red");
    auto *bCancel = Ui2::button("Annuler", "navy");
    actions->addWidget(bSave);
    actions->addWidget(bCancel);
    actions->addStretch();
    fl->addLayout(actions);
    fl->addStretch();
    root->addWidget(form, 0, Qt::AlignTop);

    connect(bSave, &QPushButton::clicked, this, &MaterialsPage::onSave);
    connect(bCancel, &QPushButton::clicked, this, &MaterialsPage::onCancel);
    for (QLineEdit *e : {leNom_, leQty_, leSeuil_, lePrix_})
        connect(e, &QLineEdit::returnPressed, this, &MaterialsPage::onSave);
    connect(cbUnit_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateQtyRule(); });
    for (QLineEdit *e : {leQty_, leSeuil_, lePrix_}) connect(e, &QLineEdit::textChanged, this, &MaterialsPage::liveForm);
}

// ======================= données affichées =======================
static int cmpStr(const QString &a, const QString &b)
{
    static QCollator coll;
    static bool init = false;
    if (!init) { coll.setNumericMode(true); coll.setCaseSensitivity(Qt::CaseInsensitive); init = true; }
    return coll.compare(a, b);
}

QVector<Material> MaterialsPage::viewRows() const
{
    QVector<Material> rows;
    const QString q = Stock::normalize(search_->text());
    const QString cat = fltCat_->currentData().toString();
    const QString sup = fltSup_->currentData().toString();
    const QString st = fltStatus_->currentData().toString();
    for (const Material &m : db_->materials()) {
        if (!q.isEmpty() && !Stock::normalize(m.nom).contains(q) && !Stock::normalize(m.categorie).contains(q)) continue;
        if (!cat.isEmpty() && m.categorie != cat) continue;
        if (!sup.isEmpty() && m.fournisseur != sup) continue;
        if (!st.isEmpty() && Stock::statusOf(m.quantite, m.seuil) != st) continue;
        rows.push_back(m);
    }
    const QString key = sortKey_;
    std::stable_sort(rows.begin(), rows.end(), [&](const Material &a, const Material &b) {
        int c = 0;
        if (key == "quantite") c = (a.quantite > b.quantite) - (a.quantite < b.quantite);
        else if (key == "seuil") c = (a.seuil > b.seuil) - (a.seuil < b.seuil);
        else if (key == "prix") c = (a.prix > b.prix) - (a.prix < b.prix);
        else if (key == "statut") c = Stock::statusRank(a) - Stock::statusRank(b);
        else if (key == "nom") c = cmpStr(a.nom, b.nom);
        else if (key == "categorie") c = cmpStr(a.categorie, b.categorie);
        else if (key == "fournisseur") c = cmpStr(a.fournisseur, b.fournisseur);
        else if (key == "unite") c = cmpStr(a.unite, b.unite);
        else c = cmpStr(a.nom, b.nom);
        if (c == 0) c = cmpStr(a.nom, b.nom);
        return sortAsc_ ? c < 0 : c > 0;
    });
    return rows;
}

void MaterialsPage::fillCombos()
{
    auto rebuild = [](QComboBox *cb, const QString &placeholder, const QStringList &items) {
        const QString cur = cb->currentData().toString();
        cb->blockSignals(true);
        cb->clear();
        cb->addItem(placeholder, "");
        for (const QString &i : items) cb->addItem(i, i);
        const int idx = cb->findData(cur);
        cb->setCurrentIndex(idx < 0 ? 0 : idx);
        cb->blockSignals(false);
    };
    const QStringList cats = db_->categories(), sups = db_->supplierNames();
    QStringList units = Stock::defaultUnits();
    for (const Material &m : db_->materials())
        if (!units.contains(m.unite)) units << m.unite;
    rebuild(fltCat_, "Toutes les catégories", cats);
    rebuild(fltSup_, "Tous les fournisseurs", sups);
    rebuild(cbUnit_, "— Choisir —", units);
    // listes éditables : on garde le texte en cours de saisie
    auto rebuildEditable = [](QComboBox *cb, const QStringList &items) {
        const QString cur = cb->currentText();
        cb->blockSignals(true);
        cb->clear();
        cb->addItems(items);
        cb->setCurrentIndex(-1);
        cb->setEditText(cur);
        cb->blockSignals(false);
    };
    rebuildEditable(cbCat_, cats);
    rebuildEditable(cbSup_, sups);
}

void MaterialsPage::fillTable()
{
    const QVector<Material> rows = viewRows();
    const int total = db_->materials().size();
    // flèche de tri dans le texte de l'en-tête (évite la place réservée par l'indicateur Qt)
    static const QStringList keys = {"nom", "categorie", "fournisseur", "quantite", "unite", "seuil", "prix", "statut"};
    static const QStringList labels = {"Nom", "Catégorie", "Fournisseur", "Quantité", "Unité", "Seuil d'alerte", "Prix unitaire (DT)", "Statut"};
    const int sortCol = keys.indexOf(sortKey_);
    for (int c = 0; c < labels.size(); ++c)
        if (auto *h = table_->horizontalHeaderItem(c))
            h->setText(labels[c] + (c == sortCol ? (sortAsc_ ? " ▲" : " ▼") : QString()));
    const bool wasLoading = loading_;
    loading_ = true;
    table_->clearSelection();
    table_->clearSpans();
    table_->setRowCount(0);
    sortCombo_->blockSignals(true);
    sortCombo_->setCurrentIndex(std::max(0, sortCombo_->findData(sortKey_)));
    sortCombo_->blockSignals(false);
    count_->setText(QString("%1 sur %2 matière(s)").arg(rows.size()).arg(total));

    if (rows.isEmpty()) {
        Ui2::showEmptyRow(table_, total == 0
            ? "Aucune matière première enregistrée.\nCliquez sur « Ajouter » pour créer la première fiche."
            : "Aucune matière ne correspond à ces critères.\nUtilisez « Effacer les filtres ».");
        loading_ = wasLoading;
        return;
    }
    table_->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r) {
        const Material &m = rows[r];
        auto *nomItem = Ui2::cell(m.nom);
        nomItem->setData(Qt::UserRole, m.nom);   // clé primaire
        table_->setItem(r, 0, nomItem);
        table_->setItem(r, 1, Ui2::cell(m.categorie));
        table_->setItem(r, 2, Ui2::cell(m.fournisseur));
        table_->setItem(r, 3, Ui2::cell(Stock::fmtNum(m.quantite), true));
        table_->setItem(r, 4, Ui2::cell(m.unite));
        table_->setItem(r, 5, Ui2::cell(Stock::fmtNum(m.seuil), true));
        table_->setItem(r, 6, Ui2::cell(Stock::fmtMoney(m.prix), true));
        table_->setItem(r, 7, Ui2::statusCell(Stock::statusOf(m.quantite, m.seuil)));
    }
    loading_ = wasLoading;
    highlightSelected();
}

void MaterialsPage::highlightSelected()
{
    if (selectedId_.isEmpty()) return;
    const bool was = loading_;
    loading_ = true;
    for (int r = 0; r < table_->rowCount(); ++r) {
        QTableWidgetItem *it = table_->item(r, 0);
        if (it && it->data(Qt::UserRole).toString().compare(selectedId_, Qt::CaseInsensitive) == 0) {
            table_->selectRow(r);
            break;
        }
    }
    loading_ = was;
}

void MaterialsPage::updateBanner()
{
    const QVector<Alert> alerts = db_->alerts();
    const int n = alerts.size();
    btnNotif_->setText(n ? QString("Notifications (%1)").arg(n) : QString("Notifications"));
    if (!n) { banner_->hide(); return; }
    int out = 0;
    for (const Alert &a : alerts) out += a.statut == Stock::Rupture;
    banner_->setText(QString("⚠ %1 matière%2 au niveau ou sous le seuil d'alerte%3.")
                         .arg(n).arg(n > 1 ? "s sont" : " est", out ? QString(" (dont %1 en rupture)").arg(out) : QString()));
    banner_->show();
}

void MaterialsPage::refresh()
{
    loading_ = true;
    fillCombos();
    loading_ = false;
    fillTable();
    updateBanner();
    // garde la fiche à jour si la matière sélectionnée a changé ailleurs (mouvement de stock, etc.)
    if (!isNew_) {
        Material m;
        if (db_->getMaterial(selectedId_, &m)) fillForm(m);
        else loadForm(nullptr);
    }
}

void MaterialsPage::selectMaterial(const QString &id)
{
    loading_ = true;
    search_->clear();
    fltCat_->setCurrentIndex(0);
    fltSup_->setCurrentIndex(0);
    fltStatus_->setCurrentIndex(0);
    loading_ = false;
    Material m;
    if (db_->getMaterial(id, &m)) loadForm(&m);
    else fillTable();
}

void MaterialsPage::clearFilters()
{
    loading_ = true;
    search_->clear();
    fltCat_->setCurrentIndex(0);
    fltSup_->setCurrentIndex(0);
    fltStatus_->setCurrentIndex(0);
    loading_ = false;
    fillTable();
}

QString MaterialsPage::filterDescription() const
{
    QStringList parts;
    if (!search_->text().trimmed().isEmpty()) parts << QString("recherche « %1 »").arg(search_->text().trimmed());
    if (!fltCat_->currentData().toString().isEmpty()) parts << "catégorie : " + fltCat_->currentText();
    if (!fltSup_->currentData().toString().isEmpty()) parts << "fournisseur : " + fltSup_->currentText();
    if (!fltStatus_->currentData().toString().isEmpty()) parts << "statut : " + fltStatus_->currentText();
    return parts.isEmpty() ? QString("Aucun filtre (toutes les matières)") : "Filtres actifs — " + parts.join(" ; ");
}

// ======================= fiche =======================
void MaterialsPage::fillForm(const Material &m)
{
    leNom_->setText(m.nom);
    cbCat_->setEditText(m.categorie);
    cbSup_->setEditText(m.fournisseur);
    leQty_->setText(QString::number(m.quantite, 'g', 12));
    cbUnit_->setCurrentIndex(std::max(0, cbUnit_->findData(m.unite)));
    leSeuil_->setText(QString::number(m.seuil, 'g', 12));
    lePrix_->setText(QString::number(m.prix, 'g', 12));
    liveForm();
}

void MaterialsPage::loadForm(const Material *m)
{
    errors_.clear();
    if (m) {
        isNew_ = false;
        selectedId_ = m->nom;
        fillForm(*m);
        formMode_->setText("Modification : " + m->nom);
    } else {
        isNew_ = true;
        selectedId_.clear();
        for (QLineEdit *e : {leNom_, leQty_, leSeuil_, lePrix_}) e->clear();
        cbCat_->setCurrentIndex(-1); cbCat_->clearEditText();
        cbSup_->setCurrentIndex(-1); cbSup_->clearEditText();
        cbUnit_->setCurrentIndex(0);
        formMode_->setText("Nouvelle matière première");
        loading_ = true;
        table_->clearSelection();
        loading_ = false;
    }
    updateQtyRule();
    liveForm();
    if (m) highlightSelected();
}

// La quantité est un entier pour « pièce », « bobine »... et un nombre à 3 décimales au maximum sinon.
void MaterialsPage::updateQtyRule()
{
    Ui2::numberRule(leQty_, Stock::isIntUnit(cbUnit_->currentData().toString()) ? 0 : 3);
}

void MaterialsPage::liveForm()
{
    double q = 0, s = 0, p = 0;
    const bool okQ = Stock::parseNum(leQty_->text(), &q) && q >= 0;
    const bool okS = Stock::parseNum(leSeuil_->text(), &s) && s >= 0;
    const bool okP = Stock::parseNum(lePrix_->text(), &p) && p >= 0;
    if (okQ && okS) {
        const QString st = Stock::statusOf(q, s);
        leStatut_->setText(st);
        leStatut_->setStyleSheet(QString("font-weight:bold;color:%1;").arg(Stock::statusColor(st)));
    } else {
        leStatut_->setText("—");
        leStatut_->setStyleSheet(QString());
    }
    leValeur_->setText(okQ && okP ? Stock::fmtMoney(q * p) + " DT" : "—");
}

void MaterialsPage::onSelectionChanged()
{
    if (loading_) return;
    const auto sel = table_->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    QTableWidgetItem *it = table_->item(sel.first().row(), 0);
    if (!it) return;
    Material m;
    if (db_->getMaterial(it->data(Qt::UserRole).toString(), &m)) loadForm(&m);
}

void MaterialsPage::onAdd()
{
    loadForm(nullptr);
    leNom_->setFocus();
    emit message("Saisissez la nouvelle matière puis cliquez sur « Enregistrer ».", "info");
}

void MaterialsPage::onEdit()
{
    Material m;
    if (selectedId_.isEmpty() || !db_->getMaterial(selectedId_, &m)) {
        emit message("Sélectionnez d'abord une matière dans le tableau.", "err");
        return;
    }
    loadForm(&m);
    leNom_->setFocus();
    leNom_->selectAll();
}

void MaterialsPage::onDelete()
{
    Material m;
    if (selectedId_.isEmpty() || !db_->getMaterial(selectedId_, &m)) {
        emit message("Sélectionnez d'abord une matière dans le tableau.", "err");
        return;
    }
    if (!Ui2::confirm(this, "Confirmation",
                      QString("Supprimer la matière « %1 » ?\nCette action est définitive. "
                              "L'historique de ses mouvements est conservé.").arg(m.nom),
                      "Supprimer"))
        return;
    Matiere M;
    if (!M.supprimer(m.id)) {
        emit message("Suppression non effectuée.\n" + M.erreur(), "err");
        return;
    }
    db_->notifyChanged();                                // les pages relisent la table
    loadForm(nullptr);
    emit message(QString("Matière « %1 » supprimée.").arg(m.nom), "ok");
}

void MaterialsPage::onCancel()
{
    Material m;
    if (!isNew_ && db_->getMaterial(selectedId_, &m)) loadForm(&m);
    else loadForm(nullptr);
}

void MaterialsPage::onSave()
{
    Stock::RawMaterial raw{leNom_->text(), cbCat_->currentText(), cbSup_->currentText(),
                           leQty_->text(), cbUnit_->currentData().toString(), leSeuil_->text(), lePrix_->text()};
    // le nom est la clé primaire : il doit être unique (sans tenir compte des majuscules ni des accents),
    // en ignorant la matière qu'on est en train de modifier
    const QString key = Stock::normalize(raw.nom.simplified());
    bool used = false;
    for (const Material &o : db_->materials())
        if (Stock::normalize(o.nom) == key && (isNew_ || o.nom != selectedId_)) { used = true; break; }
    Material m;
    const Stock::Errors errs = Stock::validateMaterial(raw, used, &m);
    if (!errs.isEmpty()) {
        errors_.show(errs);
        emit message("Veuillez corriger les champs signalés.", "err");
        return;
    }
    errors_.clear();
    Material current;
    if (!isNew_ && db_->getMaterial(selectedId_, &current)) m.id = current.id;   // ID de la matière modifiée
    Matiere M(m);                                        // objet Matière puis ajouter() ou modifier()
    const bool ok = isNew_ ? M.ajouter() : M.modifier();
    if (!ok) {
        emit message("Échec de l'enregistrement dans la base de données." +
                         (M.erreur().isEmpty() ? QString() : "\n" + M.erreur()), "err");
        return;
    }
    if (!isNew_) db_->materialRenamed(selectedId_, m.nom);   // l'historique des mouvements suit le renommage
    db_->notifyChanged();                                // les pages relisent la table
    emit message(QString(isNew_ ? "Matière « %1 » ajoutée." : "Matière « %1 » modifiée.").arg(m.nom), "ok");
    Material saved;
    if (db_->getMaterial(m.nom, &saved)) loadForm(&saved);
}

// Quantité suggérée pour le bon d'achat : remonte le stock à 2 × le seuil, ou couvre 30 jours de
// consommation au-dessus du seuil si c'est plus. Arrondie vers le haut (entier pour pièce, bobine...).
static double suggestedQty(const Material &m, double consumed30)
{
    const double target = std::max(2.0 * m.seuil, m.seuil + consumed30);
    double q = target - m.quantite;
    if (q <= 0) q = 1;
    return Stock::isIntUnit(m.unite) ? std::ceil(q - 1e-9) : std::ceil(q * 100.0 - 1e-9) / 100.0;
}

// « Réception de commande » : la livraison est arrivée ; on saisit les quantités reçues et chaque ligne
// remplie devient une entrée de stock (mouvement « Entrée ») via StockDb::recordMovement.
void MaterialsPage::onReceive()
{
    QVector<Material> need;
    for (const Material &m : db_->materials())
        if (Stock::statusOf(m.quantite, m.seuil) != Stock::Disponible) need.push_back(m);
    if (need.isEmpty()) {
        emit message("Aucune matière en rupture ou sous le seuil : rien à réceptionner.", "info");
        return;
    }
    std::sort(need.begin(), need.end(), [](const Material &a, const Material &b) {
        int c = cmpStr(a.fournisseur, b.fournisseur);
        return c != 0 ? c < 0 : cmpStr(a.nom, b.nom) < 0;
    });

    QDialog dlg(this);
    dlg.setWindowTitle("Réception de commande");
    dlg.resize(980, 440);
    auto *l = new QVBoxLayout(&dlg);
    l->addWidget(Ui2::hint("Saisissez la quantité reçue pour chaque matière livrée (double-clic sur la colonne « Qté reçue »). "
                           "Les lignes laissées vides sont ignorées. Chaque quantité est ajoutée au stock et enregistrée dans l'historique."));

    auto *t = new QTableWidget;
    Ui2::setupTable(t, {"Matière", "Fournisseur", "Statut", "Stock actuel", "Qté reçue", "Stock après"});
    t->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
    t->horizontalHeader()->setStretchLastSection(false);
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    t->setItemDelegateForColumn(4, new QuantityDelegate(t));   // « Qté reçue » : chiffres seulement
    t->setRowCount(need.size());
    for (int r = 0; r < need.size(); ++r) {
        const Material &m = need[r];
        t->setItem(r, 0, Ui2::cell(m.nom));
        t->setItem(r, 1, Ui2::cell(m.fournisseur));
        t->setItem(r, 2, Ui2::statusCell(Stock::statusOf(m.quantite, m.seuil)));
        t->setItem(r, 3, Ui2::cell(Stock::fmtNum(m.quantite) + " " + m.unite, true));
        auto *q = Ui2::cell(QString(), true);
        q->setFlags(q->flags() | Qt::ItemIsEditable);
        q->setData(Qt::UserRole + 1, Stock::isIntUnit(m.unite));   // entiers seulement pour pièce, bobine...
        t->setItem(r, 4, q);
        t->setItem(r, 5, Ui2::cell(QString("—"), true));
    }
    l->addWidget(t, 1);

    // aperçu « stock après » et détection des quantités invalides
    bool busy = false;
    auto refreshPreview = [&](int *badRow, int *filled) {
        busy = true;
        if (badRow) *badRow = -1;
        int n = 0;
        for (int r = 0; r < need.size(); ++r) {
            const Material &m = need[r];
            const QString txt = t->item(r, 4)->text().trimmed();
            double qty = 0;
            const bool empty = txt.isEmpty();
            const bool valid = !empty && Stock::parseNum(txt, &qty) && qty > 0 &&
                               (!Stock::isIntUnit(m.unite) || qty == std::floor(qty));
            t->item(r, 5)->setText(valid ? Stock::fmtNum(m.quantite + qty) + " " + m.unite : QString("—"));
            t->item(r, 4)->setForeground(QColor(empty || valid ? "#1d2b36" : "#b3261e"));
            if (valid) ++n;
            else if (!empty && badRow && *badRow < 0) *badRow = r;
        }
        if (filled) *filled = n;
        busy = false;
    };
    connect(t, &QTableWidget::itemChanged, &dlg, [&](QTableWidgetItem *) { if (!busy) refreshPreview(nullptr, nullptr); });

    auto *bb = new QDialogButtonBox;
    auto *ok = bb->addButton("Enregistrer la réception", QDialogButtonBox::AcceptRole);
    ok->setProperty("variant", "red");
    auto *cancel = bb->addButton("Annuler", QDialogButtonBox::RejectRole);
    cancel->setProperty("variant", "navy");
    l->addWidget(bb);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
        int bad = -1, filled = 0;
        refreshPreview(&bad, &filled);
        if (bad >= 0) {
            t->selectRow(bad);
            QMessageBox::warning(&dlg, "Quantité invalide",
                                 "Une quantité reçue est invalide (nombre supérieur à zéro ; entier pour pièce, bobine…).");
            return;
        }
        if (filled == 0) {
            QMessageBox::warning(&dlg, "Réception de commande", "Saisissez au moins une quantité reçue.");
            return;
        }
        dlg.accept();
    });
    if (dlg.exec() != QDialog::Accepted) return;

    int done = 0;
    for (int r = 0; r < need.size(); ++r) {
        const QString txt = t->item(r, 4)->text().trimmed();
        if (txt.isEmpty()) continue;
        const Stock::Errors e = db_->recordMovement(need[r].nom, "Entrée", txt, QDate::currentDate(),
                                                    "Réception de commande — " + need[r].fournisseur);
        if (e.isEmpty()) ++done;
        else emit message(need[r].nom + " : " + e.values().first(), "err");
    }
    if (done > 0) emit message(QString("Réception enregistrée : %1 matière(s) réapprovisionnée(s).").arg(done), "ok");
    refresh();
}

// « Exporter PDF » : bon d'achat des matières en rupture ou au niveau / sous le seuil d'alerte.
void MaterialsPage::onExportPdf()
{
    QVector<Material> need;
    for (const Material &m : db_->materials())
        if (Stock::statusOf(m.quantite, m.seuil) != Stock::Disponible) need.push_back(m);
    if (need.isEmpty()) {
        emit message("Aucune matière en rupture ou sous le seuil : pas de bon d'achat à créer.", "info");
        return;
    }
    std::sort(need.begin(), need.end(), [](const Material &a, const Material &b) {
        int c = cmpStr(a.fournisseur, b.fournisseur);
        if (c == 0) c = Stock::statusRank(a) - Stock::statusRank(b);
        if (c == 0) c = cmpStr(a.nom, b.nom);
        return c < 0;
    });

    QDialog dlg(this);
    dlg.setWindowTitle("Bon d'achat — matières à commander");
    dlg.resize(1080, 440);
    auto *l = new QVBoxLayout(&dlg);
    l->addWidget(Ui2::hint("Matières en rupture ou au niveau / sous leur seuil d'alerte, regroupées par fournisseur. "
                           "La quantité suggérée remonte le stock à 2 × le seuil (ou couvre 30 jours de consommation si c'est plus). "
                           "Double-cliquez sur une quantité pour la modifier, décochez une ligne pour l'exclure."));

    auto *t = new QTableWidget;
    Ui2::setupTable(t, {"", "Matière", "Fournisseur", "Statut", "Stock", "Qté à commander", "Prix unit. (DT)", "Total (DT)"});
    t->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    t->horizontalHeader()->setStretchLastSection(false);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    t->setItemDelegateForColumn(5, new QuantityDelegate(t));   // « Qté à commander » : chiffres seulement
    t->setRowCount(need.size());
    for (int r = 0; r < need.size(); ++r) {
        const Material &m = need[r];
        auto *chk = new QTableWidgetItem;
        chk->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        chk->setCheckState(Qt::Checked);
        chk->setData(Qt::UserRole, m.nom);
        t->setItem(r, 0, chk);
        t->setItem(r, 1, Ui2::cell(m.nom));
        t->setItem(r, 2, Ui2::cell(m.fournisseur));
        t->setItem(r, 3, Ui2::statusCell(Stock::statusOf(m.quantite, m.seuil)));
        t->setItem(r, 4, Ui2::cell(Stock::fmtNum(m.quantite) + " " + m.unite, true));
        auto *q = Ui2::cell(Stock::fmtNum(suggestedQty(m, db_->consumedSum(m.nom, 30))), true);
        q->setFlags(q->flags() | Qt::ItemIsEditable);
        q->setData(Qt::UserRole + 1, Stock::isIntUnit(m.unite));   // entiers seulement pour pièce, bobine...
        t->setItem(r, 5, q);
        t->setItem(r, 6, Ui2::cell(Stock::fmtMoney(m.prix), true));
        t->setItem(r, 7, Ui2::cell(QString(), true));
    }
    l->addWidget(t, 1);

    auto *totalLbl = new QLabel;
    totalLbl->setTextFormat(Qt::RichText);
    totalLbl->setAlignment(Qt::AlignRight);
    l->addWidget(totalLbl);

    // recalcul des totaux ; retourne les lignes cochées (et signale la première quantité invalide)
    bool busy = false;
    auto collect = [&](QVector<PdfExporter::PurchaseLine> *out, int *badRow) {
        double grand = 0;
        int count = 0;
        if (badRow) *badRow = -1;
        busy = true;
        for (int r = 0; r < need.size(); ++r) {
            const Material &m = need[r];
            double qty = 0;
            const bool checked = t->item(r, 0)->checkState() == Qt::Checked;
            const bool valid = Stock::parseNum(t->item(r, 5)->text(), &qty) && qty > 0 &&
                               (!Stock::isIntUnit(m.unite) || qty == std::floor(qty));
            t->item(r, 7)->setText(checked && valid ? Stock::fmtMoney(qty * m.prix) : QString("—"));
            t->item(r, 5)->setForeground(QColor(valid || !checked ? "#1d2b36" : "#b3261e"));
            if (!checked) continue;
            if (!valid) { if (badRow && *badRow < 0) *badRow = r; continue; }
            grand += qty * m.prix;
            ++count;
            if (out) out->push_back({m, qty});
        }
        busy = false;
        totalLbl->setText(QString("<b>%1 ligne(s) — Total estimé : %2 DT</b>").arg(count).arg(Stock::fmtMoney(grand)));
    };
    connect(t, &QTableWidget::itemChanged, &dlg, [&](QTableWidgetItem *) { if (!busy) collect(nullptr, nullptr); });
    collect(nullptr, nullptr);

    auto *bb = new QDialogButtonBox;
    auto *ok = bb->addButton("Générer le bon d'achat PDF", QDialogButtonBox::AcceptRole);
    ok->setProperty("variant", "red");
    auto *cancel = bb->addButton("Annuler", QDialogButtonBox::RejectRole);
    cancel->setProperty("variant", "navy");
    l->addWidget(bb);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    QVector<PdfExporter::PurchaseLine> lines;
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
        lines.clear();
        int bad = -1;
        collect(&lines, &bad);
        if (bad >= 0) {
            t->selectRow(bad);
            QMessageBox::warning(&dlg, "Quantité invalide",
                                 "Une quantité à commander est invalide (nombre supérieur à zéro ; entier pour pièce, bobine…).");
            return;
        }
        if (lines.isEmpty()) {
            QMessageBox::warning(&dlg, "Bon d'achat", "Cochez au moins une matière à commander.");
            return;
        }
        dlg.accept();
    });
    if (dlg.exec() != QDialog::Accepted) return;

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(this, "Enregistrer le bon d'achat",
        dir + "/bon-achat_" + QDate::currentDate().toString("yyyy-MM-dd") + ".pdf", "Fichiers PDF (*.pdf)");
    if (path.isEmpty()) return;
    const QString number = "BA-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmm");
    if (PdfExporter::writePurchaseOrder(path, lines, number))
        emit message(QString("Bon d'achat %1 exporté (%2 ligne(s)) : %3").arg(number).arg(lines.size()).arg(path), "ok");
    else
        emit message("Impossible d'écrire le fichier PDF.", "err");
}

// ======================= MovementsPage =======================
MovementsPage::MovementsPage(StockDb *db, QWidget *parent) : PageBase(db, parent)
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(14, 10, 14, 10);
    root->setSpacing(14);

    auto *left = new QVBoxLayout;
    left->setSpacing(8);
    left->addWidget(Ui2::title("Mouvements de stock"));

    fType_ = new QComboBox;
    fType_->addItem("Tous les types", "");
    fType_->addItem("Entrée", "Entrée");
    fType_->addItem("Sortie", "Sortie");
    fMat_ = new QComboBox;
    auto *bClear = Ui2::button("Effacer les filtres", "light");
    count_ = new QLabel;
    count_->setObjectName("muted");
    auto *filters = new QHBoxLayout;
    filters->addWidget(fType_);
    filters->addWidget(fMat_);
    filters->addWidget(bClear);
    filters->addStretch();
    filters->addWidget(count_);
    left->addLayout(filters);

    table_ = new QTableWidget;
    Ui2::setupTable(table_, {"Date", "Matière", "Type", "Quantité", "Unité", "Description"});
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    left->addWidget(table_, 1);
    root->addLayout(left, 1);

    // --- formulaire
    auto *form = new QFrame;
    form->setObjectName("panel");
    form->setFixedWidth(300);
    auto *fl = new QVBoxLayout(form);
    fl->setContentsMargins(14, 12, 14, 12);
    fl->setSpacing(6);
    auto *ft = new QLabel("Nouveau mouvement");
    ft->setObjectName("formTitle");
    fl->addWidget(ft);

    mat_ = new QComboBox;
    type_ = new QComboBox;
    type_->addItem("Entrée", "Entrée");
    type_->addItem("Sortie", "Sortie");
    qty_ = new QLineEdit;
    qty_->setPlaceholderText("0");
    Ui2::numberRule(qty_, 3);   // chiffres seulement (entiers si l'unité de la matière est « pièce »... : voir updateQtyRule)
    date_ = new QDateEdit(QDate::currentDate());
    date_->setCalendarPopup(true);
    date_->setDisplayFormat("dd/MM/yyyy");
    date_->setMaximumDate(QDate::currentDate());
    desc_ = new QPlainTextEdit;
    desc_->setFixedHeight(64);
    mat_->setObjectName("mMat"); type_->setObjectName("mType"); qty_->setObjectName("mQty"); table_->setObjectName("mvTable");

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);
    auto put = [&](const QString &key, const QString &label, QWidget *w, int r, int c, int span) {
        auto f = Ui2::field(label, w);
        grid->addWidget(f.container, r, c, 1, span, Qt::AlignTop);
        errors_.add(key, w, f.error);
    };
    put("materialId", "Matière première", mat_, 0, 0, 2);
    put("type", "Type", type_, 1, 0, 1);
    put("quantite", "Quantité", qty_, 1, 1, 1);
    put("date", "Date", date_, 2, 0, 2);
    put("description", "Description", desc_, 3, 0, 2);
    fl->addLayout(grid);

    info_ = new QLabel;
    info_->setObjectName("mvInfo");
    info_->setWordWrap(true);
    info_->setTextFormat(Qt::RichText);
    fl->addWidget(info_);

    auto *actions = new QHBoxLayout;
    auto *bSave = Ui2::button("Enregistrer", "red");
    auto *bReset = Ui2::button("Réinitialiser", "navy");
    auto *bTop = Ui2::button("Top consommation", "navy");
    actions->addWidget(bSave);
    actions->addWidget(bReset);
    actions->addStretch();
    fl->addLayout(actions);
    fl->addWidget(bTop, 0, Qt::AlignLeft);
    fl->addStretch();
    root->addWidget(form, 0, Qt::AlignTop);

    connect(bTop, &QPushButton::clicked, this, &MovementsPage::onTopConsumption);
    connect(bClear, &QPushButton::clicked, this, [this] {
        fType_->setCurrentIndex(0);
        fMat_->setCurrentIndex(0);
    });
    connect(fType_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { fillTable(); });
    connect(fMat_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { fillTable(); });
    connect(mat_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateQtyRule(); updateInfo(); });
    connect(desc_, &QPlainTextEdit::textChanged, this, [this] {   // 200 caractères maximum : le reste ne s'écrit pas
        if (desc_->toPlainText().size() <= 200) return;
        const QSignalBlocker block(desc_);
        desc_->setPlainText(desc_->toPlainText().left(200));
        desc_->moveCursor(QTextCursor::End);
    });
    connect(type_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateInfo(); });
    connect(qty_, &QLineEdit::textChanged, this, [this] { updateInfo(); });
    connect(qty_, &QLineEdit::returnPressed, this, &MovementsPage::onSave);
    connect(bSave, &QPushButton::clicked, this, &MovementsPage::onSave);
    connect(bReset, &QPushButton::clicked, this, &MovementsPage::onReset);
    connect(db_, &StockDb::changed, this, &MovementsPage::refresh);
    refresh();
    updateInfo();
}

// « Top consommation » : classement des matières les plus sorties du stock sur 7, 30 ou 90 jours.
void MovementsPage::onTopConsumption()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Top consommation");
    dlg.resize(560, 440);
    auto *lay = new QVBoxLayout(&dlg);

    auto *bar = new QHBoxLayout;
    auto *period = new QComboBox;
    period->addItem("7 derniers jours", 7);
    period->addItem("30 derniers jours", 30);
    period->addItem("90 derniers jours", 90);
    period->setCurrentIndex(1);
    auto *top = new QComboBox;
    top->addItem("Top 5", 5);
    top->addItem("Top 10", 10);
    bar->addWidget(new QLabel("Période :"));
    bar->addWidget(period);
    bar->addWidget(top);
    bar->addStretch();
    lay->addLayout(bar);

    auto *table = new QTableWidget;
    Ui2::setupTable(table, {"Rang", "Matière", "Quantité sortie", "Unité"});
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    lay->addWidget(table, 1);

    auto *empty = new QLabel("Aucune sortie sur cette période.");
    empty->setAlignment(Qt::AlignCenter);
    lay->addWidget(empty);

    auto *close = Ui2::button("Fermer", "navy");
    auto *foot = new QHBoxLayout;
    foot->addStretch();
    foot->addWidget(close);
    lay->addLayout(foot);

    auto fill = [&] {
        const int days = period->currentData().toInt();
        const int limit = top->currentData().toInt();
        QVector<QPair<double, Material>> rows;
        for (const Material &m : db_->materials()) {
            const double used = db_->consumedSum(m.nom, days);
            if (used > 0) rows.push_back({used, m});
        }
        std::sort(rows.begin(), rows.end(), [](const QPair<double, Material> &a, const QPair<double, Material> &b) {
            return a.first != b.first ? a.first > b.first : a.second.nom < b.second.nom;
        });
        if (rows.size() > limit) rows.resize(limit);
        table->setRowCount(rows.size());
        for (int r = 0; r < rows.size(); ++r) {
            table->setItem(r, 0, Ui2::cell(QString::number(r + 1), true));
            table->setItem(r, 1, Ui2::cell(rows[r].second.nom));
            table->setItem(r, 2, Ui2::cell(Stock::fmtNum(rows[r].first), true));
            table->setItem(r, 3, Ui2::cell(rows[r].second.unite));
        }
        empty->setVisible(rows.isEmpty());
        table->setVisible(!rows.isEmpty());
    };
    connect(period, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, [&] { fill(); });
    connect(top, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, [&] { fill(); });
    connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);
    fill();
    dlg.exec();
}

void MovementsPage::refresh()
{
    auto rebuild = [](QComboBox *cb, const QString &placeholder, const QVector<QPair<QString, QString>> &items) {
        const QString cur = cb->currentData().toString();
        cb->blockSignals(true);
        cb->clear();
        cb->addItem(placeholder, "");
        for (const auto &i : items) cb->addItem(i.second, i.first);
        const int idx = cb->findData(cur);
        cb->setCurrentIndex(idx < 0 ? 0 : idx);
        cb->blockSignals(false);
    };
    QVector<QPair<QString, QString>> mats;
    for (const Material &m : db_->materials()) mats.push_back({m.nom, m.nom});
    rebuild(mat_, "— Choisir —", mats);
    updateQtyRule();

    QMap<QString, QString> used;   // matières présentes dans l'historique (même si supprimées depuis)
    for (const Movement &mv : db_->movements()) used[mv.materialId] = mv.materialId;
    QVector<QPair<QString, QString>> filterItems;
    for (auto it = used.cbegin(); it != used.cend(); ++it) filterItems.push_back({it.key(), it.value()});
    rebuild(fMat_, "Toutes les matières", filterItems);
    fillTable();
    updateInfo();
}

void MovementsPage::fillTable()
{
    const QString type = fType_->currentData().toString(), mat = fMat_->currentData().toString();
    QVector<Movement> rows;
    for (const Movement &m : db_->movements())
        if ((type.isEmpty() || m.type == type) && (mat.isEmpty() || m.materialId == mat)) rows.push_back(m);
    count_->setText(QString("%1 mouvement(s)").arg(rows.size()));
    table_->clearSpans();
    table_->setRowCount(0);
    if (rows.isEmpty()) {
        Ui2::showEmptyRow(table_, db_->movements().isEmpty()
            ? "Aucun mouvement enregistré.\nUtilisez le formulaire à droite pour enregistrer une entrée ou une sortie."
            : "Aucun mouvement pour ces filtres.");
        return;
    }
    table_->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r) {
        const Movement &m = rows[r];
        const bool in = m.type == "Entrée";
        table_->setItem(r, 0, Ui2::cell(Stock::fmtDate(m.date)));
        table_->setItem(r, 1, Ui2::cell(m.materialId));   // nom de la matière (clé primaire)
        auto *t = Ui2::cell(m.type);
        t->setForeground(QColor(in ? "#2e7d32" : "#b86e00"));
        QFont f = t->font();
        f.setBold(true);
        t->setFont(f);
        table_->setItem(r, 2, t);
        table_->setItem(r, 3, Ui2::cell((in ? "+" : "−") + Stock::fmtNum(m.quantite), true));
        table_->setItem(r, 4, Ui2::cell(m.unite));
        table_->setItem(r, 5, Ui2::cell(m.description));
    }
}

void MovementsPage::updateInfo()
{
    Material m;
    if (!db_->getMaterial(mat_->currentData().toString(), &m)) {
        info_->setText("Choisissez une matière pour voir son stock actuel.");
        Ui2::setInvalid(info_, false);
        return;
    }
    QString txt = QString("Stock actuel : <b>%1 %2</b>").arg(Stock::fmtNum(m.quantite), m.unite);
    bool bad = false;
    double q = 0;
    if (Stock::parseNum(qty_->text(), &q) && q > 0) {
        const double after = type_->currentData().toString() == "Entrée" ? m.quantite + q : m.quantite - q;
        if (after < 0) {
            bad = true;
            txt += "<br>Retrait impossible : le stock deviendrait négatif.";
        } else {
            txt += QString(" → après mouvement : <b>%1 %2</b> (%3)")
                       .arg(Stock::fmtNum(after), m.unite, Stock::statusOf(after, m.seuil));
        }
    }
    info_->setText(txt);
    Ui2::setInvalid(info_, bad);
}

// La quantité est un entier si l'unité de la matière choisie est « pièce », « bobine »... (3 décimales max sinon).
void MovementsPage::updateQtyRule()
{
    Material m;
    const bool integer = db_->getMaterial(mat_->currentData().toString(), &m) && Stock::isIntUnit(m.unite);
    Ui2::numberRule(qty_, integer ? 0 : 3);
}

void MovementsPage::prefill(const QString &materialId, const QString &type)
{
    mat_->setCurrentIndex(std::max(0, mat_->findData(materialId)));
    type_->setCurrentIndex(std::max(0, type_->findData(type)));
    qty_->setFocus();
    updateInfo();
}

void MovementsPage::onReset()
{
    errors_.clear();
    mat_->setCurrentIndex(0);
    type_->setCurrentIndex(0);
    qty_->clear();
    desc_->clear();
    date_->setMaximumDate(QDate::currentDate());
    date_->setDate(QDate::currentDate());
    updateInfo();
}

void MovementsPage::onSave()
{
    Movement mv;
    const Stock::Errors e = db_->recordMovement(mat_->currentData().toString(), type_->currentData().toString(), qty_->text(),
                                                date_->date(), desc_->toPlainText(), &mv);
    if (!e.isEmpty()) {
        errors_.show(e);
        const bool insufficient = e.value("quantite").contains("insuffisant");
        emit message(insufficient ? "Retrait refusé : le stock ne peut pas devenir négatif."
                                  : "Veuillez corriger les champs signalés.", "err");
        return;
    }
    emit message(QString("%1 enregistrée : %2 %3 — %4.").arg(mv.type, Stock::fmtNum(mv.quantite), mv.unite, mv.nom), "ok");
    onReset();
}

// ======================= NotificationsPage =======================
NotificationsPage::NotificationsPage(StockDb *db, QWidget *parent) : PageBase(db, parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 10, 14, 10);
    root->setSpacing(8);

    auto *head = new QHBoxLayout;
    head->addWidget(Ui2::title("Notifications"));
    head->addStretch();
    auto *bRefresh = Ui2::button("Actualiser", "navy");
    head->addWidget(bRefresh);
    root->addLayout(head);

    table_ = new QTableWidget;
    Ui2::setupTable(table_, {"Date", "Matière", "Catégorie", "Quantité disponible", "Unité", "Seuil d'alerte", "Statut", "Action"});
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    root->addWidget(table_, 1);
    root->addWidget(Ui2::hint("Une alerte apparaît dès qu'une matière atteint ou passe sous son seuil. Elle n'est pas dupliquée "
                              "tant que la situation du stock ne change pas, et disparaît quand le stock repasse au-dessus du seuil."));

    connect(bRefresh, &QPushButton::clicked, this, [this] {
        db_->refreshAlerts();
        emit message(QString("Alertes actualisées : %1 en cours.").arg(db_->alertCount()), "info");
    });
    connect(db_, &StockDb::changed, this, &NotificationsPage::refresh);
    refresh();
}

void NotificationsPage::refresh()
{
    const QVector<Alert> alerts = db_->alerts();
    table_->clearSpans();
    table_->setRowCount(0);
    if (alerts.isEmpty()) {
        Ui2::showEmptyRow(table_, "✔ Aucune alerte : toutes les matières sont au-dessus de leur seuil.");
        return;
    }
    table_->setRowCount(alerts.size());
    for (int r = 0; r < alerts.size(); ++r) {
        const Alert &a = alerts[r];
        Material m;
        db_->getMaterial(a.materialId, &m);
        table_->setItem(r, 0, Ui2::cell(Stock::fmtDate(a.date)));
        table_->setItem(r, 1, Ui2::cell(a.nom));
        table_->setItem(r, 2, Ui2::cell(m.categorie));
        table_->setItem(r, 3, Ui2::cell(Stock::fmtNum(a.quantite), true));
        table_->setItem(r, 4, Ui2::cell(a.unite));
        table_->setItem(r, 5, Ui2::cell(Stock::fmtNum(a.seuil), true));
        table_->setItem(r, 6, Ui2::statusCell(a.statut));
        auto *btn = Ui2::button("Réapprovisionner", "red");
        const QString id = a.materialId;
        connect(btn, &QPushButton::clicked, this, [this, id] { emit restock(id); });
        auto *host = new QWidget;
        auto *hl = new QHBoxLayout(host);
        hl->setContentsMargins(4, 2, 4, 2);
        hl->addWidget(btn);
        table_->setCellWidget(r, 7, host);
        table_->setRowHeight(r, 32);
    }
}

// ======================= PdfExporter =======================
namespace PdfExporter {

bool writeInventory(const QString &path, const QVector<Material> &rows, const QString &scopeText)
{
    QPdfWriter writer(path);
    writer.setResolution(300);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
    writer.setTitle("Gestion des matières premières — inventaire");

    QPainter p(&writer);
    if (!p.isActive()) return false;

    auto mm = [](double v) { return int(v * 300.0 / 25.4); };
    const int W = writer.width(), H = writer.height();
    const int rowH = mm(7), headH = mm(8), pad = mm(1.5), footerH = mm(10);

    struct Col { QString title; double weight; Qt::Alignment align; };
    const QVector<Col> cols = {
        {"Nom", 31, Qt::AlignLeft}, {"Catégorie", 12, Qt::AlignLeft},
        {"Fournisseur", 17, Qt::AlignLeft}, {"Quantité", 8, Qt::AlignRight}, {"Unité", 7, Qt::AlignLeft},
        {"Seuil d'alerte", 9, Qt::AlignRight}, {"Prix unitaire (DT)", 11, Qt::AlignRight}, {"Statut", 11, Qt::AlignLeft}};
    double totalW = 0;
    for (const Col &c : cols) totalW += c.weight;
    QVector<int> widths;
    for (const Col &c : cols) widths << int(c.weight / totalW * W);

    int page = 1;
    auto drawFooter = [&]() {
        p.setFont(QFont("Arial", 8));
        p.setPen(QColor("#555555"));
        p.drawText(QRect(0, H - footerH, W, footerH), Qt::AlignRight | Qt::AlignVCenter, QString("Page %1").arg(page));
    };
    auto drawHeader = [&](int y) {
        int x = 0;
        p.setFont(QFont("Arial", 8, QFont::Bold));
        for (int i = 0; i < cols.size(); ++i) {
            p.fillRect(QRect(x, y, widths[i], headH), QColor("#1f3b4d"));
            p.setPen(Qt::white);
            p.drawText(QRect(x + pad, y, widths[i] - 2 * pad, headH), Qt::AlignVCenter | cols[i].align, cols[i].title);
            x += widths[i];
        }
    };

    // --- en-tête du document
    int y = 0;
    p.setPen(QColor("#1b2e3b"));
    p.setFont(QFont("Arial", 18, QFont::Bold));
    p.drawText(QRect(0, y, W, mm(9)), Qt::AlignVCenter, "Gestion des matières premières");
    y += mm(10);
    p.setFont(QFont("Arial", 10));
    p.setPen(QColor("#333333"));
    p.drawText(QRect(0, y, W, mm(6)), Qt::AlignVCenter,
               "Inventaire exporté le " + Stock::fmtDate(QDate::currentDate()));
    y += mm(6);
    p.drawText(QRect(0, y, W, mm(6)), Qt::AlignVCenter, scopeText);
    y += mm(9);

    drawHeader(y);
    y += headH;

    // --- lignes du tableau
    int index = 0;
    for (const Material &m : rows) {
        if (y + rowH > H - footerH - rowH) {   // saut de page (on garde la place du total)
            drawFooter();
            writer.newPage();
            ++page;
            y = 0;
            drawHeader(y);
            y += headH;
        }
        const QString st = Stock::statusOf(m.quantite, m.seuil);
        const QStringList cells = {m.nom, m.categorie, m.fournisseur, Stock::fmtNum(m.quantite), m.unite,
                                   Stock::fmtNum(m.seuil), Stock::fmtMoney(m.prix), st};
        if (index % 2) p.fillRect(QRect(0, y, W, rowH), QColor("#f3ecda"));
        int x = 0;
        for (int i = 0; i < cols.size(); ++i) {
            const bool isStatus = i == cols.size() - 1;
            p.setFont(QFont("Arial", 8, isStatus ? QFont::Bold : QFont::Normal));
            p.setPen(isStatus ? QColor(Stock::statusColor(st)) : QColor("#1d2b36"));
            const QRect r(x + pad, y, widths[i] - 2 * pad, rowH);
            p.drawText(r, Qt::AlignVCenter | cols[i].align, p.fontMetrics().elidedText(cells[i], Qt::ElideRight, r.width()));
            x += widths[i];
        }
        p.setPen(QColor("#d6cbab"));
        p.drawLine(0, y + rowH, W, y + rowH);
        y += rowH;
        ++index;
    }

    // --- total
    double total = 0;
    for (const Material &m : rows) total += Stock::valueOf(m);
    y += mm(3);
    p.setPen(QColor("#1d2b36"));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(QRect(0, y, W, mm(7)), Qt::AlignRight | Qt::AlignVCenter,
               QString("Valeur totale du stock (%1 matière%2) : %3 DT")
                   .arg(rows.size()).arg(rows.size() > 1 ? "s" : "", Stock::fmtMoney(total)));
    drawFooter();
    p.end();
    return true;
}

bool writePurchaseOrder(const QString &path, const QVector<PurchaseLine> &lines, const QString &orderNumber)
{
    if (lines.isEmpty()) return false;
    QPdfWriter writer(path);
    writer.setResolution(300);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Portrait);
    writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    writer.setTitle("Bon d'achat " + orderNumber);

    QPainter p(&writer);
    if (!p.isActive()) return false;

    auto mm = [](double v) { return int(v * 300.0 / 25.4); };
    const int W = writer.width(), H = writer.height();
    const int rowH = mm(7), headH = mm(8), barH = mm(8), pad = mm(1.5), footerH = mm(10);

    struct Col { QString title; double weight; Qt::Alignment align; };
    const QVector<Col> cols = {
        {"Matière", 42, Qt::AlignLeft}, {"Quantité", 11, Qt::AlignRight},
        {"Unité", 9, Qt::AlignLeft}, {"Prix unit. (DT)", 14, Qt::AlignRight}, {"Total (DT)", 14, Qt::AlignRight}};
    double totalW = 0;
    for (const Col &c : cols) totalW += c.weight;
    QVector<int> widths;
    int used = 0;
    for (const Col &c : cols) { widths << int(c.weight / totalW * W); used += widths.last(); }
    widths.last() += W - used;   // la dernière colonne absorbe l'arrondi

    // regroupement par fournisseur (ordre alphabétique)
    QMap<QString, QVector<PurchaseLine>> bySupplier;
    for (const PurchaseLine &ln : lines) bySupplier[ln.material.fournisseur].push_back(ln);

    int page = 1, y = 0;
    auto drawFooter = [&]() {
        p.setFont(QFont("Arial", 8));
        p.setPen(QColor("#555555"));
        p.drawText(QRect(0, H - footerH, W, footerH), Qt::AlignLeft | Qt::AlignVCenter, "Bon d'achat " + orderNumber);
        p.drawText(QRect(0, H - footerH, W, footerH), Qt::AlignRight | Qt::AlignVCenter, QString("Page %1").arg(page));
    };
    auto newPage = [&]() {
        drawFooter();
        writer.newPage();
        ++page;
        y = 0;
    };
    auto drawSupplierBar = [&](const QString &name, bool cont) {
        p.fillRect(QRect(0, y, W, barH), QColor("#8c1c2e"));
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(QRect(pad, y, W - 2 * pad, barH), Qt::AlignVCenter,
                   "Fournisseur : " + name + (cont ? "  (suite)" : QString()));
        y += barH;
    };
    auto drawColHeader = [&]() {
        int x = 0;
        p.setFont(QFont("Arial", 8, QFont::Bold));
        for (int i = 0; i < cols.size(); ++i) {
            p.fillRect(QRect(x, y, widths[i], headH), QColor("#1f3b4d"));
            p.setPen(Qt::white);
            p.drawText(QRect(x + pad, y, widths[i] - 2 * pad, headH), Qt::AlignVCenter | cols[i].align, cols[i].title);
            x += widths[i];
        }
        y += headH;
    };

    // --- en-tête du document
    p.setPen(QColor("#1b2e3b"));
    p.setFont(QFont("Arial", 20, QFont::Bold));
    p.drawText(QRect(0, y, W, mm(10)), Qt::AlignVCenter, "BON D'ACHAT");
    y += mm(11);
    p.setFont(QFont("Arial", 10));
    p.setPen(QColor("#333333"));
    p.drawText(QRect(0, y, W, mm(6)), Qt::AlignVCenter, "N° " + orderNumber + "    —    Date : " + Stock::fmtDate(QDate::currentDate()));
    y += mm(6);
    p.drawText(QRect(0, y, W, mm(6)), Qt::AlignVCenter,
               QString("Matières premières en rupture ou sous le seuil d'alerte — %1 ligne(s), %2 fournisseur(s)")
                   .arg(lines.size()).arg(bySupplier.size()));
    y += mm(10);

    // --- une section par fournisseur
    double grand = 0;
    for (auto sit = bySupplier.cbegin(); sit != bySupplier.cend(); ++sit) {
        const QString &supplier = sit.key();
        const QVector<PurchaseLine> &rows = sit.value();
        if (y + barH + headH + 2 * rowH > H - footerH) newPage();
        drawSupplierBar(supplier, false);
        drawColHeader();

        double sub = 0;
        int index = 0;
        for (const PurchaseLine &ln : rows) {
            if (y + rowH > H - footerH - rowH) {
                newPage();
                drawSupplierBar(supplier, true);
                drawColHeader();
            }
            const Material &m = ln.material;
            const double lineTotal = ln.qty * m.prix;
            sub += lineTotal;
            const QStringList cells = {m.nom, Stock::fmtNum(ln.qty), m.unite, Stock::fmtMoney(m.prix), Stock::fmtMoney(lineTotal)};
            if (index % 2) p.fillRect(QRect(0, y, W, rowH), QColor("#f3ecda"));
            int x = 0;
            for (int i = 0; i < cols.size(); ++i) {
                p.setFont(QFont("Arial", 8, i == 1 ? QFont::Bold : QFont::Normal));
                p.setPen(QColor("#1d2b36"));
                const QRect r(x + pad, y, widths[i] - 2 * pad, rowH);
                p.drawText(r, Qt::AlignVCenter | cols[i].align, p.fontMetrics().elidedText(cells[i], Qt::ElideRight, r.width()));
                x += widths[i];
            }
            p.setPen(QColor("#d6cbab"));
            p.drawLine(0, y + rowH, W, y + rowH);
            y += rowH;
            ++index;
        }
        grand += sub;
        y += mm(1.5);
        p.setPen(QColor("#1d2b36"));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(QRect(0, y, W, mm(7)), Qt::AlignRight | Qt::AlignVCenter,
                   QString("Total %1 : %2 DT").arg(supplier, Stock::fmtMoney(sub)));
        y += mm(11);
    }

    // --- total général + signatures
    if (y + mm(48) > H - footerH) newPage();
    p.setPen(QPen(QColor("#1f3b4d"), 3));
    p.drawLine(0, y, W, y);
    y += mm(2);
    p.setPen(QColor("#1b2e3b"));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(QRect(0, y, W, mm(9)), Qt::AlignRight | Qt::AlignVCenter, "TOTAL GÉNÉRAL : " + Stock::fmtMoney(grand) + " DT");
    y += mm(18);
    const int boxW = (W - mm(10)) / 2, boxH = mm(24);
    p.setPen(QPen(QColor("#9aa5ad"), 2));
    p.setFont(QFont("Arial", 9));
    const QStringList sigs = {"Demandé par (date et signature)", "Approuvé par (date et signature)"};
    for (int i = 0; i < 2; ++i) {
        const QRect box(i * (boxW + mm(10)), y, boxW, boxH);
        p.drawRect(box);
        p.drawText(box.adjusted(pad, pad, -pad, -pad), Qt::AlignTop | Qt::AlignLeft, sigs[i]);
    }
    drawFooter();
    p.end();
    return true;
}

}  // namespace PdfExporter

}  // namespace Mat

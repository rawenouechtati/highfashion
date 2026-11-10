// pagesmatieres.h — pages du module Matières premières (Tableau de bord, Matières premières,
//                   Mouvements de stock, Notifications) + petits outils d'interface.
#pragma once
#include "stockdb.h"
#include "matmodels.h"
#include "stockrules.h"
#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QDateEdit>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStyledItemDelegate>
#include <QString>
#include <QStyle>
#include <QTableView>
#include <QTableWidget>
#include <QSqlQueryModel>
#include "matiere.h"
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

namespace Mat {


// PieChart — graphique circulaire en anneau avec légende, dessiné à la main

struct BarItem {
    QString label;       // nom du secteur (légende)
    QString valueText;   // valeur affichée dans la légende
    QString subText;     // (non utilisé)
    double value = 0;    // taille du secteur
    QColor color;
};

class PieChart : public QWidget
{
    Q_OBJECT
public:
    explicit PieChart(QWidget *parent = nullptr);
    void setItems(const QVector<BarItem> &items, const QString &centerTop = QString(), const QString &centerSub = QString());
    void setEmptyText(const QString &text) { empty_ = text; update(); }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QVector<BarItem> items_;
    QString centerTop_, centerSub_;
    QString empty_ = "Aucune donnée.";
};

// UiHelpers.h — petites fabriques de widgets partagées par toutes les pages

// Classe de base des pages : émet des signaux, la fenêtre principale les relie.
class PageBase : public QWidget
{
    Q_OBJECT
public:
    explicit PageBase(StockDb *db, QWidget *parent = nullptr) : QWidget(parent), db_(db)
    {
        setObjectName("page");
        setAttribute(Qt::WA_StyledBackground, true);
    }
signals:
    void message(const QString &text, const QString &type);   // type : ok | err | warn | info
    void navigate(int page);
    void openMaterial(const QString &id);
    void restock(const QString &id);

protected:
    StockDb *db_;
};

// Onglets du module (PgNotifications n'est pas un onglet : il ouvre une fenêtre).
enum PageIndex { PgDashboard, PgMaterials, PgMovements, PgNotifications };

namespace Ui2 {

inline QPushButton *button(const QString &text, const QString &variant)
{
    auto *b = new QPushButton(text);
    b->setProperty("variant", variant);   // red | navy | light (voir main.cpp)
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

inline QLabel *title(const QString &t)
{
    auto *l = new QLabel(t);
    l->setObjectName("h1");
    return l;
}

inline QLabel *hint(const QString &t)
{
    auto *l = new QLabel(t);
    l->setObjectName("hint");
    l->setWordWrap(true);
    return l;
}

// Cadre à bordure fine avec un titre, comme les sections du logiciel de référence.
inline QFrame *panel(const QString &heading, QWidget *content)
{
    auto *f = new QFrame;
    f->setObjectName("panel");
    auto *l = new QVBoxLayout(f);
    l->setContentsMargins(10, 8, 10, 8);
    l->setSpacing(6);
    if (!heading.isEmpty()) {
        auto *t = new QLabel(heading);
        t->setObjectName("panelTitle");
        l->addWidget(t);
    }
    if (content) l->addWidget(content);
    return f;
}

inline void setupTable(QTableWidget *t, const QStringList &headers)
{
    t->setColumnCount(headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->verticalHeader()->hide();
    t->verticalHeader()->setDefaultSectionSize(24);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setShowGrid(false);
    t->setFocusPolicy(Qt::StrongFocus);
    t->horizontalHeader()->setHighlightSections(false);
    t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setStretchLastSection(true);
}

inline QTableWidgetItem *cell(const QString &text, bool right = false)
{
    auto *it = new QTableWidgetItem(text);
    it->setTextAlignment((right ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
    return it;
}

inline QTableWidgetItem *statusCell(const QString &status)
{
    auto *it = cell(status);
    it->setForeground(QColor(Stock::statusColor(status)));
    QFont f = it->font();
    f.setBold(true);
    it->setFont(f);
    return it;
}

// Ligne unique qui occupe toute la largeur : état vide du tableau.
inline void showEmptyRow(QTableWidget *t, const QString &text)
{
    t->clearSpans();
    t->setRowCount(1);
    auto *it = new QTableWidgetItem(text);
    it->setTextAlignment(Qt::AlignCenter);
    it->setFlags(Qt::NoItemFlags);
    it->setForeground(QColor("#5d6b75"));
    t->setItem(0, 0, it);
    t->setSpan(0, 0, 1, t->columnCount());
    t->setRowHeight(0, 70);
}

inline void setInvalid(QWidget *w, bool invalid)
{
    w->setProperty("invalid", invalid);
    w->style()->unpolish(w);
    w->style()->polish(w);
}

inline bool confirm(QWidget *parent, const QString &title, const QString &text, const QString &okText)
{
    QMessageBox box(QMessageBox::Question, title, text, QMessageBox::NoButton, parent);
    QPushButton *ok = box.addButton(okText, QMessageBox::AcceptRole);
    box.addButton("Annuler", QMessageBox::RejectRole);
    box.setDefaultButton(qobject_cast<QPushButton *>(ok));
    box.exec();
    return box.clickedButton() == ok;
}

// ---- Règles de saisie : le champ REFUSE les caractères qui n'ont rien à faire là (rien ne s'écrit à l'écran).
// La validation finale (obligatoire, doublon, etc.) reste faite dans Stock.cpp au moment d'enregistrer.

// Nom : lettres uniquement (espace, tiret ou apostrophe possibles entre les mots) — pas de chiffres ni de symboles.
inline void nameRule(QLineEdit *e)
{
    e->setValidator(new QRegularExpressionValidator(
        QRegularExpression("^(\\p{L}[\\p{L} '\\x{2019}\\-]*)?$"), e));
}

// Nombre positif : chiffres + (si decimals > 0) un seul séparateur « , » ou « . » suivi de 'decimals' chiffres max.
// decimals = 0 -> entiers seulement. Peut être rappelée (ex. quand l'unité change) : remplace la règle précédente.
inline void numberRule(QLineEdit *e, int decimals)
{
    const QValidator *old = e->validator();
    const QString re = decimals > 0 ? QString("^[0-9]{0,9}([.,][0-9]{0,%1})?$").arg(decimals) : QString("^[0-9]{0,9}$");
    e->setValidator(new QRegularExpressionValidator(QRegularExpression(re), e));
    if (old) const_cast<QValidator *>(old)->deleteLater();
    if (decimals == 0) {   // on enlève les décimales déjà tapées
        const int i = e->text().indexOf(QRegularExpression("[.,]"));
        if (i >= 0) e->setText(e->text().left(i));
    }
}

// Téléphone : chiffres, espaces, + (au début seulement), ( ) . -
inline void phoneRule(QLineEdit *e)
{
    e->setValidator(new QRegularExpressionValidator(QRegularExpression("^\\+?[0-9 ().\\-]{0,19}$"), e));
}

// E-mail : caractères autorisés dans une adresse, un seul « @ », aucun espace.
inline void emailRule(QLineEdit *e)
{
    e->setValidator(new QRegularExpressionValidator(
        QRegularExpression("^[A-Za-z0-9._%+\\-]*(@[A-Za-z0-9.\\-]*)?$"), e));
}

// Un champ de formulaire : étiquette + saisie + message d'erreur (caché tant qu'il est vide).
struct FormField {
    QWidget *container = nullptr;
    QLabel *error = nullptr;
};

inline FormField field(const QString &label, QWidget *input)
{
    FormField f;
    f.container = new QWidget;
    auto *l = new QVBoxLayout(f.container);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    auto *lab = new QLabel(label);
    lab->setObjectName("fieldLabel");
    l->addWidget(lab);
    l->addWidget(input);
    f.error = new QLabel;
    f.error->setObjectName("err");
    f.error->setWordWrap(true);
    f.error->hide();
    l->addWidget(f.error);
    return f;
}

// Associe les clés d'erreur de Stock::Errors aux champs du formulaire.
class FormErrors
{
public:
    void add(const QString &key, QWidget *input, QLabel *err) { map_.insert(key, {input, err}); }
    void clear()
    {
        for (auto it = map_.begin(); it != map_.end(); ++it) {
            it.value().second->clear();
            it.value().second->hide();
            setInvalid(it.value().first, false);
        }
    }
    void show(const Stock::Errors &errs)
    {
        clear();
        QWidget *first = nullptr;
        for (auto it = errs.begin(); it != errs.end(); ++it) {
            if (!map_.contains(it.key())) continue;
            auto pair = map_.value(it.key());
            pair.second->setText(it.value());
            pair.second->show();
            setInvalid(pair.first, true);
            if (!first) first = pair.first;
        }
        if (first) first->setFocus();
    }

private:
    QMap<QString, QPair<QWidget *, QLabel *>> map_;
};

}  // namespace Ui2

// DashboardPage.h — page « Tableau de bord » : indicateurs, graphiques, matières à réapprovisionner

class DashboardPage : public PageBase
{
    Q_OBJECT
public:
    explicit DashboardPage(StockDb *db, QWidget *parent = nullptr);

private slots:
    void refresh();

private:
    QFrame *kpi(const QString &head, const QString &big, const QString &sub, const QString &color = QString()) const;

    QPushButton *btnNotif_{};
    QWidget *kpiHost_{};
    PieChart *chartRefs_{}, *chartValue_{};
    PieChart *replenish_{};
    // statistiques (déplacées depuis la page « Matières premières »)
    PieChart *topCatChart_{}, *topValueChart_{};
    QLabel *totalLabel_{};
    // service : prévision de rupture
    PieChart *forecast_{};
};

// MaterialsPage.h — page « Gestion des matières premières » : tableau + fiche matière première

class MaterialsPage : public PageBase
{
    Q_OBJECT
public:
    explicit MaterialsPage(StockDb *db, QWidget *parent = nullptr);
    void selectMaterial(const QString &id);   // sélectionne une matière (efface les filtres)

private slots:
    void refresh();
    void onSelectionChanged();
    void clearFilters();
    void onAdd();
    void onEdit();
    void onDelete();
    void onExportPdf();     // bon d'achat des matières en rupture / sous le seuil
    void onReceive();       // réception de commande : entrées de stock en une fois
    void onSave();
    void onCancel();
    void liveForm();
    void updateQtyRule();   // quantité entière si l'unité est « pièce », « bobine »...

private:
    void buildUi();
    void fillCombos();
    void fillTable();
    void updateBanner();
    void loadForm(const Material *m);   // nullptr = nouvelle matière
    void fillForm(const Material &m);
    void highlightSelected();
    QVector<Material> viewRows() const;
    QString filterDescription() const;

    // filtres
    QLineEdit *search_{};
    QComboBox *fltCat_{}, *fltSup_{}, *fltStatus_{}, *sortCombo_{};
    QLabel *count_{};
    QPushButton *banner_{}, *btnNotif_{};
    QTableWidget *table_{};
    // fiche
    QLabel *formMode_{};
    QLineEdit *leNom_{}, *leQty_{}, *leSeuil_{}, *lePrix_{}, *leStatut_{}, *leValeur_{};
    QComboBox *cbCat_{}, *cbSup_{}, *cbUnit_{};
    Ui2::FormErrors errors_;

    QString selectedId_;                // nom (clé primaire) de la matière sélectionnée
    bool isNew_ = true;
    bool loading_ = false;
    QString sortKey_ = "nom";
    bool sortAsc_ = true;
};

// MovementsPage.h — page « Mouvements de stock » : historique + formulaire d'entrée / sortie

class MovementsPage : public PageBase
{
    Q_OBJECT
public:
    explicit MovementsPage(StockDb *db, QWidget *parent = nullptr);
    void prefill(const QString &materialId, const QString &type);   // utilisé par « Réapprovisionner »

private slots:
    void refresh();
    void fillTable();
    void updateInfo();
    void updateQtyRule();   // quantité entière selon l'unité de la matière choisie
    void onSave();
    void onReset();
    void onTopConsumption();   // fenêtre « Top consommation »

private:
    QComboBox *fType_{}, *fMat_{};
    QLabel *count_{};
    QTableWidget *table_{};
    QComboBox *mat_{}, *type_{};
    QLineEdit *qty_{};
    QDateEdit *date_{};
    QPlainTextEdit *desc_{};
    QLabel *info_{};
    Ui2::FormErrors errors_;
};

// NotificationsPage.h — page « Notifications » : alertes de stock sans doublon

class NotificationsPage : public PageBase
{
    Q_OBJECT
public:
    explicit NotificationsPage(StockDb *db, QWidget *parent = nullptr);

private slots:
    void refresh();

private:
    QTableWidget *table_{};
};

// PdfExporter.h — export de l'inventaire en PDF (QPdfWriter, aucune bibliothèque externe)

namespace PdfExporter {

// Écrit le PDF (A4 paysage) : titre, date d'export, critères, tableau, valeur totale.
bool writeInventory(const QString &path, const QVector<Material> &rows, const QString &scopeText);

// Une ligne du bon d'achat : la matière et la quantité à commander.
struct PurchaseLine {
    Material material;
    double qty = 0;
};

// Écrit le bon d'achat (A4 portrait) : une section par fournisseur, total par fournisseur et total général.
bool writePurchaseOrder(const QString &path, const QVector<PurchaseLine> &lines, const QString &orderNumber);

}  // namespace PdfExporter

}  // namespace Mat

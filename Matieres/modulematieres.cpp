#include "modulematieres.h"
#include "pagesmatieres.h"
#include "stockdb.h"
#include <QDialog>
#include <QTimer>

namespace Mat {

// Thème marine + crème du module (feuille de style portée par ce widget : elle ne touche pas les autres modules)
static const char *kStyle = R"QSS(
QWidget#matRoot, QWidget#page, QWidget#scrollInner, QWidget#matErreur { background: #f5f3ef; }
* { font-family: "Lato", "Segoe UI", "Noto Sans", "DejaVu Sans", sans-serif; font-size: 9pt; color: #1d2b36; }
QScrollArea { background: transparent; border: 0; }
QToolTip { background: #fffdf6; color: #1d2b36; border: 1px solid #17375e; }
QLabel#h1 { font-size: 17pt; font-weight: 700; color: #17375e; }
QLabel#formTitle { font-size: 13pt; font-weight: 700; color: #17375e; }
QLabel#panelTitle { font-size: 10.5pt; font-weight: 700; color: #17375e; }
QLabel#hint, QLabel#muted { color: #5d6b75; font-size: 8.5pt; }
QLabel#muted { font-style: italic; }
QLabel#fieldLabel { font-weight: 700; }
QLabel#err { color: #b3261e; font-size: 8pt; }
QLabel#note { background: #fbf1cd; border: 1px solid #b9a45a; padding: 5px 10px; }
QLabel#mvInfo { background: #ffffff; border: 1px solid #e3e0da; padding: 6px 8px; }
QLabel#mvInfo[invalid="true"] { color: #b3261e; border-color: #b3261e; }
QFrame#panel { background: #f5f3ef; border: 1px solid #e3e0da; }
QFrame#panel QLabel { background: transparent; }
QFrame#kpi { background: #ffffff; border: 1px solid #17375e; }
QLabel#kpiHead { background: #17375e; color: white; padding: 4px 8px; font-weight: 600; }
QLabel#kpiValue { font-size: 16pt; font-weight: 700; padding: 6px 8px 0 8px; background: transparent; }
QLabel#kpiSub { color: #5d6b75; padding: 2px 8px 6px 8px; background: transparent; }
QPushButton { background: #17375e; color: white; border: 0; border-radius: 3px; padding: 5px 14px; min-height: 16px; }
QPushButton:hover { background: #2c527f; }
QPushButton:pressed { background: #102a49; }
QPushButton[variant="red"] { background: #9b2335; }
QPushButton[variant="red"]:hover { background: #831c2c; }
QPushButton[variant="light"] { background: #ece8e0; color: #1d2b36; border: 1px solid #d9d5ce; }
QPushButton[variant="light"]:hover { background: #e2ddd2; }
QPushButton:focus { border: 1px solid #f0b429; }
QPushButton#banner { background: #f9e3e1; color: #8a1f17; border: 1px solid #c0392b; border-radius: 2px;
    text-align: left; padding: 4px 10px; }
QPushButton#banner:hover { background: #f5d3d0; }
QLineEdit, QComboBox, QDateEdit, QSpinBox, QDoubleSpinBox, QPlainTextEdit {
    background: white; border: 1px solid #b8b3a8; border-radius: 2px; padding: 2px 6px; min-height: 18px;
    selection-background-color: #17375e; selection-color: white; }
QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QSpinBox:focus, QPlainTextEdit:focus { border: 1px solid #2c527f; }
QLineEdit[readOnly="true"] { background: #ece8e0; }
QLineEdit[invalid="true"], QComboBox[invalid="true"], QDateEdit[invalid="true"], QPlainTextEdit[invalid="true"] {
    border: 1px solid #b3261e; background: #fff3f1; }
QComboBox QAbstractItemView { background: white; selection-background-color: #dce9f7; selection-color: #1d2b36; }
QRadioButton { background: transparent; spacing: 6px; }
QRadioButton::indicator { width: 12px; height: 12px; border-radius: 7px; border: 1px solid #5d6b75; background: white; }
QRadioButton::indicator:checked { border-color: #17375e; background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, stop:0 #9b2335, stop:0.55 #9b2335, stop:0.65 white, stop:1 white); }
QComboBox::drop-down, QDateEdit::drop-down { border: 0; width: 20px; subcontrol-origin: padding; subcontrol-position: center right; }
QTableView, QTableWidget { font-size: 8.5pt; background: #ffffff; border: 1px solid #17375e; gridline-color: #ece8e0;
    selection-background-color: #dce9f7; selection-color: #1d2b36; }
QTableView::item, QTableWidget::item { padding: 2px 4px; border-bottom: 1px solid #ece8e0; }
QTableView::item:hover, QTableWidget::item:hover { background: #f2f6fb; }
QTableView::item:selected, QTableWidget::item:selected { background: #dce9f7; }
QHeaderView { background: #17375e; }
QHeaderView::section { font-size: 8.5pt; background: #17375e; color: white; padding: 4px 6px; border: 0;
    border-right: 1px solid #35566e; font-weight: 600; }
QTableCornerButton::section { background: #17375e; border: 0; }
QScrollBar:vertical { background: #ece8e0; width: 12px; margin: 0; }
QScrollBar::handle:vertical { background: #c9c4bb; min-height: 24px; border-radius: 3px; margin: 2px; }
QScrollBar:horizontal { background: #ece8e0; height: 12px; margin: 0; }
QScrollBar::handle:horizontal { background: #c9c4bb; min-width: 24px; border-radius: 3px; margin: 2px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QLabel#toast { color: white; padding: 9px 14px; border-radius: 3px; }
QLabel#toast[type="ok"] { background: #2e7d32; }
QLabel#toast[type="err"] { background: #b3261e; }
QLabel#toast[type="warn"] { background: #b86e00; }
QLabel#toast[type="info"] { background: #17375e; }
QMessageBox, QDialog { background: #f5f3ef; }
QMessageBox QLabel, QDialog QLabel { background: transparent; }
QTabWidget#sections::pane { border: 0; background: #f5f3ef; }
QTabWidget#sections QTabBar { background: #f5f3ef; }
QTabBar::tab { background: transparent; color: #17375e; padding: 9px 16px; margin-right: 2px;
               border: 0; border-bottom: 3px solid transparent; font-weight: 600; }
QTabBar::tab:hover { background: #ece8e0; }
QTabBar::tab:selected { color: #9b2335; border-bottom: 3px solid #9b2335; }
)QSS";

ModuleMatieres::ModuleMatieres(QWidget *parent) : QWidget(parent), db_(new StockDb(this))
{
    setObjectName("matRoot");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QString::fromUtf8(kStyle));
    setMinimumSize(640, 420);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    stack_ = new QStackedWidget;
    lay->addWidget(stack_);

    // page d'erreur (table Oracle absente ou colonnes différentes)
    erreurPage_ = new QWidget;
    erreurPage_->setObjectName("matErreur");
    erreurPage_->setAttribute(Qt::WA_StyledBackground, true);
    auto *el = new QVBoxLayout(erreurPage_);
    el->setContentsMargins(40, 40, 40, 40);
    el->addStretch();
    auto *titre = Ui2::title("Matières premières : base de données inaccessible");
    erreurTexte_ = new QLabel;
    erreurTexte_->setWordWrap(true);
    erreurTexte_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *retry = Ui2::button("Réessayer", "red");
    el->addWidget(titre);
    el->addWidget(erreurTexte_);
    el->addWidget(retry, 0, Qt::AlignLeft);
    el->addStretch();
    connect(retry, &QPushButton::clicked, this, [this] { charger(); });
    stack_->addWidget(erreurPage_);

    // zone de messages flottants (coin inférieur droit)
    toastHost_ = new QWidget(this);
    toastHost_->setAttribute(Qt::WA_TransparentForMouseEvents);
    toastLayout_ = new QVBoxLayout(toastHost_);
    toastLayout_->setContentsMargins(0, 0, 0, 0);
    toastLayout_->setSpacing(6);
    toastLayout_->addStretch();
}

bool ModuleMatieres::charger()
{
    if (!db_->charger()) {
        erreurTexte_->setText("La table MATIERE_PREMIERE n'a pas pu être lue dans la base Oracle :\n\n" + db_->lastError() +
                              "\n\nVérifiez que le script de la base (version 5) a bien été exécuté sur ce poste, "
                              "puis cliquez sur « Réessayer ».");
        stack_->setCurrentWidget(erreurPage_);
        return false;
    }
    if (!tabs_) construirePages();
    stack_->setCurrentWidget(tabs_);
    return true;
}

void ModuleMatieres::construirePages()
{
    dashboard_ = new DashboardPage(db_);
    materials_ = new MaterialsPage(db_);
    movements_ = new MovementsPage(db_);
    notifications_ = new NotificationsPage(db_);
    notifications_->setParent(this);   // affichée seulement dans la fenêtre « Notifications »
    notifications_->hide();

    tabs_ = new QTabWidget;
    tabs_->setObjectName("sections");
    tabs_->setDocumentMode(true);
    tabs_->addTab(dashboard_, "Tableau de bord");
    tabs_->addTab(materials_, "Matières premières");
    tabs_->addTab(movements_, "Mouvements de stock");
    stack_->addWidget(tabs_);

    const QVector<PageBase *> pages = {dashboard_, materials_, movements_, notifications_};
    for (PageBase *p : pages) {
        connect(p, &PageBase::message, this, &ModuleMatieres::showToast);
        connect(p, &PageBase::navigate, this, &ModuleMatieres::showPage);
        connect(p, &PageBase::openMaterial, this, [this](const QString &id) {
            showPage(PgMaterials);
            materials_->selectMaterial(id);
        });
        connect(p, &PageBase::restock, this, [this](const QString &id) {
            showPage(PgMovements);
            movements_->prefill(id, "Entrée");
        });
    }
    connect(db_, &StockDb::alertsCreated, this, &ModuleMatieres::onAlertsCreated);
}

void ModuleMatieres::rafraichir()
{
    if (!tabs_) { charger(); return; }
    db_->recharger();
    if (!db_->lastError().isEmpty()) showToast("Lecture de la base : " + db_->lastError(), "err");
}

void ModuleMatieres::showPage(int index)
{
    if (!tabs_) return;
    if (index == PgNotifications) {   // la liste des alertes s'ouvre dans une fenêtre
        QDialog dlg(this);
        dlg.setWindowTitle("Notifications");
        dlg.resize(980, 460);
        auto *l = new QVBoxLayout(&dlg);
        l->setContentsMargins(0, 0, 0, 0);
        l->addWidget(notifications_);
        notifications_->show();
        dlg.exec();
        notifications_->setParent(this);   // reprend la page avant la destruction de la fenêtre
        notifications_->hide();
        return;
    }
    if (index < 0 || index >= tabs_->count()) return;
    tabs_->setCurrentIndex(index);
}

void ModuleMatieres::onAlertsCreated(const QVector<Alert> &created)
{
    for (int i = 0; i < created.size() && i < 3; ++i) {
        const Alert &a = created[i];
        showToast(QString("Alerte : %1 — %2 %3 (seuil %4)")
                      .arg(a.nom, Stock::fmtNum(a.quantite), a.unite, Stock::fmtNum(a.seuil)), "warn");
    }
    if (created.size() > 3) showToast(QString("… et %1 autre(s) alerte(s).").arg(created.size() - 3), "warn");
}

void ModuleMatieres::showToast(const QString &text, const QString &type)
{
    while (toastLayout_->count() > 5) {   // pas plus de 4 messages à la fois
        QLayoutItem *it = toastLayout_->takeAt(1);
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }
    auto *label = new QLabel(text, toastHost_);
    label->setObjectName("toast");
    label->setProperty("type", type);
    label->setWordWrap(true);
    label->setFixedWidth(380);
    label->ensurePolished();
    // heightForWidth ignore le remplissage (padding) de la feuille de style : 28 px horizontaux, 18 px verticaux
    label->setMinimumHeight(label->heightForWidth(380 - 28) + 18);
    toastLayout_->addWidget(label);
    toastHost_->raise();
    QTimer::singleShot(3800, label, &QWidget::deleteLater);
}

void ModuleMatieres::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    toastHost_->setGeometry(width() - 394, 0, 380, height() - 14);
    toastHost_->raise();
}

}  // namespace Mat

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Pages.h"
#include <QDialog>
#include <QDir>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>

MainWindow::MainWindow(const QString &dbPath, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), db_(new Database(this))
{
    ui->setupUi(this);
    setMinimumSize(1050, 680);
    ui->brand->setObjectName("brand");
    ui->sidebar->setObjectName("sidebar");
    ui->centralwidget->setObjectName("central");

    // --- base de données : Oracle via ODBC (DSN « Source_Projet2A » ; voir Database.cpp).
    //     Variable GMP_DB (tests) : « :memory: » ou un fichier .sqlite pour utiliser SQLite à la place.
    QString path = dbPath;
    if (path.isEmpty()) path = Database::defaultTarget();
    QString error;
    if (!db_->open(path, &error)) {
        QMessageBox::critical(this, "Base de données",
                              "Impossible d'ouvrir la base de données :\n" + error +
                                  "\n\nL'application utilisera une base temporaire (les données ne seront pas conservées).");
        db_->open(":memory:", &error);
    }

    // --- pages : 4 onglets (Catégories + Fournisseurs partagent un même onglet ; Notifications s'ouvre en fenêtre)
    dashboard_ = new DashboardPage(db_);
    materials_ = new MaterialsPage(db_);
    movements_ = new MovementsPage(db_);
    categories_ = new CategoriesPage(db_);
    suppliers_ = new SuppliersPage(db_);
    notifications_ = new NotificationsPage(db_);
    notifications_->setParent(this);   // affichée seulement dans la fenêtre « Notifications »
    notifications_->hide();

    auto *reference = new QWidget;
    auto *refLayout = new QVBoxLayout(reference);
    refLayout->setContentsMargins(0, 0, 0, 0);
    refLayout->setSpacing(0);
    refLayout->addWidget(categories_, 1);
    refLayout->addWidget(suppliers_, 1);

    tabs_ = new QTabWidget;
    tabs_->setObjectName("sections");
    tabs_->setDocumentMode(true);
    tabs_->addTab(dashboard_, "Tableau de bord");
    tabs_->addTab(materials_, "Matières premières");
    tabs_->addTab(movements_, "Mouvements de stock");
    tabs_->addTab(reference, "Catégories et fournisseurs");
    ui->stack->addWidget(tabs_);

    const QVector<PageBase *> pages = {dashboard_, materials_, categories_, suppliers_, movements_, notifications_};
    for (PageBase *p : pages) {
        connect(p, &PageBase::message, this, &MainWindow::showToast);
        connect(p, &PageBase::navigate, this, &MainWindow::showPage);
        connect(p, &PageBase::openMaterial, this, [this](const QString &id) {
            showPage(PgMaterials);
            materials_->selectMaterial(id);
        });
        connect(p, &PageBase::restock, this, [this](const QString &id) {
            showPage(PgMovements);
            movements_->prefill(id, "Entrée");
        });
    }

    // --- barre latérale : une seule entrée « Matières premières » ; les sections sont des onglets
    ui->navMaterials->setCursor(Qt::PointingHandCursor);
    ui->navMaterials->setChecked(true);
    connect(ui->navMaterials, &QPushButton::clicked, this, [this] { ui->navMaterials->setChecked(true); });

    // --- zone de notifications flottantes (coin inférieur droit)
    toastHost_ = new QWidget(ui->centralwidget);
    toastHost_->setAttribute(Qt::WA_TransparentForMouseEvents);
    toastLayout_ = new QVBoxLayout(toastHost_);
    toastLayout_->setContentsMargins(0, 0, 0, 0);
    toastLayout_->setSpacing(6);
    toastLayout_->addStretch();

    connect(db_, &Database::alertsCreated, this, &MainWindow::onAlertsCreated);
    showPage(PgDashboard);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::showPage(int index)
{
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

void MainWindow::onAlertsCreated(const QVector<Alert> &created)
{
    for (int i = 0; i < created.size() && i < 3; ++i) {
        const Alert &a = created[i];
        showToast(QString("Alerte : %1 — %2 %3 (seuil %4)")
                      .arg(a.nom, Stock::fmtNum(a.quantite), a.unite, Stock::fmtNum(a.seuil)), "warn");
    }
    if (created.size() > 3) showToast(QString("… et %1 autre(s) alerte(s).").arg(created.size() - 3), "warn");
}

void MainWindow::showToast(const QString &text, const QString &type)
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    const QRect c = ui->centralwidget->rect();
    toastHost_->setGeometry(c.width() - 394, 0, 380, c.height() - 14);
    toastHost_->raise();
}

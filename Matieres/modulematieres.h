// modulematieres.h — widget « Matières premières » à placer dans la page PMatiere de la fenêtre principale.
//
// Il regroupe les trois sections du module (Tableau de bord, Matières premières, Mouvements de stock)
// dans un QTabWidget, la fenêtre « Notifications » et les messages flottants (toasts).
// Toute la persistance passe par la classe entité Matiere (table Oracle MATIERE_PREMIERE).
#pragma once
#include "matmodels.h"
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace Mat {

class StockDb;
class DashboardPage;
class MaterialsPage;
class MovementsPage;
class NotificationsPage;

class ModuleMatieres : public QWidget
{
    Q_OBJECT
public:
    explicit ModuleMatieres(QWidget *parent = nullptr);

    bool charger();          // premier chargement depuis Oracle (false = page d'erreur affichée)
    void rafraichir();       // relit la table : à appeler à chaque affichage de la page
    StockDb *base() const { return db_; }
    void showPage(int index);   // PgDashboard, PgMaterials, PgMovements ou PgNotifications

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void showToast(const QString &text, const QString &type);
    void onAlertsCreated(const QVector<Alert> &created);

private:
    void construirePages();

    StockDb *db_;
    QStackedWidget *stack_;
    QWidget *erreurPage_;
    QLabel *erreurTexte_;
    QTabWidget *tabs_ = nullptr;
    DashboardPage *dashboard_ = nullptr;
    MaterialsPage *materials_ = nullptr;
    MovementsPage *movements_ = nullptr;
    NotificationsPage *notifications_ = nullptr;
    QWidget *toastHost_;
    QVBoxLayout *toastLayout_;
};

}  // namespace Mat

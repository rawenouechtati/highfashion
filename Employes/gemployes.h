#ifndef GEMPLOYES_H
#define GEMPLOYES_H

#include <QMainWindow>
#include <QList>
#include <QMap>
#include <QString>
#include "employe.h"
#include "posteTravail.h"

class PlanPostes;
class RepartitionPostes;
class ValidationFormulaire;
class QComboBox;
class QFrame;
class QGridLayout;
class QTimer;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class GEmployes;
}
QT_END_NAMESPACE

class GEmployes : public QMainWindow
{
    Q_OBJECT

public:
    // employeConnecte sert a adapter l'affichage au poste (Gerant = acces complet
    // a la gestion des employes, autre poste = espace personnel seulement).
    explicit GEmployes(const Employe &employeConnecte = Employe(), QWidget *parent = nullptr);
    ~GEmployes() override;

signals:
    // Emis quand l'utilisateur clique sur "Deconnexion" : main.cpp ecoute ce signal
    // pour fermer cette fenetre et rouvrir l'ecran de connexion.
    void deconnexionDemandee();

private slots:
    void onAjouterClicked();
    void onModifierClicked();
    void onSupprimerClicked();
    void onEnregistrerClicked();
    void onAnnulerClicked();
    void onLigneSelectionnee(int row, int column);
    void onRechercheChanged(const QString &texte);
    void onExporterDocumentClicked();

private:
    void actualiserTableau();
    void viderFormulaire();
    void chargerFormulaireDepuisEmploye(const Employe &e);
    void chargerEmployesDepuisBase(); // va chercher les employes dans Oracle (SELECT)
    void appliquerRestrictionsSelonPoste(); // bascule page Gerant / page employe selon le poste
    // --- Plan de disponibilite des postes de travail (Modele : PosteTravail / Employe) ---
    void chargerPostesTravailDepuisBase();      // recharge m_occupants (qui occupe quel poste)
    void construirePlanDisponibilitePostes();   // (re)construit la grille de cartes
    void configurerFormulaire();                // listes, longueurs max et validation en temps reel de la fiche
    void afficherErreurModele(const QString &champ, const QString &message); // message du Modele sous le bon champ
    void adapterMiseEnPage();                   // layouts adaptatifs (remplacent les positions absolues du .ui)
    void remplirComboServices();                // combo "Service" du formulaire (obligatoire, ID_SERVICE)
    void remplirComboPostesTravail();           // combo "Poste de travail" du formulaire (+ "Aucun")
    void changerMonPosteTravail(const QString &poste); // l'employe connecte prend (ou libere avec "") un poste
    void construireRadarStatistiques();         // "Mes statistiques" : diagramme araignee par type de service
    void rafraichirPostesTravail();             // recharge + reconstruit le plan (timer et apres chaque ecriture)
    void construireGraphiqueStatistiques();// graphique "employes par poste", base sur m_employes

    Ui::GEmployes *ui;
    QList<Employe> m_employes;
    Employe m_employeConnecte;
    int m_ligneSelectionnee = -1;
    bool m_modeModification = false;

    QMap<QString, Employe> m_occupants;     // POSTE_TRAVAIL -> employe qui l'occupe (absent = libre)
    bool m_erreurPostesSignalee = false;    // n'avertit qu'une fois si la lecture des postes echoue
    QComboBox *m_comboService = nullptr;         // liste des services (SERVICE.ID memorise, TYPE_SERVICE affiche)
    ValidationFormulaire *m_validation = nullptr; // messages d'erreur sous les champs de la fiche
    RepartitionPostes *m_repartition = nullptr;   // "Statistiques des employes" : barre de repartition par poste
    PlanPostes *m_planPostesGerant = nullptr;   // tableau de bord (page Gerant, lecture seule)
    PlanPostes *m_planPostesEmploye = nullptr;  // tableau de bord (Mon espace, avec Prendre/Liberer)
    QTimer *m_timerPostes = nullptr;        // rafraichit le plan : plusieurs instances partagent la base
};
#endif // GEMPLOYES_H

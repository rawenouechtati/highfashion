#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDate>
#include <QList>
#include <QMap>
#include <QString>
#include <QVector>
#include <QHash>
#include <QRect>
#include <QSize>
#include <QStringList>
#include "service.h"       // module Service
#include "employe.h"       // module Employe
#include "posteTravail.h"  // module Employe
#include "evenement.h"      // module Evenement

namespace Mat { class ModuleMatieres; }   // module Matieres premieres (dossier Matieres/)
namespace Art { class ModuleArticles; }   // module Articles (dossier Articles/)

class QLabel;
class QListWidgetItem;
class QTextEdit;
class QPushButton;
class QComboBox;
class QScrollArea;
class QBoxLayout;
class QResizeEvent;
// module Employe
class PlanPostes;
class RepartitionPostes;
class ValidationFormulaire;
class QFrame;
class QGridLayout;
class QTimer;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ==========================================================================
// === MODULE CLIENT : widget de diagramme (repartition des clients par niveau) ===
// ==========================================================================
class DiagrammeWidget : public QWidget
{
public:
    explicit DiagrammeWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(120);
    }
    void setDonnees(const QVector<int> &c, int sel, const QStringList &n = QStringList())
    { comptes = c; trancheSel = sel; noms = n; update(); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QVector<int> comptes = QVector<int>(5, 0);
    int trancheSel = -1;
    QStringList noms;                       // noms des barres (Bronze, Argent, Or)
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    // employeConnecte : l'employe qui vient de se connecter (poste inclus). Il sert a adapter la page
    // Employes : Gerant = gestion complete, autre poste = espace personnel seulement.
    explicit MainWindow(const Employe &employeConnecte = Employe(), QWidget *parent = nullptr);
    ~MainWindow();

    // Pages du QStackedWidget principal (ui->SWSmartStitch) : une page par module
    enum PageModule {
        PageEmploye   = 0,   // PEmpl
        PageService   = 1,   // PServices
        PageClient    = 2,   // PClient
        PageArticle   = 3,   // PArticle
        PageMatiere   = 4,   // PMatiere
        PageEvenement = 5    // PEvenement
    };

signals:
    // Emis par les boutons "Deconnexion" : main.cpp ferme la fenetre et rouvre l'ecran de connexion.
    void deconnexionDemandee();

protected:
    void resizeEvent(QResizeEvent *e) override;

    // ==========================================================================
    // === SLOTS GÉNÉRAUX ===   navigation entre les modules (stackedWidget principal)
    // ==========================================================================
private slots:
    void allerVersEmployes();
    void allerVersServices();
    void allerVersClients();
    void allerVersArticles();
    void allerVersMatieres();
    void allerVersEvenements();

private:
    void connecterNavigation();
    Ui::MainWindow *ui;
    // === FIN SLOTS GÉNÉRAUX ===


    // ==========================================================================
    // === MODULE SERVICE ===
    // ==========================================================================
private slots:
    void onRowSelected();
    void recalc();
    void onSave();
    void onCancel();
    void onSearch(const QString &text);
    void exportPdf();
    void genererRecu();
    void envoyerRappels();
    void onAjouter();
    void onModifier();
    void onSupprimer();
    void onPlanning();


private:
    void initialiserModuleService();
    void setupSidebar();
    void setupHeader();
    void setupFiche();
    void setupTable();
    void setupChart();
    void setupStatusBar();
    void setupLayout();
    void applyResponsive();

    void loadDemoData();
    void fillRow(int row);
    void applyRowStyle(int row);
    void fillForm(int idx);
    void updateChart();
    void updateBanner();
    void updateCount();
    QVector<int> trouverConflits(const Service &s, int ignore) const;
    void verifierConflit();

    QVector<Service> m_services;
    int m_current = -1;

    QComboBox   *cmbEmploye = nullptr;
    QLabel      *lblSoldeVal = nullptr;
    QLabel      *lblBenefVal = nullptr;
    QLabel      *lblCount = nullptr;
    QTextEdit   *txtAlertes = nullptr;
    QPushButton *btnExportPDF = nullptr;
    QPushButton *btnRappels = nullptr;
    QPushButton *btnGenerer = nullptr;
    QPushButton *btnEnregistrer = nullptr;
    QPushButton *btnAnnuler = nullptr;

    QLabel      *lblSousTitre = nullptr;
    QLabel      *lblConflit = nullptr;
    QScrollArea *scrollDetails = nullptr;
    QBoxLayout  *m_content = nullptr;
    int          m_layoutState = -1;
    // === FIN MODULE SERVICE ===


    // ==========================================================================
    // === MODULE EMPLOYE ===
    // ==========================================================================
private slots:
    void onAjouterEmployeClicked();
    void onModifierEmployeClicked();
    void onSupprimerEmployeClicked();
    void onEnregistrerEmployeClicked();
    void onAnnulerEmployeClicked();
    void onLigneEmployeSelectionnee(int row, int column);
    void onRechercheEmployeChanged(const QString &texte);
    void onExporterDocumentRHClicked();

private:
    void initialiserModuleEmploye();            // ex-constructeur de la fenetre Employes : connexions, plan, timer
    void actualiserTableauEmployes();
    void viderFormulaireEmploye();
    void chargerFormulaireDepuisEmploye(const Employe &e);
    void chargerEmployesDepuisBase(); // va chercher les employes dans Oracle (SELECT)
    void appliquerRestrictionsSelonPosteEmploye(); // bascule page Gerant / page employe selon le poste
    // --- Plan de disponibilite des postes de travail (Modele : PosteTravail / Employe) ---
    void chargerPostesTravailDepuisBase();      // recharge m_occupants (qui occupe quel poste)
    void construirePlanDisponibilitePostes();   // (re)construit la grille de cartes
    void configurerFormulaireEmploye();                // listes, longueurs max et validation en temps reel de la fiche
    void afficherErreurModeleEmploye(const QString &champ, const QString &message); // message du Modele sous le bon champ
    void adapterMiseEnPageEmploye();                   // layouts adaptatifs (remplacent les positions absolues du .ui)
    void remplirComboServicesEmploye();                // combo "Service" du formulaire (obligatoire, ID_SERVICE)
    void remplirComboPostesTravail();           // combo "Poste de travail" du formulaire (+ "Aucun")
    void changerMonPosteTravail(const QString &poste); // l'employe connecte prend (ou libere avec "") un poste
    void construireRadarStatistiques();         // "Mes statistiques" : diagramme araignee par type de service
    void rafraichirPostesTravail();             // recharge + reconstruit le plan (timer et apres chaque ecriture)
    void construireGraphiqueStatistiquesEmploye();// graphique "employes par poste", base sur m_employes

    Employe m_employeConnecte;              // l'employe qui s'est connecte (poste inclus)
    QList<Employe> m_employes;
    int m_ligneEmployeSelectionnee = -1;
    bool m_modeModificationEmploye = false;

    QMap<QString, Employe> m_occupants;     // POSTE_TRAVAIL -> employe qui l'occupe (absent = libre)
    bool m_erreurPostesSignalee = false;    // n'avertit qu'une fois si la lecture des postes echoue
    QComboBox *m_comboServiceEmploye = nullptr;         // liste des services (SERVICE.ID memorise, TYPE_SERVICE affiche)
    ValidationFormulaire *m_validationEmploye = nullptr; // messages d'erreur sous les champs de la fiche
    RepartitionPostes *m_repartition = nullptr;   // "Statistiques des employes" : barre de repartition par poste
    PlanPostes *m_planPostesGerant = nullptr;   // tableau de bord (page Gerant, lecture seule)
    PlanPostes *m_planPostesEmploye = nullptr;  // tableau de bord (Mon espace, avec Prendre/Liberer)
    QTimer *m_timerPostes = nullptr;        // rafraichit le plan : plusieurs instances partagent la base
    // === FIN MODULE EMPLOYE ===


    // ==========================================================================
    // === MODULE CLIENT ===
    // ==========================================================================
private slots:
    void on_cli_pushButton_6_clicked();            // bouton "generer" (recu)
    void on_cli_pushButtonOuvrir_clicked();        // bouton "ouvrir" (contrat)
    void on_cli_pushButtonEnregistrer_clicked();
    void on_cli_pushButtonAnnuler_clicked();
    void on_cli_pushButton_4_clicked();            // bouton "supprimer"
    void onDeconnexionClientClicked();
    void rechercherClients(const QString &texte);
    void trierClients();
    void onAjouterClientClicked();
    void onModifierClientClicked();
    void exporterListeClientsPdf();

private:
    void initialiserModuleClient();     // ex-constructeur du module : connexions, interface, style
    void viderChampsClient();
    bool emailClientValide(const QString &email) const;
    void chargerClients();              // SELECT via Client::afficher, appele a chaque affichage de la page
    void majDiagrammeClient();
    void activerMenuClient(QPushButton *actif);
    void collecterGeometriesClient(QWidget *parent);
    void appliquerEchelleClient();

    bool m_modeAjoutClient = false;
    bool m_modeModifClient = false;
    int m_ligneModifClient = -1;
    int m_idModifClient = -1;
    QTimer *m_timerHorlogeClient = nullptr;
    QList<QPushButton*> m_boutonsMenuClient;
    QString m_styleMenuActifClient;
    QString m_styleMenuNormalClient;
    DiagrammeWidget *m_diagrammeClient = nullptr;
    QComboBox *m_cmbNiveauClient = nullptr;     // niveau de fidelite (Bronze, Argent, Or)
    QComboBox *m_cmbStatutClient = nullptr;     // statut (Actif, Inactif)
    QHash<QWidget*, QRect> m_geomOrigClient;
    QSize m_baseSizeClient;
    // === FIN MODULE CLIENT ===


    // ==========================================================================
    // === MODULE ARTICLE ===   (Aziz)
    // ==========================================================================
private slots:
    void onDeconnexionArticleClicked();

private:
    void initialiserModuleArticle();      // menu lateral : utilisateur, logo
    void ouvrirModuleArticle();           // cree le widget au 1er affichage, puis relit Oracle a chaque affichage
    Art::ModuleArticles *m_moduleArticles = nullptr;   // onglets Catalogue / Statistiques / Estimation (Articles/)
    // === FIN MODULE ARTICLE ===


    // ==========================================================================
    // === MODULE MATIERE PREMIERE ===   (Ahmed)
    // ==========================================================================
private slots:
    void onDeconnexionMatiereClicked();

private:
    void initialiserModuleMatiere();      // menu lateral : utilisateur, logo
    void ouvrirModuleMatiere();           // cree le widget au 1er affichage, puis relit Oracle a chaque affichage
    Mat::ModuleMatieres *m_moduleMatieres = nullptr;   // onglets Tableau de bord / Matieres / Mouvements (Matieres/)
    // === FIN MODULE MATIERE PREMIERE ===


    // ==========================================================================
    // === MODULE EVENEMENT ===   (Melek : ex-projet GEvents)
    // ==========================================================================
private slots:
    void enregistrerEvenement();
    void supprimerEvenement();
    void modifierEvenement();
    void choisirImageEvenement();
    void rechercherEvenements(const QString &texte);
    void trierEvenements(int index);
    void filtrerEvenements(int index);

    void onAjouterEvenementClicked();
    void onAnnulerEvenementClicked();
    void onReinitialiserEvenementClicked();
    void onSelectionEvenementChanged();
    void onCalendrierEvenementClicked(const QDate &date);
    void onDeconnexionEvenementClicked();

    // navigation interne : tableau de bord / page "Calendrier"
    void afficherListeEvenements();
    void afficherCalendrierEvenements();
    void majListeJourEvenement();
    void ouvrirEvenementDuJour();
    void onJourEvenementActivated(QListWidgetItem *item);

private:
    void initialiserModuleEvenement();      // ex-constructeur de la fenetre GEvents : connexions, chargement

    // donnees : table Oracle EVENEMENT, via la classe Evenement (Evenements/evenement.h)
    void chargerEvenements();
    QString dossierDonneesEvenement() const;      // copie locale des affiches (images)
    const Evenement *trouverEvenementParId(int id) const;

    // affichage
    void rafraichirEvenements(int selectId = -1);
    void remplirTableEvenements(int selectId);
    QVector<Evenement> evenementsFiltresTries() const;
    void majCombosFiltreEvenement();
    void majCartesEvenement();
    void majBarresEvenement();
    void majLieuxEvenement();
    void majMarquesCalendrierEvenement();
    void ouvrirEvenementDansFiche(int id);
    QDate dateInitialeCalendrierEvenement() const;
    static QWidget *creerBadgeEvenement(const QString &statut);

    // fiche evenement
    int  idEvenementSelectionne() const;
    void viderFicheEvenement();
    void remplirFicheEvenement(const Evenement &e);
    Evenement donneesFicheEvenement();
    void definirApercuEvenement(const QString &path);
    QString copierImageEvenement(const QString &path) const;

    QVector<Evenement> m_evenements;          // toutes les donnees
    QVector<Evenement> m_evenementsVisibles;  // lignes affichees (apres filtres + tri)
    int     m_idEvenementCourant = -1;        // -1 = nouvel evenement
    QString m_imageEvenementCourante;
    // === FIN MODULE EVENEMENT ===
};

#endif // MAINWINDOW_H

#include "gemployes.h"
#include "ui_gemployes.h"
#include "employe.h"
#include <QHeaderView>
#include <QBarSet>
#include <QBarSeries>
#include <QChart>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QMap>
#include <QFrame>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <QSqlQueryModel>
#include <QRegularExpression>
#include "posteTravail.h"
#include "planpostes.h"
#include "validationui.h"
#include "repartitionpostes.h"
#include "pictogrammes.h"
#include <QAction>
#include <QIcon>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QScrollArea>
#include <QMenuBar>
#include <QStatusBar>
#include "documentrh.h"
#include "radarstatistiques.h"
#include <QChartView>
#include <QDate>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QPainter>
#include <QPushButton>
#include <QUrl>
// Note : plus aucune requete SQL ni acces direct a la base dans cette classe (Vue) --
// tout est delegue aux classes Modele Employe / PosteTravail, cf. consigne de l'atelier
// CRUD du tuteur ("Aucune requete SQL... en dehors de la classe C++").

GEmployes::GEmployes(const Employe &employeConnecte, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GEmployes)
    , m_employeConnecte(employeConnecte)
{
    ui->setupUi(this);
    QPixmap pixmap(":/images/logo.png");
    ui->labelLogoClient->setPixmap(pixmap.scaled(
        ui->labelLogoClient->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        ));
    connect(ui->btnAjouter, &QPushButton::clicked, this, &GEmployes::onAjouterClicked);
    connect(ui->btnModifier, &QPushButton::clicked, this, &GEmployes::onModifierClicked);
    connect(ui->btnSupprimer, &QPushButton::clicked, this, &GEmployes::onSupprimerClicked);
    connect(ui->btnEnregistrer, &QPushButton::clicked, this, &GEmployes::onEnregistrerClicked);
    connect(ui->btnAnnuler, &QPushButton::clicked, this, &GEmployes::onAnnulerClicked);
    connect(ui->tableEmployes, &QTableWidget::cellClicked, this, &GEmployes::onLigneSelectionnee);
    connect(ui->champRecherche, &QLineEdit::textChanged, this, &GEmployes::onRechercheChanged);
    connect(ui->btnDeconnexion, &QPushButton::clicked, this, &GEmployes::deconnexionDemandee);

    ui->champMotDePasse->setEchoMode(QLineEdit::Password);

    viderFormulaire();
    chargerEmployesDepuisBase(); // charge les employes existants depuis Oracle (construit aussi le graphique)

    // Plan de l'atelier : un seul tableau de bord, en lecture seule pour le gerant, avec
    // "Prendre / Liberer mon poste" pour les autres employes.
    m_planPostesGerant = new PlanPostes(false);
    m_planPostesGerant->setDefilementInterne(false); // la page Gerant defile deja en entier
    m_planPostesEmploye = new PlanPostes(true);
    m_planPostesEmploye->setDefilementInterne(false); // pas de defilement : le plan prend la hauteur de ses cartes
    connect(m_planPostesEmploye, &PlanPostes::changerPosteDemande, this, &GEmployes::changerMonPosteTravail);
    adapterMiseEnPage();
    configurerFormulaire();
    connect(ui->btnExporterDocument, &QPushButton::clicked, this, &GEmployes::onExporterDocumentClicked);

    chargerPostesTravailDepuisBase();
    remplirComboPostesTravail();
    construirePlanDisponibilitePostes();

    // Plusieurs instances de l'application (un employe par poste) partagent la meme base :
    // le plan se met a jour tout seul toutes les 15 s, en plus du rafraichissement fait
    // juste apres chaque ajout/modification/suppression locale. Parente a "this" : le timer
    // est detruit avec la fenetre.
    m_timerPostes = new QTimer(this);
    m_timerPostes->setInterval(15000);
    connect(m_timerPostes, &QTimer::timeout, this, &GEmployes::rafraichirPostesTravail);
    m_timerPostes->start();

    ui->tableEmployes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    appliquerRestrictionsSelonPoste();
}

GEmployes::~GEmployes()
{
    delete ui;
}

void GEmployes::appliquerRestrictionsSelonPoste()
{
    // 2 pages dans "stackedModule" :
    //  - index 0 ("page")   : gestion complete des employes -> reservee au Gerant
    //  - index 1 ("page_2") : espace personnel generique (Couturier, Vendeur, Styliste)
    const QString poste = m_employeConnecte.poste();

    const bool estGerant = poste.compare("Gérant", Qt::CaseInsensitive) == 0
                            || poste.compare("Gerant", Qt::CaseInsensitive) == 0;

    // Affiche dans la barre laterale, pour tout le monde (remplace le "Mouna Ayari" fige du .ui)
    ui->labelUtilisateur->setText(m_employeConnecte.prenom() + " " + m_employeConnecte.nom()
                                   + "\n" + poste);

    if (estGerant) {
        ui->stackedModule->setCurrentIndex(0);
        ui->label_8->setText("Gestion des employés");
        ui->label_8->setVisible(false);   // le titre est deja dans la carte "Gestion des employés"
        return;
    }

    // Couturier / Vendeur / Styliste : espace personnel generique
    ui->stackedModule->setCurrentIndex(1);
    ui->label_8->setText("Mon espace — " + poste);

    // "Mon profil" : affichage en lecture seule des infos de l'employe connecte. On
    // s'assure que son poste exact est bien dans la liste du combo avant de le
    // selectionner (la liste pre-remplie dans le .ui peut differer legerement,
    // ex. anciennes donnees "Couturier").
    ui->lineEdit->setText(m_employeConnecte.prenom() + " " + m_employeConnecte.nom());
    ui->lineEdit->setReadOnly(true);
    if (ui->comboBox->findText(poste) < 0)
        ui->comboBox->addItem(poste);
    ui->comboBox->setCurrentText(poste);
    ui->comboBox->setEnabled(false);
    ui->lineEdit_3->setText(m_employeConnecte.email());
    ui->lineEdit_3->setToolTip(m_employeConnecte.email());
    ui->lineEdit_3->setReadOnly(true);
    ui->lineEdit_4->setText(m_employeConnecte.telephone());
    ui->lineEdit_4->setReadOnly(true);

    // Mes documents RH : conge par defaut du lendemain a +4 jours.
    ui->dateDebutConge->setMinimumDate(QDate::currentDate());
    ui->dateDebutConge->setDate(QDate::currentDate().addDays(1));
    ui->dateFinConge->setMinimumDate(QDate::currentDate());
    ui->dateFinConge->setDate(QDate::currentDate().addDays(5));

    construireRadarStatistiques();
}

void GEmployes::construireGraphiqueStatistiques()
{
    // Repartition des employes par poste (role), calculee sur la liste chargee depuis la
    // table EMPLOYE : une part par poste autorise, dans l'ordre des postes de la base.
    if (!m_repartition)
        return;   // la page n'est pas encore construite (constructeur) : adapterMiseEnPage() le rappelle

    QMap<QString, int> nombreParPoste;
    for (const Employe &e : m_employes)
        nombreParPoste[e.poste()]++;

    QList<QPair<QString, int>> parts;
    for (const QString &poste : Employe::postes())
        parts.append({poste, nombreParPoste.take(poste)});
    for (auto it = nombreParPoste.constBegin(); it != nombreParPoste.constEnd(); ++it)   // ancienne valeur hors liste
        parts.append({it.key().isEmpty() ? QString("Non renseigné") : it.key(), it.value()});
    m_repartition->setDonnees(parts);
}

void GEmployes::chargerEmployesDepuisBase()
{
    QString erreur;
    m_employes = Employe::chargerTous(&erreur);
    if (!erreur.isEmpty()) {
        QMessageBox::warning(this, "Chargement",
                              "Impossible de charger les employés : " + erreur);
    }

    actualiserTableau();
    construireGraphiqueStatistiques();
}

void GEmployes::actualiserTableau()
{
    ui->tableEmployes->setRowCount(0);
    for (const Employe &e : m_employes) {
        int row = ui->tableEmployes->rowCount();
        ui->tableEmployes->insertRow(row);
        ui->tableEmployes->setItem(row, 0, new QTableWidgetItem(e.nom()));
        ui->tableEmployes->setItem(row, 1, new QTableWidgetItem(e.prenom()));
        ui->tableEmployes->setItem(row, 2, new QTableWidgetItem(e.poste()));
        ui->tableEmployes->setItem(row, 3, new QTableWidgetItem(e.email()));
        ui->tableEmployes->setItem(row, 4, new QTableWidgetItem(e.telephone()));
        ui->tableEmployes->setItem(row, 5, new QTableWidgetItem(e.statut()));
    }
}

void GEmployes::viderFormulaire()
{
    ui->champNom->clear();
    ui->champPrenom->clear();
    ui->comboPoste->setCurrentIndex(-1);   // "Choisir..." : rien n'est choisi au depart
    ui->comboStatut->setCurrentIndex(-1);
    ui->comboPosteTravail->setCurrentIndex(0); // "Aucun"
    if (m_comboService)
        m_comboService->setCurrentIndex(-1);
    ui->champEmail->clear();
    ui->champTelephone->clear();
    ui->champMotDePasse->clear();
    m_ligneSelectionnee = -1;
    m_modeModification = false;
    ui->tableEmployes->clearSelection();

    // formulaire vierge : pas de message d'erreur affiche, mais Enregistrer reste desactive
    // tant que les champs obligatoires ne sont pas remplis
    if (m_validation) {
        m_validation->effacerMessages();
        m_validation->evaluer();
    }
}

void GEmployes::chargerFormulaireDepuisEmploye(const Employe &e)
{
    ui->champNom->setText(e.nom());
    ui->champPrenom->setText(e.prenom());
    ui->comboPoste->setCurrentIndex(ui->comboPoste->findData(e.poste()));     // -1 si valeur inconnue
    ui->comboStatut->setCurrentIndex(ui->comboStatut->findData(e.statut()));
    const int indexPoste = ui->comboPosteTravail->findData(e.posteTravail());
    ui->comboPosteTravail->setCurrentIndex(indexPoste >= 0 ? indexPoste : 0);
    m_comboService->setCurrentIndex(m_comboService->findData(QString::number(e.idService())));
    ui->champEmail->setText(e.email());
    ui->champTelephone->setText(e.telephone());
    ui->champMotDePasse->clear(); // on ne réaffiche jamais un mot de passe existant
}

void GEmployes::onLigneSelectionnee(int row, int /*column*/)
{
    m_ligneSelectionnee = row;
}

void GEmployes::onAjouterClicked()
{
    viderFormulaire();
    ui->champNom->setFocus();
}

void GEmployes::onModifierClicked()
{
    if (m_ligneSelectionnee < 0 || m_ligneSelectionnee >= m_employes.size()) {
        QMessageBox::warning(this, "Modifier", "Sélectionne d'abord un employé dans le tableau.");
        return;
    }
    chargerFormulaireDepuisEmploye(m_employes.at(m_ligneSelectionnee));
    m_modeModification = true;   // le mot de passe devient facultatif (vide = on garde l'ancien)
    m_validation->revalider();   // signale tout de suite une ancienne donnee devenue invalide
}

void GEmployes::onSupprimerClicked()
{
    if (m_ligneSelectionnee < 0 || m_ligneSelectionnee >= m_employes.size()) {
        QMessageBox::warning(this, "Supprimer", "Sélectionne d'abord un employé dans le tableau.");
        return;
    }

    Employe e = m_employes.at(m_ligneSelectionnee);
    auto reponse = QMessageBox::question(this, "Confirmer",
                                         "Supprimer l'employé « " + e.prenom() + " " + e.nom() + " » ?");
    if (reponse != QMessageBox::Yes)
        return;

    QString erreur;
    if (!e.supprimer(&erreur)) {
        QMessageBox::warning(this, "Supprimer", "Échec de la suppression : " + erreur);
        return;
    }

    chargerEmployesDepuisBase();
    rafraichirPostesTravail(); // un employe supprime libere son poste
    viderFormulaire();
}

void GEmployes::onEnregistrerClicked()
{
    // Normalement le bouton est desactive tant que la fiche est invalide ; on revalide quand
    // meme (touche Entree, etat change entre-temps) et on place le curseur sur le premier
    // champ en erreur.
    if (!m_validation->revalider()) {
        if (QWidget *fautif = m_validation->premierEnErreur())
            fautif->setFocus();
        return;
    }

    const QString nom = ui->champNom->text().trimmed();
    const QString prenom = ui->champPrenom->text().trimmed();
    const QString poste = ui->comboPoste->currentData().toString();
    const QString statut = ui->comboStatut->currentData().toString();
    const QString email = ui->champEmail->text().trimmed();
    const QString telephone = ui->champTelephone->text().trimmed();
    const QString motDePasse = ui->champMotDePasse->text(); // en clair : Employe se charge du hachage
    const QString posteTravail = ui->comboPosteTravail->currentData().toString(); // "" = "Aucun"
    const int idService = m_comboService->currentData().toString().toInt();       // 0 = rien de choisi

    QString erreur;
    QString champ;   // champ concerne par l'erreur, quand le Modele le sait
    bool succes;

    if (m_modeModification && m_ligneSelectionnee >= 0 && m_ligneSelectionnee < m_employes.size()) {
        // --- MODIFICATION d'un employe existant (UPDATE) ---
        int id = m_employes.at(m_ligneSelectionnee).id();
        Employe e(id, nom, prenom, poste, email, telephone, statut, motDePasse, posteTravail, idService);
        succes = e.modifier(&erreur, &champ);
    } else {
        // --- AJOUT d'un nouvel employe (INSERT), l'ID vient de la sequence Oracle ---
        Employe e(-1, nom, prenom, poste, email, telephone, statut, motDePasse, posteTravail, idService);
        succes = e.ajouter(&erreur, &champ);
    }

    if (!succes) {
        afficherErreurModele(champ, erreur);
        return;
    }

    chargerEmployesDepuisBase(); // on recharge tout depuis Oracle pour rester synchronise
    rafraichirPostesTravail();   // l'affectation a pu changer le plan
    viderFormulaire();
}

void GEmployes::afficherErreurModele(const QString &champ, const QString &message)
{
    // Le Modele renvoie un message lisible (jamais d'erreur Oracle brute) et, s'il le sait, le
    // champ concerne : on l'affiche sous ce champ plutot que dans une fenetre.
    QWidget *cible = nullptr;
    if (champ == "nom")               cible = ui->champNom;
    else if (champ == "prenom")       cible = ui->champPrenom;
    else if (champ == "poste")        cible = ui->comboPoste;
    else if (champ == "statut")       cible = ui->comboStatut;
    else if (champ == "email")        cible = ui->champEmail;
    else if (champ == "telephone")    cible = ui->champTelephone;
    else if (champ == "posteTravail") cible = ui->comboPosteTravail;
    else if (champ == "service")      cible = m_comboService;
    else if (champ == "motDePasse")   cible = ui->champMotDePasse;

    if (cible) {
        m_validation->afficherErreur(cible, message);
        cible->setFocus();
    } else {
        QMessageBox::warning(this, "Enregistrer", message);
    }
}

void GEmployes::configurerFormulaire()
{
    // Longueurs alignees sur les colonnes de la table EMPLOYE
    ui->champNom->setMaxLength(100);
    ui->champPrenom->setMaxLength(100);
    ui->champEmail->setMaxLength(100);
    ui->champTelephone->setMaxLength(8);
    ui->champMotDePasse->setMaxLength(100);

    // Listes : valeurs exactement egales aux contraintes CHECK de la base ; rien n'est
    // choisi au depart ("Choisir...").
    ui->comboPoste->clear();
    for (const QString &poste : Employe::postes())
        ui->comboPoste->addItem(poste, poste);
    ui->comboStatut->clear();
    ui->comboStatut->addItem("Actif", "Actif");
    ui->comboStatut->addItem("Inactif", "Inactif");
    ui->comboStatut->addItem("Congé", "Conge");   // affiche avec accent, enregistre sans (comme le CHECK)
    ui->comboPoste->setPlaceholderText("Choisir...");
    ui->comboStatut->setPlaceholderText("Choisir...");

    m_validation = new ValidationFormulaire(this);

    // Les regles vivent dans le Modele (Employe::...Valide) : la Vue les branche seulement.
    m_validation->attacherChamp(ui->champNom, [](const QString &v, QString *e) { return Employe::nomValide(v, e); });
    m_validation->attacherChamp(ui->champPrenom, [](const QString &v, QString *e) { return Employe::prenomValide(v, e); });

    // chaque liste a sa propre cellule : son message se place juste sous elle
    QLabel *erreurPoste = ValidationFormulaire::creerEtiquette(ui->groupFiche);
    QLabel *erreurStatut = ValidationFormulaire::creerEtiquette(ui->groupFiche);
    ValidationFormulaire::placerSous(ui->comboPoste, erreurPoste);
    ValidationFormulaire::placerSous(ui->comboStatut, erreurStatut);
    m_validation->attacherCombo(ui->comboPoste, [](const QString &v, QString *e) { return Employe::posteValide(v, e); }, erreurPoste);
    m_validation->attacherCombo(ui->comboStatut, [](const QString &v, QString *e) { return Employe::statutValide(v, e); }, erreurStatut);

    m_validation->attacherChamp(ui->champEmail, [](const QString &v, QString *e) { return Employe::emailValide(v, e); });
    m_validation->attacherChamp(ui->champTelephone, [](const QString &v, QString *e) { return Employe::telephoneValide(v, e); });

    QLabel *erreurService = ValidationFormulaire::creerEtiquette(ui->groupFiche);
    ValidationFormulaire::placerSous(m_comboService, erreurService);
    m_comboService->setPlaceholderText("Choisir...");
    remplirComboServices();
    m_validation->attacherCombo(m_comboService, [](const QString &v, QString *e) { return Employe::serviceValide(v, e); }, erreurService);

    QLabel *erreurPosteTravail = ValidationFormulaire::creerEtiquette(ui->groupFiche);
    ValidationFormulaire::placerSous(ui->comboPosteTravail, erreurPosteTravail);
    m_validation->attacherCombo(ui->comboPosteTravail, [](const QString &v, QString *e) { return Employe::posteTravailValide(v, e); }, erreurPosteTravail);

    // en modification, un mot de passe vide est accepte (on garde l'ancien)
    m_validation->attacherChamp(ui->champMotDePasse, [this](const QString &v, QString *e) {
        return Employe::motDePasseValide(v, e, !m_modeModification);
    });

    // Enregistrer n'est actif que lorsque tous les champs sont valides
    connect(m_validation, &ValidationFormulaire::etatChange, ui->btnEnregistrer, &QPushButton::setEnabled);
    viderFormulaire();
}

void GEmployes::onAnnulerClicked()
{
    viderFormulaire();
}

void GEmployes::onRechercheChanged(const QString &texte)
{
    QString type = ui->comboTri->currentText(); // "Nom" ou "Poste"
    int colonne = (type == "Poste") ? 2 : 0;
    for (int row = 0; row < ui->tableEmployes->rowCount(); ++row) {
        auto *item = ui->tableEmployes->item(row, colonne);
        bool correspond = texte.isEmpty() || (item && item->text().contains(texte, Qt::CaseInsensitive));
        ui->tableEmployes->setRowHidden(row, !correspond);
    }
}

// --- Plan de disponibilite des postes de travail ---------------------------------
//
// Nouvelle fonctionnalite : chaque poste de travail (table POSTE_TRAVAIL) est libre
// ou occupe selon qu'un employe a ce moment-la son EMPLOYE.POSTE_TRAVAIL egal a ce poste
// dessus (contrainte UNIQUE cote base : un poste ne peut avoir qu'un seul occupant a
// la fois). Reutilise l'emplacement et le groupBox de l'ancienne "File d'attente
// intelligente" (retiree sur demande de la prof, jugee trop basique).

void GEmployes::chargerPostesTravailDepuisBase()
{
    // Les 6 postes sont une liste fixe (PosteTravail::tous()) ; seule la table EMPLOYE dit
    // qui occupe quoi : un poste absent de ce resultat est libre.
    QString erreur;
    m_occupants = Employe::occupantsParPoste(&erreur);
    if (!erreur.isEmpty() && !m_erreurPostesSignalee) {
        m_erreurPostesSignalee = true; // le timer rappelle cette methode : un seul avertissement
        QMessageBox::warning(this, "Postes de travail",
                              "Impossible de lire les postes de travail : " + erreur);
    }

    // Le poste de l'employe connecte a pu etre change ailleurs (par le gerant, ou depuis
    // une autre instance de l'application) : on le relit pour que les boutons "Prendre /
    // Liberer mon poste" restent justes.
    QString monPoste;
    for (auto it = m_occupants.constBegin(); it != m_occupants.constEnd(); ++it) {
        if (it.value().id() == m_employeConnecte.id()) {
            monPoste = it.key();
            break;
        }
    }
    m_employeConnecte.setPosteTravail(monPoste);
}

void GEmployes::remplirComboServices()
{
    // La liste vient de la table SERVICE, via le Modele : "ID - TYPE_SERVICE" s'affiche, l'ID est memorise.
    const QString selection = m_comboService->currentData().toString();
    QString erreur;
    QSqlQueryModel *services = Employe::listerServices(&erreur);
    m_comboService->clear();
    for (int i = 0; i < services->rowCount(); ++i)
    {
        const QString id = services->data(services->index(i, 0)).toString();
        const QString type = services->data(services->index(i, 1)).toString();
        m_comboService->addItem(id + " - " + type, id);   // affiche "1 - Nettoyage", memorise l'ID
    }
    delete services;
    m_comboService->setCurrentIndex(m_comboService->findData(selection));   // -1 = "Choisir..."
    if (!erreur.isEmpty())
        QMessageBox::warning(this, "Services", "Impossible de lire les services : " + erreur);
}

void GEmployes::remplirComboPostesTravail()
{
    // Conserve la selection en cours (le plan se rafraichit pendant qu'on edite le formulaire).
    const QString selection = ui->comboPosteTravail->currentData().toString();

    ui->comboPosteTravail->clear();
    ui->comboPosteTravail->addItem("Aucun", QString());
    for (const QString &poste : PosteTravail::tous()) {
        const bool occupe = m_occupants.contains(poste);
        const Employe occupant = m_occupants.value(poste);
        ui->comboPosteTravail->addItem(
            PosteTravail::libelle(poste)
                + (occupe ? " (occupé : " + occupant.prenom() + " " + occupant.nom() + ")" : QString()),
            poste);
    }

    const int index = ui->comboPosteTravail->findData(selection);
    ui->comboPosteTravail->setCurrentIndex(index >= 0 ? index : 0);
}

void GEmployes::rafraichirPostesTravail()
{
    chargerPostesTravailDepuisBase();
    remplirComboPostesTravail();
    construirePlanDisponibilitePostes();
}

void GEmployes::construirePlanDisponibilitePostes()
{
    // Les deux tableaux de bord (gerant / employe) recoivent les memes donnees ; PlanPostes
    // ne reconstruit ses cartes que si quelque chose a change.
    m_planPostesGerant->setDonnees(PosteTravail::tous(), m_occupants, QString());
    m_planPostesEmploye->setDonnees(PosteTravail::tous(), m_occupants, m_employeConnecte.posteTravail());
}

// Carte de section : fond creme arrondi, titre en Playfair suivi d'un filet dore, puis le contenu.
// Libelle + champ empiles dans une cellule (le message de validation vient se glisser dessous).
static QWidget *creerCellule(QLabel *libelle, QWidget *champ)
{
    auto *cellule = new QWidget();
    auto *v = new QVBoxLayout(cellule);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(4);
    v->addWidget(libelle);
    v->addWidget(champ);
    return cellule;
}

// "compact" : marges et titre reduits, pour les cartes qui doivent rester basses.
// "icone" : pictogramme optionnel affiche a gauche du titre.
static QFrame *creerCarteSection(const QString &titre, QWidget *contenu, bool compact = false,
                                 const Icone *icone = nullptr)
{
    auto *carte = new QFrame();
    carte->setObjectName("carteSection");
    carte->setStyleSheet("QFrame#carteSection { background-color: #FFFDF9; border: 1px solid #EADFD0; border-radius: 14px; }");
    auto *colonne = new QVBoxLayout(carte);
    colonne->setContentsMargins(20, compact ? 10 : 16, 20, compact ? 12 : 18);
    colonne->setSpacing(compact ? 6 : 12);

    auto *entete = new QHBoxLayout();
    entete->setSpacing(14);
    auto *libelle = new QLabel(titre);
    libelle->setStyleSheet(QString("font-family: 'Playfair Display'; font-weight: bold; font-size: %1pt; "
                           "color: #1F3A4D; background: transparent; border: none;").arg(compact ? 13 : 14));
    auto *filet = new QFrame();
    filet->setFixedHeight(1);
    filet->setStyleSheet("background-color: #E2D5BF; border: none;");
    if (icone) {
        auto *pastille = new QLabel();
        pastille->setFixedSize(30, 30);
        pastille->setAlignment(Qt::AlignCenter);
        pastille->setPixmap(pixmapIcone(*icone, QColor("#8B1E2F"), 18));
        pastille->setStyleSheet("background-color: #F6EDE4; border: none; border-radius: 15px;");
        entete->setSpacing(10);
        entete->addWidget(pastille);
    }
    entete->addWidget(libelle);
    entete->addWidget(filet, 1, Qt::AlignVCenter);
    colonne->addLayout(entete);
    colonne->addWidget(contenu, 1);
    return carte;
}

void GEmployes::adapterMiseEnPage()
{
    // Le .ui place tout en positions absolues pour une fenetre de 1499 x 872 : trop grand
    // pour beaucoup d'ecrans (contenu coupe en bas) et avec des vides en plein ecran. On
    // remplace ces positions par des layouts qui suivent la taille de la fenetre.
    setMinimumSize(1100, 640);
    ui->menubar->hide();
    ui->statusbar->hide();

    QWidget *barreRecherche = ui->champRecherche->parentWidget();
    QWidget *navigation = ui->navArticles->parentWidget();

    // --- barre laterale : logo, navigation, puis utilisateur et deconnexion en bas ---
    ui->sidebar->setFixedWidth(281);
    auto *lateral = new QVBoxLayout(ui->sidebar);
    lateral->setContentsMargins(25, 28, 25, 28);
    lateral->setSpacing(18);
    ui->labelLogoClient->setFixedSize(231, 101);
    lateral->addWidget(ui->labelLogoClient, 0, Qt::AlignHCenter);
    lateral->addSpacing(10);
    navigation->setFixedHeight(271);
    lateral->addWidget(navigation);
    lateral->addStretch();
    lateral->addWidget(ui->labelUtilisateur);
    lateral->addWidget(ui->btnDeconnexion);

    // --- fenetre : barre laterale a gauche, titre + pages a droite ---
    auto *principal = new QHBoxLayout(ui->centralwidget);
    principal->setContentsMargins(0, 0, 0, 0);
    principal->setSpacing(0);
    auto *zoneDroite = new QWidget(ui->centralwidget);
    auto *droite = new QVBoxLayout(zoneDroite);
    droite->setContentsMargins(28, 22, 28, 18);
    droite->setSpacing(10);
    ui->label_8->setMinimumHeight(44);
    droite->addWidget(ui->label_8);
    droite->addWidget(ui->stackedModule, 1);
    principal->addWidget(ui->sidebar);
    principal->addWidget(zoneDroite, 1);

    // --- page Gerant : cartes "Gestion des employes" / "Statistiques" a gauche, fiche a droite,
    // plan de l'atelier compact en bas ; la page defile verticalement si l'ecran est petit ---
    auto *defilement = new QScrollArea(ui->page);
    defilement->setWidgetResizable(true);
    defilement->setFrameShape(QFrame::NoFrame);
    defilement->setStyleSheet("QScrollArea { background: transparent; }");
    auto *contenu = new QWidget();
    contenu->setObjectName("contenuGerant");
    contenu->setStyleSheet("QWidget#contenuGerant { background: transparent; }");
    defilement->setWidget(contenu);
    auto *lPage = new QVBoxLayout(ui->page);
    lPage->setContentsMargins(0, 0, 0, 0);
    lPage->addWidget(defilement);

    // loupe dans le champ de recherche
    ui->champRecherche->addAction(QIcon(pixmapIcone(Icone::Loupe, QColor("#5F707D"), 18)), QLineEdit::LeadingPosition);

    // carte 1 : recherche + boutons + tableau
    auto *contenuEmployes = new QWidget();
    auto *lEmployes = new QVBoxLayout(contenuEmployes);
    lEmployes->setContentsMargins(0, 0, 0, 0);
    lEmployes->setSpacing(10);
    lEmployes->addWidget(barreRecherche);
    lEmployes->addWidget(ui->tableEmployes, 1);
    ui->tableEmployes->setMinimumHeight(170);
    ui->tableEmployes->verticalHeader()->setDefaultSectionSize(34);
    QFrame *carteEmployes = creerCarteSection("Gestion des employés", contenuEmployes);

    // carte 2 : repartition par poste (a la place de l'ancien histogramme)
    m_repartition = new RepartitionPostes();
    ui->groupStatistiques->hide();
    const Icone iconeStatistiques = Icone::Repartition;
    QFrame *carteStatistiques = creerCarteSection("Statistiques des employés", m_repartition, true, &iconeStatistiques);

    // carte 3 : fiche employe sur deux colonnes. Chaque champ est range avec son libelle dans
    // une cellule ; le message de validation vient se glisser sous le champ, dans la cellule.
    auto cellule = [](QLabel *libelle, QWidget *champ) {
        auto *c = new QWidget();
        auto *v = new QVBoxLayout(c);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(4);
        v->addWidget(libelle);
        v->addWidget(champ);
        return c;
    };
    delete ui->groupFiche->layout();   // l'ancienne pile verticale ; les widgets restent dans le groupe
    ui->groupFiche->setTitle(QString());
    ui->groupFiche->setStyleSheet("QGroupBox { border: none; margin: 0; padding: 0; background: transparent; }");
    auto *fiche = new QGridLayout(ui->groupFiche);
    fiche->setContentsMargins(0, 0, 0, 0);
    fiche->setHorizontalSpacing(14);
    fiche->setVerticalSpacing(12);
    fiche->addWidget(cellule(ui->label, ui->champNom), 0, 0);
    fiche->addWidget(cellule(ui->label_15, ui->champPrenom), 0, 1);
    fiche->addWidget(cellule(ui->label_2, ui->comboPoste), 1, 0);
    fiche->addWidget(cellule(ui->label_3, ui->comboStatut), 1, 1);
    fiche->addWidget(cellule(ui->label_4, ui->champEmail), 2, 0);
    fiche->addWidget(cellule(ui->label_5, ui->champTelephone), 2, 1);
    auto *labelService = new QLabel("Service", ui->groupFiche);
    labelService->setStyleSheet(ui->label_2->styleSheet());
    labelService->setFont(ui->label_2->font());
    m_comboService = new QComboBox(ui->groupFiche);
    fiche->addWidget(cellule(ui->labelPosteTravail, ui->comboPosteTravail), 3, 0);
    fiche->addWidget(cellule(labelService, m_comboService), 3, 1);
    fiche->addWidget(cellule(ui->label_6, ui->champMotDePasse), 4, 0, 1, 2);
    auto *boutons = new QHBoxLayout();
    boutons->setSpacing(14);
    ui->btnEnregistrer->setMinimumHeight(40);
    ui->btnAnnuler->setMinimumHeight(40);
    boutons->addWidget(ui->btnEnregistrer, 1);
    boutons->addWidget(ui->btnAnnuler, 1);
    // l'espace libre se repartit entre les lignes de champs ; les boutons restent en bas
    for (int ligne = 0; ligne < 5; ++ligne)
        fiche->setRowStretch(ligne, 1);
    fiche->addLayout(boutons, 5, 0, 1, 2);
    for (QWidget *champ : std::initializer_list<QWidget *>{
             ui->champNom, ui->champPrenom, ui->comboPoste, ui->comboStatut, ui->champEmail,
             ui->champTelephone, ui->comboPosteTravail, m_comboService, ui->champMotDePasse})
        champ->setMinimumHeight(38);
    QFrame *carteFiche = creerCarteSection("Fiche employé", ui->groupFiche);

    // plan de l'atelier : version compacte, sur toute la largeur
    m_planPostesGerant->setCompact(true);

    auto *grille = new QGridLayout(contenu);
    grille->setContentsMargins(0, 4, 8, 8);
    grille->setHorizontalSpacing(18);
    grille->setVerticalSpacing(16);
    grille->addWidget(carteEmployes, 0, 0);
    grille->addWidget(carteStatistiques, 1, 0);
    grille->addWidget(carteFiche, 0, 1, 2, 1);
    grille->addWidget(m_planPostesGerant, 2, 0, 1, 2);
    grille->setColumnStretch(0, 3);
    grille->setColumnStretch(1, 2);
    grille->setColumnMinimumWidth(1, 440);
    grille->setRowStretch(0, 1);   // le tableau prend toute la place libre, les statistiques restent basses
    grille->setRowStretch(1, 0);
    construireGraphiqueStatistiques();

    // --- page employe : tout tient a l'ecran, sans defilement. Le plan est compact (une seule
    // ligne de cartes) et les trois blocs du bas rangent leurs champs par layouts : ils
    // s'adaptent a la largeur de la fenetre au lieu d'avoir une taille fixe. ---
    ui->groupBox->hide();   // ancienne "File d'attente intelligente", remplacee par le plan
    m_planPostesEmploye->setCompact(true);

    auto neutraliser = [](QGroupBox *groupe) {
        groupe->setTitle(QString());   // la carte porte le titre
        groupe->setStyleSheet("QGroupBox { border: none; margin: 0; padding: 0; background: transparent; }");
    };
    auto grilleDe = [](QWidget *parent) {
        auto *g = new QGridLayout(parent);
        g->setContentsMargins(0, 0, 0, 0);
        g->setHorizontalSpacing(12);
        g->setVerticalSpacing(8);
        return g;
    };

    // Mon profil : 2 x 2 champs en lecture seule
    neutraliser(ui->groupBox_2);
    auto *profil = grilleDe(ui->groupBox_2);
    profil->addWidget(creerCellule(ui->label_9, ui->lineEdit), 0, 0);
    profil->addWidget(creerCellule(ui->label_10, ui->comboBox), 0, 1);
    profil->addWidget(creerCellule(ui->label_11, ui->lineEdit_3), 1, 0, 1, 2);
    profil->addWidget(creerCellule(ui->label_12, ui->lineEdit_4), 2, 0, 1, 2);

    // Mes documents RH : liste, dates, motif, bouton
    neutraliser(ui->groupBox_3);
    ui->label_13->hide();   // "Document pre-rempli..." : la liste se suffit a elle-meme
    auto *documents = grilleDe(ui->groupBox_3);
    documents->addWidget(ui->comboBox_2, 0, 0, 1, 2);
    documents->addWidget(creerCellule(ui->labelDebutConge, ui->dateDebutConge), 1, 0);
    documents->addWidget(creerCellule(ui->labelFinConge, ui->dateFinConge), 1, 1);
    documents->addWidget(ui->champMotifConge, 2, 0, 1, 2);
    documents->addWidget(ui->btnExporterDocument, 3, 0, 1, 2);

    // Mes statistiques : le diagramme araignee remplit la carte
    neutraliser(ui->groupBox_4);
    if (ui->groupBox_4->layout())
        ui->groupBox_4->layout()->setContentsMargins(0, 0, 0, 0);

    for (QWidget *champ : std::initializer_list<QWidget *>{
             ui->lineEdit, ui->comboBox, ui->lineEdit_3, ui->lineEdit_4, ui->comboBox_2,
             ui->dateDebutConge, ui->dateFinConge, ui->champMotifConge})
    {
        champ->setMinimumHeight(38);
        champ->setMaximumHeight(64);
        champ->setSizePolicy(champ->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    }
    ui->btnExporterDocument->setMinimumHeight(40);
    ui->btnExporterDocument->setMaximumHeight(64);
    ui->btnExporterDocument->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QFrame *carteProfil = creerCarteSection("Mon profil", ui->groupBox_2, true);
    QFrame *carteDocuments = creerCarteSection("Mes documents RH", ui->groupBox_3, true);
    QFrame *carteStatsEmploye = creerCarteSection("Mes statistiques", ui->groupBox_4, true);

    auto *lEmploye = new QGridLayout(ui->page_2);
    lEmploye->setContentsMargins(0, 4, 8, 8);
    lEmploye->setHorizontalSpacing(16);
    lEmploye->setVerticalSpacing(14);
    lEmploye->addWidget(m_planPostesEmploye, 0, 0, 1, 3);
    lEmploye->addWidget(carteProfil, 1, 0);
    lEmploye->addWidget(carteDocuments, 1, 1);
    lEmploye->addWidget(carteStatsEmploye, 1, 2);
    lEmploye->setColumnStretch(0, 3);
    lEmploye->setColumnStretch(1, 3);
    lEmploye->setColumnStretch(2, 4);
    lEmploye->setRowStretch(0, 0);   // le plan prend sa hauteur naturelle
    lEmploye->setRowStretch(1, 1);   // les trois cartes se partagent le reste de la hauteur
}

void GEmployes::changerMonPosteTravail(const QString &poste)
{
    QString erreur;
    if (!m_employeConnecte.changerPosteTravail(poste, &erreur)) {
        QMessageBox::warning(this, "Poste de travail", erreur);
    }
    rafraichirPostesTravail();
}

void GEmployes::construireRadarStatistiques()
{
    QString erreur;
    const QMap<QString, int> parType = Employe::compterServicesParType(m_employeConnecte.id(), &erreur);
    if (!erreur.isEmpty()) {
        QMessageBox::warning(this, "Statistiques", "Impossible de charger les statistiques : " + erreur);
    }

    int total = 0;
    for (int n : parType)
        total += n;

    // Vide l'ancien contenu (au cas ou la methode est rappelee) avant d'ajouter le graphique.
    while (QLayoutItem *item = ui->layoutStatistiquesEmploye->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    auto *vue = new QChartView(creerDiagrammeAraignee(
        total == 0 ? "Aucun service pour l'instant" : "Mes services par type",
        Employe::typesService(), parType));
    vue->setRenderHint(QPainter::Antialiasing);
    vue->setStyleSheet("background: transparent;");
    vue->setMinimumHeight(150);
    ui->layoutStatistiquesEmploye->addWidget(vue);
}

void GEmployes::onExporterDocumentClicked()
{
    const QDate debut = ui->dateDebutConge->date();
    const QDate fin = ui->dateFinConge->date();
    if (fin < debut) {
        QMessageBox::warning(this, "Demande de congé",
                              "La date de fin doit être postérieure ou égale à la date de début.");
        ui->dateFinConge->setFocus();
        return;
    }

    const QString nomFichier = "Demande_conge_" + m_employeConnecte.nom() + "_"
                               + debut.toString("yyyyMMdd") + ".pdf";
    const QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter la demande de congé", QDir::home().filePath(nomFichier), "Documents PDF (*.pdf)");
    if (chemin.isEmpty())
        return; // annule par l'utilisateur

    QString erreur;
    if (!DocumentRH::exporterDemandeConge(m_employeConnecte, debut, fin,
                                          ui->champMotifConge->text(), chemin, &erreur)) {
        QMessageBox::warning(this, "Demande de congé", "Échec de l'export : " + erreur);
        return;
    }

    if (QMessageBox::question(this, "Demande de congé",
                              "Document exporté :\n" + chemin + "\n\nVoulez-vous l'ouvrir ?")
        == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(chemin));
    }
}

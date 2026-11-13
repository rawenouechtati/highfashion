// modulearticles.h — widget « Articles » à placer dans la page PArticle de la fenêtre principale.
//
// Trois sections dans un QTabWidget : Catalogue (tableau + fiche : CRUD avec contrôle de saisie),
// Statistiques (articles par décennie) et Estimation (métier innovant : estimation du prix + certificat PDF).
// Toute la persistance passe par la classe entité Article (table Oracle ARTICLE) : aucune requête SQL ici.
#pragma once
#include "article.h"
#include <QMap>
#include <QSet>
#include <QWidget>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSortFilterProxyModel;
class QSqlQueryModel;
class QStackedWidget;
class QTabWidget;
class QTableView;
class QTimer;
class QVBoxLayout;
class QChartView;

namespace Art {

class ModuleArticles : public QWidget
{
    Q_OBJECT
public:
    explicit ModuleArticles(QWidget *parent = nullptr);

    bool charger();       // premier chargement depuis Oracle (false = page d'erreur affichée)
    void rafraichir();    // relit la table : à appeler à chaque affichage de la page

    // métier innovant : certificat d'authenticité (PDF) avec QR code -> fiche d'origine de l'article (PDF).
    // Publiques pour pouvoir être testées sans boîte de dialogue.
    static bool ecrireFicheOrigine(const Article &a, double prixEstime, const QString &chemin, QString *erreur = nullptr);
    static bool ecrireCertificat(const Article &a, double prixEstime, const QString &chemin, const QString &lien,
                                 QString *erreur = nullptr);
    static QString lienFiche(const QString &cheminFiche, const QString &adresseEnLigne);   // contenu du QR code
    bool genererDocuments(const QString &dossier);   // fiche d'origine + certificat dans ce dossier

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onAjouter();
    void onModifier();
    void onSupprimer();
    void onEnregistrer();
    void onAnnuler();
    void onLigneChoisie();
    void onTri(int index);
    void onRecherche(const QString &texte);
    void onEstimer();
    void onAppliquerPrix();
    void onCertificat();
    void onOuvrirFiche();
    void onArticleEstimation(int index);

private:
    void construire();
    QWidget *construireCatalogue();
    QWidget *construireStatistiques();
    QWidget *construireEstimation();
    void recharger(int idASelectionner = 0);
    void remplirListes();
    void mettreAJourStatistiques();
    void viderFiche();
    void remplirFiche(const Article &a);
    Article::Saisie lireSaisie() const;
    void validerChamp(const QString &cle, bool force = false);
    bool validerTout();
    void montrerToast(const QString &texte, const QString &type);
    const Article *trouver(int id) const;
    int idCourant() const { return idCourant_; }

    // --- état ---
    QVector<Article> cache_;
    int idCourant_ = 0;                 // 0 = nouvel article (mode ajout)
    QSet<QString> touches_;             // champs déjà modifiés : leur erreur est affichée
    double derniereEstimation_ = 0;
    QString derniereFiche_;             // chemin de la dernière fiche d'origine générée

    // --- structure ---
    QStackedWidget *stack_ = nullptr;
    QWidget *erreurPage_ = nullptr;
    QLabel *erreurTexte_ = nullptr;
    QTabWidget *tabs_ = nullptr;
    bool construit_ = false;

    // catalogue
    QSqlQueryModel *model_ = nullptr;
    QSortFilterProxyModel *proxy_ = nullptr;
    QTableView *table_ = nullptr;
    QLineEdit *recherche_ = nullptr;
    QComboBox *tri_ = nullptr;
    QLabel *compte_ = nullptr;
    QLabel *ficheTitre_ = nullptr;
    QLineEdit *nom_ = nullptr, *prix_ = nullptr, *origine_ = nullptr;
    QComboBox *maison_ = nullptr, *decennie_ = nullptr, *categorie_ = nullptr, *etat_ = nullptr, *statut_ = nullptr, *service_ = nullptr;
    QDateEdit *date_ = nullptr;
    QMap<QString, QLabel *> erreurs_;
    QPushButton *btnEnregistrer_ = nullptr, *btnModifier_ = nullptr, *btnSupprimer_ = nullptr;

    // statistiques
    QChartView *graphique_ = nullptr;
    QLabel *kpiTotal_ = nullptr, *kpiValeur_ = nullptr, *kpiMoyen_ = nullptr, *kpiDispo_ = nullptr;

    // estimation
    QComboBox *estArticle_ = nullptr, *estMaison_ = nullptr, *estDecennie_ = nullptr, *estEtat_ = nullptr;
    QLabel *estResultat_ = nullptr, *estQr_ = nullptr, *estLienInfo_ = nullptr;
    QLineEdit *estLien_ = nullptr;
    QPushButton *btnAppliquer_ = nullptr, *btnCertificat_ = nullptr, *btnOuvrirFiche_ = nullptr;

    QLabel *toast_ = nullptr;
    QTimer *toastTimer_ = nullptr;
};

}  // namespace Art

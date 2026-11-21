#ifndef PLANPOSTES_H
#define PLANPOSTES_H

#include <QList>
#include <QMap>
#include <QString>
#include <QWidget>
#include "employe.h"

class QFrame;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QProgressBar;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class Atelier3D;

// Tableau de bord des postes de travail : 3 indicateurs (total / occupes / libres), taux
// d'occupation, filtre par statut, puis le plan de l'atelier en 3D (Atelier3D) ou, au choix, une
// carte par poste (icone, badge Libre/Occupe, occupant et son poste). Avec "actions", on peut
// "Prendre ce poste" ou "Liberer mon poste" (espace employe) ; sinon l'affichage est en lecture
// seule (gerant).
// Le widget ne lit jamais la base : les donnees lui sont fournies avec setDonnees().
class PlanPostes : public QWidget
{
    Q_OBJECT

public:
    explicit PlanPostes(bool actions, QWidget *parent = nullptr);

    // postes : liste fixe PosteTravail::tous() ; occupants : poste -> employe qui l'occupe
    // (les postes absents sont libres) ; monPoste : poste de l'utilisateur connecte, "" si aucun.
    void setDonnees(const QStringList &postes, const QMap<QString, Employe> &occupants,
                    const QString &monPoste);

    // Libelle du service de chaque employe (ID_SERVICE -> type de service) : la fiche d'un poste
    // en 3D indique le service dans lequel travaille l'occupant.
    void setServices(const QMap<int, QString> &services);

    // Avec defilement interne (par defaut) la zone des cartes defile dans l'espace disponible.
    // Sans defilement, elle prend la hauteur de toutes les cartes : a utiliser dans une page
    // qui defile deja elle-meme (evite deux barres de defilement imbriquees).
    void setDefilementInterne(bool actif);

    // Version compacte : ni titre ni indicateurs, taux d'occupation, bascule de vue et filtres
    // sur une seule ligne, pour laisser le maximum de hauteur au plan.
    void setCompact(bool actif);

signals:
    // poste vide : l'utilisateur libere son poste.
    void changerPosteDemande(const QString &poste);

protected:
    bool eventFilter(QObject *objet, QEvent *evenement) override;

private:
    enum class Filtre { Tous, Occupes, Libres };

    QFrame *creerTuile(const QString &icone, QLabel **valeur, QLabel **legende);
    QFrame *creerCarte(const QString &poste);
    void reconstruireCartes();
    int colonnesPour(int visibles) const;

    bool m_actions;
    QStringList m_postes;
    QMap<QString, Employe> m_occupants;
    QString m_monPoste;
    Filtre m_filtre = Filtre::Tous;
    int m_colonnes = 0;
    int m_visibles = 0;   // nombre de cartes affichees (apres filtre)
    bool m_defilementInterne = true;
    bool m_compact = false;
    QString m_signature; // resume des donnees : evite de reconstruire les cartes pour rien

    QWidget *m_blocTitre = nullptr;         // titre + sous-titre (masques en compact)
    QHBoxLayout *m_entete = nullptr;        // titre + (indicateurs ou, en compact, taux et filtres)
    QHBoxLayout *m_ligneFiltres = nullptr;
    QFrame *m_cadreTaux = nullptr;
    QFrame *m_cadreFiltres = nullptr;
    QFrame *m_cadreVue = nullptr;           // bascule "3D" / "Liste"
    QPushButton *m_bouton3D = nullptr;
    QPushButton *m_boutonListe = nullptr;
    QStackedWidget *m_vues = nullptr;       // page 0 : plan 3D, page 1 : cartes
    Atelier3D *m_atelier3d = nullptr;
    QList<QFrame *> m_tuiles;               // les 3 indicateurs
    QLabel *m_valeurTotal = nullptr;
    QLabel *m_valeurOccupes = nullptr;
    QLabel *m_valeurLibres = nullptr;
    QLabel *m_legendeTotal = nullptr;
    QLabel *m_legendeOccupes = nullptr;
    QLabel *m_legendeLibres = nullptr;
    QProgressBar *m_barre = nullptr;
    QLabel *m_pourcentage = nullptr;
    QScrollArea *m_zoneCartes = nullptr;
    QWidget *m_contenuCartes = nullptr;
    QGridLayout *m_grille = nullptr;
};

#endif // PLANPOSTES_H

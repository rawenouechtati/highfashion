#ifndef ATELIER3D_H
#define ATELIER3D_H

#include <QColor>
#include <QElapsedTimer>
#include <QMap>
#include <QMatrix4x4>
#include <QPointF>
#include <QPolygonF>
#include <QStringList>
#include <QVector>
#include <QVector3D>
#include <QWidget>
#include "employe.h"

class QFrame;
class QLabel;
class QPushButton;
class QTimer;
class QToolButton;

// Plan 3D de l'atelier : les postes de travail sont des petits ilots (table, machine, mannequin...)
// poses sur le sol de l'atelier. On fait pivoter la scene a la souris, on zoome a la molette,
// un double-clic remet la vue d'ensemble. Un poste libre est vert, un poste occupe est bordeaux
// et l'employe y est represente ; "mon poste" ressort en bleu marine.
//
// La 3D est dessinee au QPainter (projection en perspective calculee a la main, aucun module Qt
// supplementaire) : le projet se compile partout comme avant. Le widget ne lit jamais la base :
// les donnees lui sont fournies avec setDonnees(), comme pour PlanPostes dont il reprend les
// entrees et le signal.
class Atelier3D : public QWidget
{
    Q_OBJECT

public:
    // actions = true (espace employe) : le bandeau du bas propose "Prendre ce poste" / "Liberer mon poste".
    explicit Atelier3D(bool actions, QWidget *parent = nullptr);

    // postes : liste fixe PosteTravail::tous() ; occupants : poste -> employe qui l'occupe (les
    // postes absents sont libres) ; monPoste : poste de l'utilisateur connecte, "" si aucun.
    void setDonnees(const QStringList &postes, const QMap<QString, Employe> &occupants,
                    const QString &monPoste);

    // Libelle du service de chaque employe (ID_SERVICE -> type de service), affiche dans la fiche du poste.
    void setServices(const QMap<int, QString> &services);

    // Postes mis en avant : 0 = tous, 1 = occupes seulement, 2 = libres seulement (les autres
    // s'estompent). Meme logique que le filtre de PlanPostes.
    void setFiltre(int filtre);

    // Poste actuellement selectionne ("" si aucun) et selection par le code (utile aux tests).
    QString posteSelectionne() const;
    void selectionnerPoste(const QString &poste);

    // Remet la vue d'ensemble (angle, hauteur, zoom d'origine).
    void reinitialiserVue();

    QSize sizeHint() const override { return QSize(720, 340); }
    QSize minimumSizeHint() const override { return QSize(360, 230); }

signals:
    // poste vide : l'utilisateur libere son poste.
    void changerPosteDemande(const QString &poste);
    // Un poste vient d'etre choisi dans la scene ("" = selection levee).
    void posteChoisi(const QString &poste);

protected:
    void paintEvent(QPaintEvent *evenement) override;
    void resizeEvent(QResizeEvent *evenement) override;
    void mousePressEvent(QMouseEvent *evenement) override;
    void mouseMoveEvent(QMouseEvent *evenement) override;
    void mouseReleaseEvent(QMouseEvent *evenement) override;
    void mouseDoubleClickEvent(QMouseEvent *evenement) override;
    void wheelEvent(QWheelEvent *evenement) override;
    void leaveEvent(QEvent *evenement) override;
    void showEvent(QShowEvent *evenement) override;
    void hideEvent(QHideEvent *evenement) override;

public:
    // --- geometrie (publique : utilisee par les fonctions de construction du .cpp) ---
    struct Face {
        QVector<QVector3D> points;
        QVector3D normale;
        QVector3D centre;
    };
    struct Solide {
        QVector<Face> faces;
        QColor couleur;
        QVector3D centre;
    };

private:
    struct Element;           // un objet a dessiner, trie du plus loin au plus proche
    struct Zone {             // surface cliquable d'un poste, dans l'ordre de dessin
        int poste;
        QPolygonF polygone;
    };
    struct Etat {             // etat anime d'un poste
        qreal presence = 0;   // 0 = personne, 1 = employe present (apparition en fondu)
        qreal visibilite = 1; // 1 = normal, 0.3 = estompe par le filtre
        qreal survol = 0;     // mise en lumiere sous la souris
    };

    void construireScene();
    void construireDecorPoste(int index, const QString &type, const QVector3D &centre);
    QVector<Solide> personnage(int index, const QVector3D &centre, qreal montee, qreal balancement,
                               const QColor &tenue) const;
    QVector3D positionPoste(int index) const;

    // --- camera et projection ---
    struct Camera {
        QMatrix4x4 vue;
        QMatrix4x4 mvp;
        QVector3D oeil;
        QVector3D droite;
        QRectF zone;
        QPointF decalage;
        bool valide = false;
    };
    QRectF zoneScene() const;
    void preparerCamera(Camera &cam, qreal yaw, qreal pitch, qreal distance, const QRectF &zone, bool avecDecalage) const;
    bool projeter(const Camera &cam, const QVector3D &point, QPointF *ecran) const;
    qreal distanceAjustee(qreal yaw, qreal pitch, const QRectF &zone) const;
    QRectF encombrement(const Camera &cam) const;

    // --- dessin ---
    void dessinerFond(QPainter &p, const QRectF &zone) const;
    void dessinerSol(QPainter &p);
    void dessinerSolide(QPainter &p, const Solide &solide, int poste, qreal alpha, qreal eclat);
    void dessinerEtiquettes(QPainter &p);
    void dessinerLegende(QPainter &p, const QRectF &zone) const;
    QColor couleurAccent(int poste) const;

    // --- interaction / animation ---
    int posteSous(const QPointF &position) const;
    void majBandeau();
    void surTic();

    bool m_actions;
    QStringList m_postes;
    QMap<QString, Employe> m_occupants;
    QMap<int, QString> m_services;
    QString m_monPoste;
    QString m_signature;
    int m_filtre = 0;

    QVector<QVector<Solide> > m_decor;   // objets fixes de chaque poste
    QVector<Etat> m_etats;
    QVector<QVector3D> m_centres;
    qreal m_demiLargeur = 9.9;           // demi-dimensions du sol (unites de la scene)
    qreal m_demiProfondeur = 6.4;

    // vue
    qreal m_yaw = 0, m_yawCible = 0;
    qreal m_pitch = 0, m_pitchCible = 0;
    qreal m_distance = 40;               // distance de la camera, lissee
    qreal m_zoom = 1.0, m_zoomCible = 1.0;
    qreal m_balayage = 0;                // oscillation douce quand personne ne touche a la scene
    qreal m_balayageForce = 0;
    qreal m_temps = 0;
    QElapsedTimer m_depuisAction;
    QTimer *m_minuteur = nullptr;
    QElapsedTimer m_horloge;

    // souris
    bool m_appui = false;
    bool m_glisse = false;
    QPoint m_dernierePosition;
    QPoint m_positionAppui;
    int m_survolePoste = -1;
    int m_selection = -1;

    // zones cliquables recalculees a chaque image
    QVector<Zone> m_zones;
    QVector<QRectF> m_rectEtiquettes;
    QVector<Solide> m_dyn;               // personnages de l'image en cours
    Camera m_camera;

    // bandeau d'information (widgets standards)
    QFrame *m_bandeau = nullptr;
    QLabel *m_pastille = nullptr;
    QLabel *m_titre = nullptr;
    QLabel *m_detail = nullptr;
    QLabel *m_badge = nullptr;
    QPushButton *m_action = nullptr;
    QToolButton *m_boutonVue = nullptr;
};

#endif // ATELIER3D_H

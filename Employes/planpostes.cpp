#include "planpostes.h"
#include "atelier3d.h"
#include "pictogrammes.h"
#include "posteTravail.h"

#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QEvent>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

const char *kMarine = "#1F3A4D";
const char *kBordeaux = "#8B1E2F";
const char *kVert = "#4F7857";
const char *kGris = "#8A8375";

QLabel *etiquette(const QString &texte, const QString &style)
{
    QLabel *label = new QLabel(texte);
    label->setStyleSheet(style);
    return label;
}

QLabel *pastille(const QPixmap &pixmap, int diametre, const QString &fond)
{
    QLabel *label = new QLabel();
    label->setFixedSize(diametre, diametre);
    label->setAlignment(Qt::AlignCenter);
    label->setPixmap(pixmap);
    label->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(fond).arg(diametre / 2));
    return label;
}

} // namespace

PlanPostes::PlanPostes(bool actions, QWidget *parent)
    : QWidget(parent), m_actions(actions)
{
    setObjectName("planPostes");
    auto *principal = new QVBoxLayout(this);
    principal->setContentsMargins(4, 4, 4, 4);
    principal->setSpacing(12);

    // --- en-tete : titre a gauche, 3 indicateurs a droite ---
    auto *entete = new QHBoxLayout();
    m_blocTitre = new QWidget();
    auto *titres = new QVBoxLayout(m_blocTitre);
    titres->setContentsMargins(0, 0, 0, 0);
    titres->setSpacing(2);
    titres->addWidget(etiquette("Plan de l'atelier",
        QString("font-family: 'Playfair Display'; font-weight: bold; font-size: 22pt; color: %1;").arg(kMarine)));
    titres->addWidget(etiquette("Vue d'ensemble des postes et des affectations",
        QString("font-size: 10pt; color: %1;").arg(kGris)));
    entete->addWidget(m_blocTitre);
    entete->addStretch();
    m_entete = entete;
    m_tuiles.append(creerTuile("total", &m_valeurTotal, &m_legendeTotal));
    m_tuiles.append(creerTuile("occupes", &m_valeurOccupes, &m_legendeOccupes));
    m_tuiles.append(creerTuile("libres", &m_valeurLibres, &m_legendeLibres));
    for (QFrame *tuile : std::as_const(m_tuiles))
        entete->addWidget(tuile);
    principal->addLayout(entete);

    // --- taux d'occupation + filtres ---
    auto *ligneFiltres = new QHBoxLayout();
    m_ligneFiltres = ligneFiltres;
    QFrame *cadreTaux = new QFrame();
    m_cadreTaux = cadreTaux;
    cadreTaux->setObjectName("cadreTaux");
    cadreTaux->setStyleSheet("QFrame#cadreTaux { background-color: #FFFDF9; border: 1px solid #EADFD0; border-radius: 10px; }");
    auto *lTaux = new QHBoxLayout(cadreTaux);
    lTaux->setContentsMargins(16, 8, 16, 8);
    lTaux->addWidget(etiquette("Taux d'occupation", QString("font-size: 10pt; color: %1;").arg(kMarine)));
    m_barre = new QProgressBar();
    m_barre->setRange(0, 100);
    m_barre->setTextVisible(false);
    m_barre->setFixedHeight(10);
    m_barre->setMinimumWidth(160);
    m_barre->setStyleSheet(QString(
        "QProgressBar { background-color: #EADFD0; border: none; border-radius: 5px; }"
        "QProgressBar::chunk { background-color: %1; border-radius: 5px; }").arg(kBordeaux));
    lTaux->addWidget(m_barre, 1);
    m_pourcentage = etiquette("0%", QString("font-weight: bold; font-size: 11pt; color: %1;").arg(kMarine));
    lTaux->addWidget(m_pourcentage);
    ligneFiltres->addWidget(cadreTaux, 1);

    QFrame *cadreFiltres = new QFrame();
    m_cadreFiltres = cadreFiltres;
    cadreFiltres->setObjectName("cadreFiltres");
    cadreFiltres->setStyleSheet("QFrame#cadreFiltres { background-color: #FFFDF9; border: 1px solid #EADFD0; border-radius: 10px; }");
    auto *lFiltres = new QHBoxLayout(cadreFiltres);
    lFiltres->setContentsMargins(14, 6, 14, 6);
    lFiltres->setSpacing(6);
    lFiltres->addWidget(etiquette("Filtrer par statut :", QString("font-size: 9pt; color: %1;").arg(kMarine)));

    auto *groupe = new QButtonGroup(this);
    const QList<std::pair<QString, Filtre>> filtres = {
        {"Tous", Filtre::Tous}, {QString::fromUtf8("Occupés"), Filtre::Occupes}, {"Libres", Filtre::Libres}};
    for (const auto &entree : filtres) {
        QPushButton *puce = new QPushButton(entree.first);
        puce->setCheckable(true);
        puce->setChecked(entree.second == Filtre::Tous);
        puce->setCursor(Qt::PointingHandCursor);
        puce->setStyleSheet(QString(
            "QPushButton { background-color: #F3ECE2; color: %1; border: none; border-radius: 13px; "
            "padding: 5px 14px; font-size: 9pt; }"
            "QPushButton:checked { background-color: %2; color: white; font-weight: bold; }").arg(kMarine, kBordeaux));
        groupe->addButton(puce);
        lFiltres->addWidget(puce);
        const Filtre filtre = entree.second;
        connect(puce, &QPushButton::clicked, this, [this, filtre]() {
            m_filtre = filtre;
            m_atelier3d->setFiltre(int(filtre));
            reconstruireCartes();
        });
    }
    ligneFiltres->addWidget(cadreFiltres);
    principal->addLayout(ligneFiltres);

    // --- bascule de vue : plan 3D (par defaut) ou liste de cartes ---
    QFrame *cadreVue = new QFrame();
    m_cadreVue = cadreVue;
    cadreVue->setObjectName("cadreVue");
    cadreVue->setStyleSheet("QFrame#cadreVue { background-color: #FFFDF9; border: 1px solid #EADFD0; border-radius: 10px; }");
    auto *lVue = new QHBoxLayout(cadreVue);
    lVue->setContentsMargins(14, 6, 14, 6);
    lVue->setSpacing(6);
    lVue->addWidget(etiquette("Vue :", QString("font-size: 9pt; color: %1;").arg(kMarine)));
    auto *groupeVue = new QButtonGroup(this);
    const QString styleVue = QString(
        "QPushButton { background-color: #F3ECE2; color: %1; border: none; border-radius: 13px; "
        "padding: 5px 14px; font-size: 9pt; }"
        "QPushButton:checked { background-color: %1; color: white; font-weight: bold; }").arg(kMarine);
    m_bouton3D = new QPushButton("3D");
    m_boutonListe = new QPushButton("Liste");
    for (QPushButton *bouton : {m_bouton3D, m_boutonListe}) {
        bouton->setCheckable(true);
        bouton->setCursor(Qt::PointingHandCursor);
        bouton->setStyleSheet(styleVue);
        groupeVue->addButton(bouton);
        lVue->addWidget(bouton);
    }
    m_bouton3D->setChecked(true);
    ligneFiltres->insertWidget(1, cadreVue);   // entre le taux d'occupation et les filtres

    // --- plan 3D et cartes (zone defilante : beaucoup de postes ou petite fenetre) ---
    m_vues = new QStackedWidget();
    m_atelier3d = new Atelier3D(m_actions);
    connect(m_atelier3d, &Atelier3D::changerPosteDemande, this, &PlanPostes::changerPosteDemande);
    m_vues->addWidget(m_atelier3d);
    m_zoneCartes = new QScrollArea();
    m_zoneCartes->setWidgetResizable(true);
    m_zoneCartes->setFrameShape(QFrame::NoFrame);
    m_zoneCartes->setStyleSheet("QScrollArea { background: transparent; } QScrollArea > QWidget > QWidget { background: transparent; }");
    m_contenuCartes = new QWidget();
    m_grille = new QGridLayout(m_contenuCartes);
    m_grille->setContentsMargins(2, 4, 2, 4);
    m_grille->setSpacing(14);
    m_zoneCartes->setWidget(m_contenuCartes);
    m_zoneCartes->viewport()->installEventFilter(this);
    m_vues->addWidget(m_zoneCartes);
    principal->addWidget(m_vues, 1);

    connect(m_bouton3D, &QPushButton::clicked, this, [this]() { m_vues->setCurrentWidget(m_atelier3d); });
    connect(m_boutonListe, &QPushButton::clicked, this, [this]() {
        m_vues->setCurrentWidget(m_zoneCartes);
        reconstruireCartes();   // la largeur de la zone n'est connue qu'une fois la liste affichee
    });
}

QFrame *PlanPostes::creerTuile(const QString &icone, QLabel **valeur, QLabel **legende)
{
    QFrame *tuile = new QFrame();
    tuile->setObjectName("tuile");
    tuile->setMinimumWidth(190);
    QString bordure = "#EADFD0", fond = "#F6EDE4", couleur = kMarine;
    QPixmap pictogramme;
    if (icone == "total") {
        pictogramme = pixmapPictogrammePoste("Couture", QColor("#8B5A2B"), 30);
    } else if (icone == "occupes") {
        bordure = "#E5B9BF"; fond = "#F8E7E9"; couleur = kBordeaux;
        pictogramme = pixmapIcone(Icone::Groupe, QColor(kBordeaux), 30);
    } else {
        bordure = "#BFD8C3"; fond = "#E8F1E9"; couleur = kVert;
        pictogramme = pixmapIcone(Icone::Personne, QColor(kVert), 30);
    }
    tuile->setStyleSheet(QString("QFrame#tuile { background-color: #FFFDF9; border: 1px solid %1; border-radius: 10px; }").arg(bordure));

    auto *ligne = new QHBoxLayout(tuile);
    ligne->setContentsMargins(14, 8, 18, 8);
    ligne->setSpacing(12);
    ligne->addWidget(pastille(pictogramme, 46, fond));
    auto *textes = new QVBoxLayout();
    textes->setSpacing(0);
    *valeur = etiquette("0", QString("font-family: 'Playfair Display'; font-weight: bold; font-size: 18pt; color: %1;").arg(couleur));
    *legende = etiquette("", QString("font-size: 9pt; color: %1;").arg(couleur));
    textes->addWidget(*valeur);
    textes->addWidget(*legende);
    ligne->addLayout(textes);
    return tuile;
}

void PlanPostes::setDonnees(const QStringList &postes, const QMap<QString, Employe> &occupants,
                            const QString &monPoste)
{
    // Le plan est rafraichi toutes les 15 s : si rien n'a change, on ne touche pas aux cartes
    // (sinon la zone defilante revient en haut et un clic en cours peut etre perdu).
    QString signature = "moi:" + monPoste;
    for (const QString &poste : postes) {
        const Employe occupant = occupants.value(poste);
        signature += "|" + poste + ";" + QString::number(occupant.id()) + ";" + occupant.prenom()
                     + ";" + occupant.nom() + ";" + occupant.poste() + ";" + QString::number(occupant.idService());
    }
    if (signature == m_signature)
        return;
    m_signature = signature;

    m_postes = postes;
    m_occupants = occupants;
    m_monPoste = monPoste;

    int occupes = 0;
    for (const QString &poste : std::as_const(m_postes))
        if (m_occupants.contains(poste))
            ++occupes;
    const int total = m_postes.size();
    const int libres = total - occupes;
    const int taux = total > 0 ? qRound(100.0 * occupes / total) : 0;

    m_valeurTotal->setText(QString::number(total));
    m_legendeTotal->setText(total > 1 ? "Postes au total" : "Poste au total");
    m_valeurOccupes->setText(QString::number(occupes));
    m_legendeOccupes->setText(occupes > 1 ? QString::fromUtf8("Postes occupés") : QString::fromUtf8("Poste occupé"));
    m_valeurLibres->setText(QString::number(libres));
    m_legendeLibres->setText(libres > 1 ? "Postes libres" : "Poste libre");
    m_barre->setValue(taux);
    m_pourcentage->setText(m_compact ? QString("%1% (%2/%3)").arg(taux).arg(occupes).arg(total)
                                     : QString::number(taux) + "%");

    m_atelier3d->setDonnees(m_postes, m_occupants, m_monPoste);
    reconstruireCartes();
}

void PlanPostes::setServices(const QMap<int, QString> &services)
{
    m_atelier3d->setServices(services);
}

void PlanPostes::setCompact(bool actif)
{
    if (m_compact == actif)
        return;
    m_compact = actif;
    for (QFrame *tuile : std::as_const(m_tuiles))
        tuile->setVisible(!actif);
    m_blocTitre->setVisible(!actif);   // compact : une seule ligne (taux, vue, filtres), la place va au plan

    // en compact, le titre disparait : taux, bascule de vue et filtres passent sur la ligne d'en-tete
    m_ligneFiltres->removeWidget(m_cadreTaux);
    m_ligneFiltres->removeWidget(m_cadreVue);
    m_ligneFiltres->removeWidget(m_cadreFiltres);
    if (actif) {
        m_cadreTaux->setMinimumWidth(240);
        m_entete->addWidget(m_cadreTaux, 1);
        m_entete->addWidget(m_cadreVue);
        m_entete->addWidget(m_cadreFiltres);
    } else {
        m_entete->removeWidget(m_cadreTaux);
        m_entete->removeWidget(m_cadreVue);
        m_entete->removeWidget(m_cadreFiltres);
        m_cadreTaux->setMinimumWidth(0);
        m_ligneFiltres->addWidget(m_cadreTaux, 1);
        m_ligneFiltres->addWidget(m_cadreVue);
        m_ligneFiltres->addWidget(m_cadreFiltres);
    }
    m_signature.clear();   // force la reconstruction des cartes avec les nouvelles dimensions
    reconstruireCartes();
}

void PlanPostes::setDefilementInterne(bool actif)
{
    m_defilementInterne = actif;
    m_zoneCartes->setVerticalScrollBarPolicy(actif ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    reconstruireCartes();
}

int PlanPostes::colonnesPour(int visibles) const
{
    const int largeur = m_zoneCartes->viewport()->width();
    const int maximum = qBound(1, largeur / 230, 6);
    const int n = qMax(1, visibles);
    const int lignes = (n + maximum - 1) / maximum;
    return (n + lignes - 1) / lignes;   // cartes reparties a parts egales : 3 + 3 plutot que 4 + 2
}

bool PlanPostes::eventFilter(QObject *objet, QEvent *evenement)
{
    // La largeur utile est celle de la zone defilante : on re-dispose les cartes quand
    // elle change assez pour modifier le nombre de colonnes.
    if (objet == m_zoneCartes->viewport() && evenement->type() == QEvent::Resize
        && colonnesPour(m_visibles) != m_colonnes)
        reconstruireCartes();
    return QWidget::eventFilter(objet, evenement);
}

QFrame *PlanPostes::creerCarte(const QString &poste)
{
    const bool occupe = m_occupants.contains(poste);
    const Employe employe = m_occupants.value(poste);
    const QString occupant = employe.prenom() + " " + employe.nom();
    const bool monPoste = !m_monPoste.isEmpty() && m_monPoste == poste;
    const QString accent = occupe ? kBordeaux : kVert;

    QFrame *carte = new QFrame();
    carte->setObjectName("carte");
    carte->setMinimumWidth(190);
    // hauteur minimale : sans elle les cartes s'ecrasent (texte et bouton superposes)
    // quand la zone est trop basse ; la zone defilante prend alors le relais.
    // Les cartes grandissent avec la zone (jusqu'a 340 px) pour ne pas laisser de grand vide.
    carte->setMinimumHeight(m_actions ? 215 : 190);
    carte->setMaximumHeight(m_actions ? 300 : 210);
    carte->setStyleSheet(QString(
        "QFrame#carte { background-color: #FFFDF9; border: 1px solid #EADFD0; "
        "border-top: 3px solid %1; border-radius: 12px; }").arg(accent));

    auto *colonne = new QVBoxLayout(carte);
    colonne->setContentsMargins(18, 16, 18, 14);
    colonne->setSpacing(10);

    auto *entete = new QHBoxLayout();
    entete->setSpacing(12);
    entete->addWidget(pastille(pixmapPictogrammePoste(PosteTravail::type(poste), QColor(accent), 36), 56,
                               occupe ? "#F6EDE4" : "#E8F1E9"));
    QLabel *nom = etiquette(PosteTravail::libelle(poste), QString(
        "font-family: 'Playfair Display'; font-weight: bold; font-size: 12pt; color: %1;").arg(kMarine));
    nom->setWordWrap(true);
    entete->addWidget(nom, 1);
    colonne->addLayout(entete);

    QLabel *badge = etiquette(occupe ? QString::fromUtf8("●  Occupé") : QString::fromUtf8("●  Libre"), QString(
        "background-color: %1; color: white; border-radius: 12px; padding: 0px 16px; "
        "font-size: 10pt; font-weight: bold;").arg(accent));
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedHeight(24);
    auto *ligneBadge = new QHBoxLayout();
    ligneBadge->addStretch();
    ligneBadge->addWidget(badge);
    ligneBadge->addStretch();
    colonne->addLayout(ligneBadge);

    QFrame *separateur = new QFrame();
    separateur->setFixedHeight(1);
    separateur->setStyleSheet("background-color: #EADFD0;");
    colonne->addWidget(separateur);

    auto *ligneOccupant = new QHBoxLayout();
    ligneOccupant->setSpacing(12);
    QLabel *icone = new QLabel();
    icone->setPixmap(pixmapIcone(Icone::Personne, QColor(occupe ? kBordeaux : kGris), 30));
    icone->setFixedSize(34, 34);
    ligneOccupant->addWidget(icone);
    auto *textes = new QVBoxLayout();
    textes->setSpacing(1);
    if (occupe) {
        textes->addWidget(etiquette(occupant, QString("font-weight: bold; font-size: 11pt; color: %1;").arg(kMarine)));
        textes->addWidget(etiquette(monPoste ? employe.poste() + QString::fromUtf8(" · vous") : employe.poste(),
                                    QString("font-size: 10pt; color: %1;").arg(monPoste ? kBordeaux : kGris)));
    } else {
        textes->addWidget(etiquette("Aucun employé", QString("font-size: 11pt; color: %1;").arg(kGris)));
        textes->addWidget(etiquette(QString::fromUtf8("—"), QString("font-size: 10pt; color: %1;").arg(kGris)));
    }
    ligneOccupant->addLayout(textes, 1);
    colonne->addLayout(ligneOccupant);
    colonne->addStretch(1);   // l'espace en trop reste en bas de la carte, le bouton s'y ancre

    // Espace employe : prendre un poste libre, ou liberer le sien. Un poste occupe par
    // quelqu'un d'autre n'a pas de bouton. Prendre un autre poste libere l'ancien.
    if (m_actions && (monPoste || !occupe)) {
        QPushButton *bouton = new QPushButton(monPoste ? QString::fromUtf8("Libérer mon poste") : "Prendre ce poste");
        bouton->setCursor(Qt::PointingHandCursor);
        bouton->setMinimumHeight(36);
        bouton->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: white; border: none; border-radius: 6px; "
            "padding: 6px 10px; font-weight: bold; }"
            "QPushButton:hover { background-color: %2; }")
            .arg(monPoste ? kBordeaux : kMarine, monPoste ? "#6E1725" : "#2B4F68"));
        const QString cible = monPoste ? QString() : poste;
        connect(bouton, &QPushButton::clicked, this, [this, cible]() { emit changerPosteDemande(cible); });
        colonne->addWidget(bouton);
    } else if (m_actions) {
        // poste pris par un collegue : une ligne d'information a la place du bouton, les cartes gardent la meme structure
        QLabel *info = etiquette(QString::fromUtf8("Poste occupé par un collègue"), QString(
            "background-color: #F6EFE6; color: %1; border-radius: 6px; padding: 6px 10px; font-size: 10pt;").arg(kGris));
        info->setAlignment(Qt::AlignCenter);
        info->setMinimumHeight(36);
        colonne->addWidget(info);
    }
    return carte;
}

void PlanPostes::reconstruireCartes()
{
    while (QLayoutItem *item = m_grille->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    QStringList visibles;
    for (const QString &poste : std::as_const(m_postes)) {
        const bool occupe = m_occupants.contains(poste);
        if ((m_filtre == Filtre::Occupes && !occupe) || (m_filtre == Filtre::Libres && occupe))
            continue;
        visibles.append(poste);
    }
    m_visibles = visibles.size();
    m_colonnes = colonnesPour(m_visibles);

    for (int r = 0; r < m_grille->rowCount(); ++r)
        m_grille->setRowStretch(r, 0);
    int index = 0;
    int hauteurCarte = 0;
    for (const QString &poste : std::as_const(visibles)) {
        QFrame *carte = creerCarte(poste);
        hauteurCarte = qMax(hauteurCarte, qMax(carte->sizeHint().height(), carte->minimumHeight()));
        m_grille->addWidget(carte, index / m_colonnes, index % m_colonnes);
        ++index;
    }
    const int lignes = qMax(1, (index + m_colonnes - 1) / m_colonnes);

    if (m_defilementInterne) {
        m_zoneCartes->setMinimumHeight(0);
        m_zoneCartes->setMaximumHeight(QWIDGETSIZE_MAX);
    } else {
        // hauteur exacte de toutes les cartes : c'est la page qui defile, pas cette zone
        const int hauteur = lignes * hauteurCarte + (lignes - 1) * m_grille->spacing() + 16;
        m_zoneCartes->setFixedHeight(hauteur);
    }

    for (int c = 0; c < m_colonnes; ++c)
        m_grille->setColumnStretch(c, 1);
    // les lignes de cartes se partagent la hauteur ; une ligne vide en dessous absorbe le reste
    // quand les cartes ont atteint leur hauteur maximale
    for (int r = 0; r < lignes; ++r)
        m_grille->setRowStretch(r, 100);
    m_grille->setRowStretch(lignes, 1);
}

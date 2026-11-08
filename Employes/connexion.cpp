#include "connexion.h"
#include "validationui.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QToolButton>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QFont>
#include <QPainter>
#include <QPaintEvent>

Connexion::Connexion(QWidget *parent)
    : QDialog(parent)
{
    // Charge la photo de fond si elle existe dans les ressources.
    // Si le fichier n'est pas encore ajoute a ressources.qrc, m_imageFond reste "null"
    // et on retombe simplement sur le fond uni (voir paintEvent).
    m_imageFond = QPixmap(":/images/fond_connexion.png");

    construireInterface();
    chargerIdentifiantMemorise();
}

void Connexion::paintEvent(QPaintEvent *event)
{
    if (!m_imageFond.isNull()) {
        QPainter peintre(this);
        // "KeepAspectRatioByExpanding" remplit toute la fenetre sans deformer l'image
        // (elle peut deborder un peu sur un cote, on la centre pour rogner symetriquement).
        // On tient aussi compte du devicePixelRatio de l'ecran : sans ca, sur un ecran
        // avec mise a l'echelle Windows (125%, 150%...), l'image est redimensionnee en
        // pixels "logiques" puis ré-agrandie par Qt pour remplir l'ecran physique, ce qui
        // la rend floue meme si le fichier source est en haute resolution.
        const qreal ratioEcran = devicePixelRatioF();
        QPixmap imageMiseAEchelle = m_imageFond.scaled(size() * ratioEcran,
                                                        Qt::KeepAspectRatioByExpanding,
                                                        Qt::SmoothTransformation);
        imageMiseAEchelle.setDevicePixelRatio(ratioEcran);
        int x = (width() - int(imageMiseAEchelle.width() / ratioEcran)) / 2;
        int y = (height() - int(imageMiseAEchelle.height() / ratioEcran)) / 2;
        peintre.drawPixmap(x, y, imageMiseAEchelle);
    }
    QDialog::paintEvent(event);
}

void Connexion::construireInterface()
{
    setWindowTitle("Connexion - SmartStitch");
    resize(900, 560);
    setStyleSheet("QDialog { background-color: #F2EADD; }");

    // --- Carte centrale ---
    QFrame *carte = new QFrame(this);
    carte->setObjectName("carteConnexion");
    carte->setFixedWidth(380);
    carte->setStyleSheet(
        "#carteConnexion {"
        "  background-color: #FFFDF8;"
        "  border-radius: 16px;"
        "  border: 1px solid #E6DCC8;"
        "}"
        );

    QVBoxLayout *layoutCarte = new QVBoxLayout(carte);
    layoutCarte->setContentsMargins(36, 22, 36, 18);
    layoutCarte->setSpacing(8);

    // Logo SmartStitch : image a la place du texte.
    // Ajoute ton fichier logo dans ressources.qrc sous le prefixe /images,
    // et remplace "logo_smartstitch.png" ci-dessous par le nom exact utilise.
    QLabel *labelLogo = new QLabel(carte);
    QPixmap imageLogo(":/images/logo_smartstitch.png");
    if (!imageLogo.isNull()) {
        const qreal ratioEcran = devicePixelRatioF();
        QPixmap logoMisEAEchelle = imageLogo.scaledToHeight(
            int(110 * ratioEcran), Qt::SmoothTransformation);
        logoMisEAEchelle.setDevicePixelRatio(ratioEcran);
        labelLogo->setPixmap(logoMisEAEchelle);
    } else {
        // Repli texte tant que l'image n'est pas encore ajoutee aux ressources
        labelLogo->setTextFormat(Qt::RichText);
        labelLogo->setText(
            "<span style=\"color:#1F3A4D;\">Smart</span>"
            "<span style=\"color:#8B1E2F;\">Stitch</span>"
            );
        QFont policeLogo("Playfair Display", 26, QFont::Bold);
        labelLogo->setFont(policeLogo);
    }
    labelLogo->setAlignment(Qt::AlignCenter);

    // Ligne decorative
    QFrame *ligne = new QFrame(carte);
    ligne->setFixedHeight(1);
    ligne->setStyleSheet("background-color: #E6DCC8;");

    // Champ identifiant (email)
    m_champIdentifiant = new QLineEdit(carte);
    m_champIdentifiant->setPlaceholderText("Identifiant ou email");
    m_champIdentifiant->setFixedHeight(42);
    m_champIdentifiant->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #DED5C2;"
        "  border-radius: 8px;"
        "  padding: 0 12px;"
        "  background-color: #FFFFFF;"
        "}"
        "QLineEdit:focus { border: 1px solid #8B1E2F; }"
        );

    // Champ mot de passe + bouton Afficher/Masquer
    QFrame *conteneurMotDePasse = new QFrame(carte);
    conteneurMotDePasse->setFixedHeight(42);
    conteneurMotDePasse->setStyleSheet(
        "QFrame {"
        "  border: 1px solid #DED5C2;"
        "  border-radius: 8px;"
        "  background-color: #FFFFFF;"
        "}"
        );
    QHBoxLayout *layoutMotDePasse = new QHBoxLayout(conteneurMotDePasse);
    layoutMotDePasse->setContentsMargins(12, 0, 8, 0);
    layoutMotDePasse->setSpacing(4);

    m_champMotDePasse = new QLineEdit(conteneurMotDePasse);
    m_champMotDePasse->setPlaceholderText("Mot de passe");
    m_champMotDePasse->setEchoMode(QLineEdit::Password);
    m_champMotDePasse->setFrame(false);
    m_champMotDePasse->setStyleSheet("background: transparent;");

    m_boutonOeil = new QToolButton(conteneurMotDePasse);
    m_boutonOeil->setText("Afficher");
    m_boutonOeil->setCursor(Qt::PointingHandCursor);
    m_boutonOeil->setStyleSheet(
        "QToolButton { border: none; background: transparent; color: #8A8375; font-size: 10px; }"
        );
    connect(m_boutonOeil, &QToolButton::clicked, this, &Connexion::onBasculerAffichageMotDePasse);

    layoutMotDePasse->addWidget(m_champMotDePasse);
    layoutMotDePasse->addWidget(m_boutonOeil);

    // Ligne "Se souvenir de moi" / "Mot de passe oublie"
    QHBoxLayout *layoutOptions = new QHBoxLayout();
    m_caseSouvenir = new QCheckBox("Se souvenir de moi", carte);
    m_caseSouvenir->setStyleSheet("color: #5A5347; font-size: 11px;");

    QPushButton *boutonOublie = new QPushButton("Mot de passe oublié ?", carte);
    boutonOublie->setFlat(true);
    boutonOublie->setCursor(Qt::PointingHandCursor);
    boutonOublie->setStyleSheet(
        "QPushButton { border: none; color: #8B1E2F; font-size: 11px; }"
        "QPushButton:hover { text-decoration: underline; }"
        );
    connect(boutonOublie, &QPushButton::clicked, this, &Connexion::onMotDePasseOublieClicked);

    layoutOptions->addWidget(m_caseSouvenir);
    layoutOptions->addStretch();
    layoutOptions->addWidget(boutonOublie);

    // Message d'erreur (cache par defaut)
    m_labelErreur = new QLabel(carte);
    m_labelErreur->setStyleSheet("color: #B3261E; font-size: 11px;");
    m_labelErreur->setWordWrap(true);
    m_labelErreur->hide();

    // Bouton Se connecter
    QPushButton *boutonConnexion = new QPushButton("Se connecter", carte);
    m_boutonConnexion = boutonConnexion;
    boutonConnexion->setFixedHeight(44);
    boutonConnexion->setCursor(Qt::PointingHandCursor);
    boutonConnexion->setStyleSheet(
        "QPushButton {"
        "  background-color: #8B1E2F;"
        "  color: white;"
        "  border-radius: 8px;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #7A1A28; }"
        "QPushButton:pressed { background-color: #671621; }"
        "QPushButton:disabled { background-color: #D3C4C7; color: #F7F1F2; }"
        );
    connect(boutonConnexion, &QPushButton::clicked, this, &Connexion::onSeConnecterClicked);
    connect(m_champIdentifiant, &QLineEdit::returnPressed, boutonConnexion, &QPushButton::click);
    connect(m_champMotDePasse, &QLineEdit::returnPressed, boutonConnexion, &QPushButton::click);

    layoutCarte->addWidget(labelLogo);
    layoutCarte->addSpacing(4);
    layoutCarte->addWidget(ligne);
    layoutCarte->addSpacing(6);
    layoutCarte->addWidget(m_champIdentifiant);
    layoutCarte->addWidget(conteneurMotDePasse);
    layoutCarte->addLayout(layoutOptions);
    layoutCarte->addWidget(m_labelErreur);
    layoutCarte->addSpacing(4);
    layoutCarte->addWidget(boutonConnexion);

    // --- Validation en temps reel : message precis sous chaque champ, bouton desactive tant
    // que l'email ou le mot de passe est invalide. Les regles sont celles du Modele. ---
    m_champIdentifiant->setMaxLength(100);
    m_champMotDePasse->setMaxLength(100);
    m_validation = new ValidationFormulaire(this);
    m_validation->attacherChamp(m_champIdentifiant,
        [](const QString &v, QString *e) { return Employe::emailValide(v, e); });
    m_validation->attacherChamp(m_champMotDePasse,
        [](const QString &v, QString *e) { return Employe::motDePasseValide(v, e, true); },
        conteneurMotDePasse);
    connect(m_validation, &ValidationFormulaire::etatChange, boutonConnexion, &QPushButton::setEnabled);
    m_validation->evaluer();
    // le message "identifiant ou mot de passe incorrect" disparait des qu'on retape quelque chose
    connect(m_champIdentifiant, &QLineEdit::textEdited, m_labelErreur, &QLabel::hide);
    connect(m_champMotDePasse, &QLineEdit::textEdited, m_labelErreur, &QLabel::hide);

    // --- Mise en page generale : la carte centree dans la fenetre ---
    // Qt::AlignVCenter empeche la carte d'etre etiree sur toute la hauteur de la
    // fenetre : elle garde sa hauteur naturelle (celle du contenu) et c'est cette
    // hauteur naturelle qui determine le "rectangle moins long", pas seulement les
    // marges internes.
    QHBoxLayout *layoutPrincipal = new QHBoxLayout(this);
    layoutPrincipal->addStretch();
    layoutPrincipal->addWidget(carte, 0, Qt::AlignVCenter);
    layoutPrincipal->addStretch();
}

void Connexion::chargerIdentifiantMemorise()
{
    QSettings parametres;
    const QString identifiant = parametres.value("connexion/identifiant").toString();
    if (!identifiant.isEmpty()) {
        m_champIdentifiant->setText(identifiant);
        m_caseSouvenir->setChecked(true);
        m_champMotDePasse->setFocus();
    }
}

void Connexion::onBasculerAffichageMotDePasse()
{
    m_motDePasseVisible = !m_motDePasseVisible;
    m_champMotDePasse->setEchoMode(m_motDePasseVisible ? QLineEdit::Normal : QLineEdit::Password);
    m_boutonOeil->setText(m_motDePasseVisible ? "Masquer" : "Afficher");
}

void Connexion::onMotDePasseOublieClicked()
{
    QMessageBox::information(this, "Mot de passe oublié",
                              "Contacte un administrateur pour réinitialiser ton mot de passe.");
}

void Connexion::onSeConnecterClicked()
{
    const QString identifiant = m_champIdentifiant->text().trimmed();
    const QString motDePasse = m_champMotDePasse->text();

    // Normalement le bouton est desactive tant que les champs sont invalides ; on revalide
    // quand meme (touche Entree) et on place le curseur sur le premier champ en erreur.
    if (!m_validation->revalider()) {
        if (QWidget *fautif = m_validation->premierEnErreur())
            fautif->setFocus();
        return;
    }

    // Toute la logique SQL (recherche de l'employe + verification du mot de passe
    // haché) vit dans Employe::authentifier() -- cette fenetre (Vue) ne contient
    // aucune requete, cf. consigne de l'atelier CRUD du tuteur.
    Employe employeTrouve;
    QString erreur;
    if (!Employe::authentifier(identifiant, motDePasse, employeTrouve, &erreur)) {
        // erreur = message lisible fourni par le Modele (jamais d'erreur Oracle brute)
        m_labelErreur->setText(erreur.isEmpty()
            ? "Identifiant ou mot de passe incorrect."
            : erreur);
        m_labelErreur->show();
        return;
    }

    if (employeTrouve.statut().compare("Inactif", Qt::CaseInsensitive) == 0) {
        m_labelErreur->setText("Ce compte est désactivé. Contacte un administrateur.");
        m_labelErreur->show();
        return;
    }

    // On garde l'employe connecte en memoire (poste inclus) : la fenetre principale
    // s'en sert pour savoir quoi afficher selon le role. Les 4 postes possibles (Gerant,
    // Couturier, Vendeur, Styliste -- cf. contrainte CHECK en base sur EMPLOYE.POSTE) sont
    // tous pris en charge par la fenetre Employes : Gerant -> gestion des employes, les 3
    // autres -> espace personnel generique (voir GEmployes::appliquerRestrictionsSelonPoste()).
    m_employeConnecte = employeTrouve;

    QSettings parametres;
    if (m_caseSouvenir->isChecked())
        parametres.setValue("connexion/identifiant", identifiant);
    else
        parametres.remove("connexion/identifiant");

    accept(); // ferme la boite de dialogue en mode "Accepted"
}

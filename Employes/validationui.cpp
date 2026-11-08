#include "validationui.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QEvent>
#include <QLabel>
#include <QLineEdit>

ValidationFormulaire::ValidationFormulaire(QObject *parent)
    : QObject(parent)
{
}

QLabel *ValidationFormulaire::creerEtiquette(QWidget *parent)
{
    QLabel *etiquette = new QLabel(parent);
    etiquette->setStyleSheet("color: #C0392B; font-size: 8pt; background: transparent; border: none;");
    etiquette->setWordWrap(true);
    etiquette->hide();
    return etiquette;
}

// Cherche "cible" dans ce layout (ou dans ses layouts imbriques) et insere l'etiquette juste apres.
static bool insererApres(QLayout *layout, QWidget *cible, QLabel *etiquette)
{
    const int index = layout->indexOf(cible);
    if (index >= 0) {
        if (auto *boite = qobject_cast<QBoxLayout *>(layout)) {
            boite->insertWidget(index + 1, etiquette);
            return true;
        }
        return false;
    }
    for (int i = 0; i < layout->count(); ++i) {
        if (QLayout *sous = layout->itemAt(i)->layout())
            if (insererApres(sous, cible, etiquette))
                return true;
    }
    return false;
}

bool ValidationFormulaire::placerSous(QWidget *cible, QLabel *etiquette)
{
    QWidget *parent = cible->parentWidget();
    if (!parent || !parent->layout())
        return false;
    etiquette->setParent(parent);
    return insererApres(parent->layout(), cible, etiquette);
}

void ValidationFormulaire::attacherChamp(QLineEdit *champ, Verificateur verificateur, QWidget *cadre)
{
    Champ c;
    c.widget = champ;
    c.cadre = cadre ? cadre : champ;
    c.etiquette = creerEtiquette(champ->parentWidget());
    c.verificateur = verificateur;
    c.styleBase = c.cadre->styleSheet();
    placerSous(c.cadre, c.etiquette);
    m_champs.append(c);

    // frappe : on controle le format (un champ vide n'est pas encore une erreur) ;
    // sortie du champ : un champ obligatoire vide devient une erreur.
    connect(champ, &QLineEdit::textEdited, this, [this, champ]() {
        if (const Champ *c = trouver(champ))
            controler(*c, false);
    });
    connect(champ, &QLineEdit::editingFinished, this, [this, champ]() {
        if (const Champ *c = trouver(champ))
            controler(*c, true);
    });
}

void ValidationFormulaire::attacherCombo(QComboBox *combo, Verificateur verificateur, QLabel *etiquette)
{
    Champ c;
    c.widget = combo;
    c.cadre = combo;
    c.etiquette = etiquette;
    c.verificateur = verificateur;
    c.styleBase = combo->styleSheet();
    etiquette->setStyleSheet("color: #C0392B; font-size: 8pt; background: transparent; border: none;");
    etiquette->setWordWrap(true);
    etiquette->hide();
    m_champs.append(c);

    connect(combo, &QComboBox::currentIndexChanged, this, [this, combo]() {
        if (const Champ *c = trouver(combo))
            controler(*c, true);
    });
    combo->installEventFilter(this); // sortie du champ -> "Veuillez choisir ..."
}

bool ValidationFormulaire::eventFilter(QObject *objet, QEvent *evenement)
{
    if (evenement->type() == QEvent::FocusOut) {
        if (const Champ *c = trouver(objet))
            controler(*c, true);
    }
    return QObject::eventFilter(objet, evenement);
}

const ValidationFormulaire::Champ *ValidationFormulaire::trouver(QObject *widget) const
{
    for (const Champ &c : m_champs)
        if (c.widget == widget)
            return &c;
    return nullptr;
}

QString ValidationFormulaire::valeurDe(const Champ &champ)
{
    if (auto *ligne = qobject_cast<QLineEdit *>(champ.widget))
        return ligne->text();
    if (auto *combo = qobject_cast<QComboBox *>(champ.widget))
        return combo->currentData().toString();
    return QString();
}

void ValidationFormulaire::montrer(const Champ &champ, const QString &message)
{
    champ.etiquette->setText(message);
    champ.etiquette->show();
    // la bordure rouge s'ajoute a la feuille de style d'origine (la derniere regle l'emporte)
    champ.cadre->setStyleSheet(champ.styleBase + QString("\n%1 { border: 1.5px solid #C0392B; }")
                                                     .arg(champ.cadre->metaObject()->className()));
}

void ValidationFormulaire::cacher(const Champ &champ)
{
    champ.etiquette->hide();
    champ.cadre->setStyleSheet(champ.styleBase);
}

void ValidationFormulaire::controler(const Champ &champ, bool afficherSiVide)
{
    QString message;
    const QString valeur = valeurDe(champ);
    const bool valide = champ.verificateur(valeur, &message);

    if (valide || (valeur.trimmed().isEmpty() && !afficherSiVide))
        cacher(champ);
    else
        montrer(champ, message);

    emit etatChange(toutValide());
}

bool ValidationFormulaire::toutValide() const
{
    for (const Champ &c : m_champs)
        if (!c.verificateur(valeurDe(c), nullptr))
            return false;
    return true;
}

bool ValidationFormulaire::revalider()
{
    bool tout = true;
    for (const Champ &c : std::as_const(m_champs)) {
        QString message;
        if (c.verificateur(valeurDe(c), &message)) {
            cacher(c);
        } else {
            montrer(c, message);
            tout = false;
        }
    }
    emit etatChange(tout);
    return tout;
}

QWidget *ValidationFormulaire::premierEnErreur() const
{
    for (const Champ &c : m_champs)
        if (!c.verificateur(valeurDe(c), nullptr))
            return c.widget;
    return nullptr;
}

void ValidationFormulaire::afficherErreur(QWidget *champ, const QString &message)
{
    if (const Champ *c = trouver(champ))
        montrer(*c, message);
}

void ValidationFormulaire::effacerMessages()
{
    for (const Champ &c : std::as_const(m_champs))
        cacher(c);
}

void ValidationFormulaire::evaluer()
{
    emit etatChange(toutValide());
}

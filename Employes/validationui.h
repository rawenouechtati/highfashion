#ifndef VALIDATIONUI_H
#define VALIDATIONUI_H

#include <QList>
#include <QObject>
#include <QString>
#include <functional>

class QComboBox;
class QLabel;
class QLineEdit;
class QWidget;

// Regle de validation : renvoie true si la valeur est acceptable, sinon false et remplit
// *erreur avec le message a afficher. Les regles elles-memes vivent dans le Modele
// (ex. Employe::telephoneValide) ; la Vue les branche ici sans en connaitre le detail.
using Verificateur = std::function<bool(const QString &valeur, QString *erreur)>;

// Validation en temps reel d'un formulaire : sous chaque champ, un message rouge precis et
// une bordure rouge des qu'il est invalide ; un signal indique si tout le formulaire est
// valide (pour activer/desactiver le bouton Enregistrer). Aucune logique metier ici.
class ValidationFormulaire : public QObject
{
    Q_OBJECT

public:
    explicit ValidationFormulaire(QObject *parent = nullptr);

    // Champ texte : controle a chaque frappe (format) et a la sortie du champ (obligatoire).
    // L'etiquette d'erreur est creee et placee juste sous le champ, dans son layout ; "cadre"
    // est le widget qui porte la bordure et sous lequel on place le message (par defaut le
    // champ lui-meme ; utile quand le champ est dans un conteneur borde).
    void attacherChamp(QLineEdit *champ, Verificateur verificateur, QWidget *cadre = nullptr);

    // Liste deroulante : controlee au choix et a la sortie du champ. L'etiquette est deja
    // placee par l'appelant. La valeur transmise a la regle est la donnee (userData) de
    // l'element choisi, ou une chaine vide si rien n'est choisi.
    void attacherCombo(QComboBox *combo, Verificateur verificateur, QLabel *etiquette);

    // Etiquette d'erreur au style standard (rouge, petite, retour a la ligne, cachee).
    static QLabel *creerEtiquette(QWidget *parent = nullptr);
    // Place "etiquette" dans le layout du widget "cible", juste apres lui.
    static bool placerSous(QWidget *cible, QLabel *etiquette);

    bool toutValide() const;
    // Controle tous les champs et AFFICHE les messages ; renvoie true si tout est valide.
    bool revalider();
    // Premier champ invalide (dans l'ordre d'attachement), nullptr si tout est valide.
    QWidget *premierEnErreur() const;

    // Affiche un message sous un champ (ex. erreur renvoyee par le Modele apres l'ecriture).
    void afficherErreur(QWidget *champ, const QString &message);
    // Cache tous les messages et retire les bordures rouges (formulaire remis a zero).
    void effacerMessages();
    // Recalcule l'etat global sans rien afficher, puis emet etatChange().
    void evaluer();

signals:
    void etatChange(bool toutValide);

protected:
    bool eventFilter(QObject *objet, QEvent *evenement) override;

private:
    struct Champ {
        QWidget *widget = nullptr;   // QLineEdit ou QComboBox
        QWidget *cadre = nullptr;    // widget qui porte la bordure rouge
        QLabel *etiquette = nullptr;
        Verificateur verificateur;
        QString styleBase;           // feuille de style d'origine du cadre
    };

    static QString valeurDe(const Champ &champ);
    void controler(const Champ &champ, bool afficherSiVide);
    void montrer(const Champ &champ, const QString &message);
    void cacher(const Champ &champ);
    const Champ *trouver(QObject *widget) const;

    QList<Champ> m_champs;
};

#endif // VALIDATIONUI_H

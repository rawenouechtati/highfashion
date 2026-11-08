#ifndef CONNEXION_H
#define CONNEXION_H

#include <QDialog>
#include <QPixmap>
#include "employe.h"

class QLineEdit;
class QLabel;
class QCheckBox;
class QToolButton;
class QPushButton;
class ValidationFormulaire;

// Boite de dialogue de connexion affichee au lancement de l'application.
// Verifie l'identifiant (email) + mot de passe contre la table EMPLOYE
// avant de laisser l'utilisateur acceder a la fenetre principale.
class Connexion : public QDialog
{
    Q_OBJECT

public:
    explicit Connexion(QWidget *parent = nullptr);

    // Disponible seulement apres une connexion reussie (exec() == Accepted) :
    // donne acces a l'employe qui vient de se connecter (poste inclus), pour que
    // la fenetre principale puisse adapter ce qu'elle affiche a son role.
    Employe employeConnecte() const { return m_employeConnecte; }

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onSeConnecterClicked();
    void onBasculerAffichageMotDePasse();
    void onMotDePasseOublieClicked();

private:
    void construireInterface();
    void chargerIdentifiantMemorise();

    QLineEdit *m_champIdentifiant = nullptr;
    QLineEdit *m_champMotDePasse = nullptr;
    QCheckBox *m_caseSouvenir = nullptr;
    QLabel *m_labelErreur = nullptr;
    QToolButton *m_boutonOeil = nullptr;
    QPushButton *m_boutonConnexion = nullptr;
    ValidationFormulaire *m_validation = nullptr; // messages d'erreur sous les champs
    bool m_motDePasseVisible = false;
    QPixmap m_imageFond; // photo d'arriere-plan (vide si le fichier n'existe pas)
    Employe m_employeConnecte; // rempli juste avant accept(), vide tant que non connecte
};

#endif // CONNEXION_H

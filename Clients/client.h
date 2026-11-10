#ifndef CLIENT_H
#define CLIENT_H

#include <QString>
#include <QStringList>
#include <QSqlQuery>

// Entite CLIENT de la base Oracle v5 :
//   ID, NOM, PRENOM, EMAIL (unique), TELEPHONE, NIVEAU (Bronze/Argent/Or), STATUT
// Toutes les requetes SQL sont ici (modele-vue) : l'interface n'appelle que ces methodes.
class Client
{
public:
    Client() {}
    explicit Client(int id) : id(id) {}
    Client(int id, const QString &nom, const QString &prenom, const QString &email,
           const QString &tel, const QString &niveau = "Bronze", const QString &statut = "Actif");

    // controle de saisie (meme regles que les contraintes Oracle) ; message = premiere erreur trouvee
    bool valider(QString *message = nullptr) const;

    bool ajouter();
    bool modifier();
    bool supprimer();
    static bool afficher(QSqlQuery &q);

    static QStringList niveauxAutorises();   // Bronze, Argent, Or
    static QStringList statutsAutorises();   // Actif, Inactif

    QString derniereErreur() const { return erreur; }

private:
    int id = 0;
    QString nom, prenom, email, tel, niveau = "Bronze", statut = "Actif";
    QString erreur;
};

#endif // CLIENT_H

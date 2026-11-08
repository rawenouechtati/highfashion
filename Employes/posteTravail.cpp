#include "posteTravail.h"

QStringList PosteTravail::tous()
{
    // Meme liste et meme orthographe que la contrainte CHECK de EMPLOYE.POSTE_TRAVAIL.
    return {"Couture 1", "Couture 2", "Nettoyage", "Authentification", "Pressing", "Controle qualite"};
}

bool PosteTravail::estValide(const QString &poste)
{
    return tous().contains(poste);
}

QString PosteTravail::libelle(const QString &poste)
{
    if (poste.startsWith("Couture"))
        return "Poste de couture" + poste.mid(QString("Couture").size());
    if (poste == "Nettoyage")
        return "Poste de nettoyage";
    if (poste == "Authentification")
        return QString::fromUtf8("Poste d'authentification");
    if (poste == "Pressing")
        return "Poste de pressing";
    if (poste == "Controle qualite")
        return QString::fromUtf8("Poste de contrôle qualité");
    return poste;
}

QString PosteTravail::libelleCourt(const QString &poste)
{
    if (poste == "Controle qualite")
        return QString::fromUtf8("Contrôle qualité");
    return poste;   // les autres postes portent deja un nom court
}

QString PosteTravail::type(const QString &poste)
{
    return poste.startsWith("Couture") ? QString("Couture") : poste;
}

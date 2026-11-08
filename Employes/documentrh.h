#ifndef DOCUMENTRH_H
#define DOCUMENTRH_H

#include <QDate>
#include <QRectF>
#include <QString>
#include "employe.h"

class QPainter;

// Generation des documents RH en PDF (pas de base de donnees ici : on recoit l'employe
// deja charge). Pour l'instant un seul document : la demande de conge.
class DocumentRH
{
public:
    // Ecrit la demande de conge pre-remplie avec les informations de l'employe dans
    // "chemin" (PDF A4). Renvoie false et remplit *erreur si le fichier ne peut pas etre ecrit.
    static bool exporterDemandeConge(const Employe &employe, const QDate &debut, const QDate &fin,
                                     const QString &motif, const QString &chemin,
                                     QString *erreur = nullptr);

    // Dessine la demande de conge dans "page" (rectangle de la page, de proportions A4).
    // Utilise par l'export PDF ; accessible pour dessiner sur une image (apercu, tests).
    static void dessinerDemandeConge(QPainter &p, const QRectF &page, const Employe &employe,
                                     const QDate &debut, const QDate &fin, const QString &motif,
                                     const QDate &dateDemande);
};

#endif // DOCUMENTRH_H

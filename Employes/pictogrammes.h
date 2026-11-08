#ifndef PICTOGRAMMES_H
#define PICTOGRAMMES_H

#include <QColor>
#include <QPixmap>
#include <QRectF>
#include <QString>

class QPainter;

// Pictogrammes dessines au QPainter (aucun fichier image a livrer, nets a toutes les tailles).

// Pictogramme d'un type de poste (Couture, Nettoyage, Authentification, Pressing,
// Controle qualite), dessine dans un carre de cote 2 centre en (0,0).
void dessinerPictogrammePoste(QPainter &p, const QString &type, const QColor &couleur);

// Meme pictogramme dans un QPixmap carre (taille en pixels logiques, net sur ecrans HiDPI).
QPixmap pixmapPictogrammePoste(const QString &type, const QColor &couleur, int taille);

enum class Icone { Personne, Groupe, Mallette, Enveloppe, Telephone, Calendrier, Horloge, Document, Loupe, Repartition };

// Icone "ligne" dessinee dans le rectangle donne.
void dessinerIcone(QPainter &p, Icone icone, const QRectF &zone, const QColor &couleur);
QPixmap pixmapIcone(Icone icone, const QColor &couleur, int taille);

#endif // PICTOGRAMMES_H

// codeqr.h — générateur de QR code (norme ISO/IEC 18004, modèle 2) écrit pour le module Articles.
//
// Mode « octets » (texte UTF-8), niveau de correction d'erreurs L ou M, versions 1 à 20 (jusqu'à 858 octets).
// Aucune bibliothèque externe : seulement Qt (QImage pour l'affichage et l'insertion dans un PDF).
#pragma once
#include <QByteArray>
#include <QImage>
#include <QVector>

namespace Art {

class CodeQr
{
public:
    enum Correction { L = 0, M = 1 };   // ~7 % ou ~15 % de modules récupérables

    // Encode le texte ; renvoie false si le texte est trop long pour la version 20 (le QR reste alors vide).
    static bool encoder(const QByteArray &octets, CodeQr &resultat, Correction niveau = M);

    int taille() const { return taille_; }                  // nombre de modules par côté
    int version() const { return version_; }
    bool module(int x, int y) const;                        // true = module noir
    QImage image(int pixelsParModule = 8, int bordure = 4) const;   // noir sur blanc, bordure blanche de 4 modules

private:
    int version_ = 0, taille_ = 0;
    QVector<quint8> modules_;      // taille_ * taille_ (0 ou 1)
    QVector<quint8> fonction_;     // 1 = module de motif fixe (ne reçoit pas de données)

    void tracerMotifs(int niveauFormat);
    void tracerFormat(int niveauFormat, int masque);
    void placerDonnees(const QVector<quint8> &mots);
    void appliquerMasque(int masque);
    long penalite() const;
    void fixer(int x, int y, bool noir) { modules_[y * taille_ + x] = noir; fonction_[y * taille_ + x] = 1; }
};

}  // namespace Art

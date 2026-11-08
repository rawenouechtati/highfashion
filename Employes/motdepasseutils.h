#ifndef MOTDEPASSEUTILS_H
#define MOTDEPASSEUTILS_H

#include <QString>
#include <QCryptographicHash>

// Hache un mot de passe en clair avec SHA-256 (empreinte hexadecimale, 64 caracteres).
// A utiliser partout ou on stocke OU verifie un mot de passe : on ne compare/stocke
// jamais le mot de passe en clair, seulement son empreinte.
inline QString hacherMotDePasse(const QString &motDePasseClair)
{
    const QByteArray donnees = motDePasseClair.toUtf8();
    const QByteArray empreinte = QCryptographicHash::hash(donnees, QCryptographicHash::Sha256);
    return QString::fromLatin1(empreinte.toHex());
}

#endif // MOTDEPASSEUTILS_H

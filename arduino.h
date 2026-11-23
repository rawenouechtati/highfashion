#ifndef ARDUINO_H
#define ARDUINO_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QString>

// Classe C++ pure de communication Qt <-> Arduino (liaison série USB, 9600 bauds).
// Protocole : une commande = une ligne de texte terminée par '\n'.
// Ajouter dans le .pro :   QT += serialport
class Arduino : public QObject
{
    Q_OBJECT
public:
    explicit Arduino(QObject *parent = nullptr);
    ~Arduino();

    // nomPort vide = détection automatique de la carte (Uno, Nano CH340, FTDI...)
    bool connecter(const QString &nomPort = QString());
    void deconnecter();
    bool estConnecte() const;
    QString portUtilise() const;

    bool envoyer(const QString &ligne);          // ajoute '\n' automatiquement

    static QString sansAccents(const QString &t); // l'écran LCD n'affiche pas les accents

signals:
    void pret();                                  // la carte a envoyé "READY" (elle a fini de redémarrer)
    void ligneRecue(const QString &ligne);        // une ligne complète reçue de la carte
    void etatChange(bool connecte);               // branchée / débranchée

private slots:
    void lire();
    void onErreur(QSerialPort::SerialPortError e);

private:
    static QString detecterPort();

    QSerialPort m_port;
    QByteArray  m_tampon;
};

#endif // ARDUINO_H
#include "arduino.h"
#include <QSerialPortInfo>
#include <QDebug>
#include <QChar>

Arduino::Arduino(QObject *parent) : QObject(parent)
{
    connect(&m_port, &QSerialPort::readyRead, this, &Arduino::lire);
    connect(&m_port, &QSerialPort::errorOccurred, this, &Arduino::onErreur);
}

Arduino::~Arduino() { deconnecter(); }

// Cherche une carte Arduino (ou clone) parmi les ports série disponibles
QString Arduino::detecterPort()
{
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &p : ports) {
        if (p.hasVendorIdentifier()) {
            const quint16 v = p.vendorIdentifier();
            // 0x2341/0x2A03 = Arduino, 0x1A86 = CH340, 0x0403 = FTDI, 0x10C4 = CP210x
            if (v == 0x2341 || v == 0x2A03 || v == 0x1A86 || v == 0x0403 || v == 0x10C4)
                return p.portName();
        }
        if (p.description().contains("Arduino", Qt::CaseInsensitive))
            return p.portName();
    }
    return QString();
}

bool Arduino::connecter(const QString &nomPort)
{
    if (m_port.isOpen()) return true;

    const QString nom = nomPort.isEmpty() ? detecterPort() : nomPort;
    if (nom.isEmpty()) {
        qDebug() << "Arduino : aucune carte détectée";
        return false;
    }
    m_port.setPortName(nom);
    m_port.setBaudRate(QSerialPort::Baud9600);       // même vitesse que Serial.begin(9600)
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadWrite)) {
        qDebug() << "Arduino : ouverture impossible :" << m_port.errorString();
        return false;
    }
    m_tampon.clear();
    emit etatChange(true);
    return true;                                     // la carte redémarre : attendre le signal pret()
}

void Arduino::deconnecter()
{
    if (m_port.isOpen()) {
        m_port.close();
        emit etatChange(false);
    }
}

bool Arduino::estConnecte() const { return m_port.isOpen(); }
QString Arduino::portUtilise() const { return m_port.portName(); }

bool Arduino::envoyer(const QString &ligne)
{
    if (!m_port.isOpen()) return false;
    const QByteArray data = sansAccents(ligne).toLatin1() + '\n';
    return m_port.write(data) == data.size();
}

// Reçoit les octets par morceaux : on ne traite que les lignes complètes
void Arduino::lire()
{
    m_tampon += m_port.readAll();
    int fin;
    while ((fin = m_tampon.indexOf('\n')) >= 0) {
        const QString ligne = QString::fromLatin1(m_tampon.left(fin)).trimmed();
        m_tampon.remove(0, fin + 1);
        if (ligne.isEmpty()) continue;
        if (ligne == "READY") emit pret();
        emit ligneRecue(ligne);
    }
}

// Carte débranchée en cours d'utilisation
void Arduino::onErreur(QSerialPort::SerialPortError e)
{
    if (e == QSerialPort::ResourceError) {
        qDebug() << "Arduino : carte débranchée";
        deconnecter();
    }
}

QString Arduino::sansAccents(const QString &t)
{
    QString d = t.normalized(QString::NormalizationForm_D);
    QString r;
    for (const QChar c : d)
        if (c.category() != QChar::Mark_NonSpacing) r += c;
    return r;
}
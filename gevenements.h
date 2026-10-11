#ifndef GEVENEMENTS_H
#define GEVENEMENTS_H

#include <QDate>
#include <QMainWindow>
#include <QString>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class GEvenements; }
QT_END_NAMESPACE

struct Evenement
{
    int     id = 0;
    QString nom, type, statut, date, heure, lieu, client, description, image;
};

class GEvenements : public QMainWindow
{
    Q_OBJECT

public:
    explicit GEvenements(QWidget *parent = nullptr);
    ~GEvenements() override;

private slots:
    // --- slots déclarés dans mainwindow.ui (Éditeur de signaux/slots) ---
    void enregistrerEvenement();
    void supprimerEvenement();
    void modifierEvenement();
    void choisirImage();
    void rechercher(const QString &texte);
    void trierEvenements(int index);
    void filtrerEvenements(int index);

    // --- slots complémentaires ---
    void onAjouter();
    void onAnnuler();
    void onReinitialiser();
    void onSelectionChanged();
    void onCalendarClicked(const QDate &date);
    void onCalendarPageChanged(int year, int month);

private:
    // données (events.json)
    void loadEvents();
    void saveEvents() const;
    void seedEvents();
    QString dataDir() const;
    int  nextId() const;
    const Evenement *findById(int id) const;

    // affichage
    void refreshAll(int selectId = -1);
    void fillTable(int selectId);
    QVector<Evenement> filteredSorted() const;
    void updateFilterCombos();
    void updateCards();
    void updateBars();
    void updateLieux();
    void updateCalendarMarks();
    QDate initialCalendarDate() const;
    static QWidget *makeBadge(const QString &statut);

    // fiche événement
    int  selectedId() const;
    void clearForm();
    void fillForm(const Evenement &e);
    Evenement formData();
    void setPreview(const QString &path);
    QString storeImage(const QString &path) const;

    Ui::GEvenements *ui;
    QVector<Evenement> m_events;    // toutes les données
    QVector<Evenement> m_visible;   // lignes affichées (après filtres + tri)
    int     m_currentId = -1;       // -1 = nouvel événement
    QString m_currentImage;
};

#endif // GEVENEMENTS_H

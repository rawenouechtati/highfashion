#include "gevenements.h"
#include "ui_gevenements.h"
#include "evolutionchart.h"

#include <QCollator>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPixmap>
#include <QProgressBar>
#include <QStandardPaths>
#include <QTextCharFormat>
#include <QTime>

#include <algorithm>
#include <climits>

namespace {

const QStringList kTypes = { "Défilé", "Atelier", "Vente", "Salon", "Lancement", "Autre" };
const char *kDateFmt = "yyyy-MM-dd";

// clé utilisée par le QSS : QLabel[badge="programme"] { ... }
QString badgeKey(const QString &statut)
{
    static const QHash<QString, QString> map = {
        { "Programmé", "programme" }, { "À venir", "avenir" }, { "Planifié", "planifie" },
        { "En cours", "encours" },    { "Terminé", "termine" }
    };
    return map.value(statut, "termine");
}

QJsonObject toJson(const Evenement &e)
{
    return QJsonObject{ { "id", e.id },           { "nom", e.nom },       { "type", e.type },
                        { "statut", e.statut },   { "date", e.date },     { "heure", e.heure },
                        { "lieu", e.lieu },       { "client", e.client }, { "description", e.description },
                        { "image", e.image } };
}

Evenement fromJson(const QJsonObject &o)
{
    Evenement e;
    e.id = o["id"].toInt();
    e.nom = o["nom"].toString();
    e.type = o["type"].toString();
    e.statut = o["statut"].toString();
    e.date = o["date"].toString();
    e.heure = o["heure"].toString();
    e.lieu = o["lieu"].toString();
    e.client = o["client"].toString();
    e.description = o["description"].toString();
    e.image = o["image"].toString();
    return e;
}

} // namespace

// ======================================================================
//  Construction
// ======================================================================
GEvenements::GEvenements(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::GEvenements)
{
    ui->setupUi(this);   // construit l'interface + connexions définies dans le .ui

    // Réapplique la feuille de style du .ui : certains widgets (dans la zone de
    // défilement) sont « polis » avant d'avoir un parent, donc sans style.
    {
        const QString qss = styleSheet();
        setStyleSheet(QString());
        setStyleSheet(qss);
    }

    const QLocale fr(QLocale::French, QLocale::France);
    ui->calendar->setLocale(fr);
    ui->dateEdit->setLocale(fr);

    // Connexions complémentaires (celles qui ne sont pas dans le .ui)
    connect(ui->btnAjouter,       &QPushButton::clicked, this, &GEvenements::onAjouter);
    connect(ui->btnAnnuler,       &QPushButton::clicked, this, &GEvenements::onAnnuler);
    connect(ui->btnReinitialiser, &QPushButton::clicked, this, &GEvenements::onReinitialiser);
    connect(ui->tableEvents,      &QTableWidget::itemSelectionChanged, this, &GEvenements::onSelectionChanged);
    connect(ui->calendar,         &QCalendarWidget::clicked,           this, &GEvenements::onCalendarClicked);
    connect(ui->calendar,         &QCalendarWidget::currentPageChanged, this, &GEvenements::onCalendarPageChanged);

    loadEvents();
    ui->calendar->setSelectedDate(initialCalendarDate());
    clearForm();
    refreshAll();
    if (ui->tableEvents->rowCount() > 0)
        ui->tableEvents->selectRow(0);
}

GEvenements::~GEvenements()
{
    delete ui;
}

// ======================================================================
//  Données : events.json (dans le dossier AppData de l'utilisateur)
// ======================================================================
QString GEvenements::dataDir() const
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (d.isEmpty())
        d = QDir::homePath() + "/.gevents";
    QDir().mkpath(d);
    return d;
}

void GEvenements::loadEvents()
{
    m_events.clear();
    QFile f(dataDir() + "/events.json");
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        for (const QJsonValue &v : doc.array())
            m_events.push_back(fromJson(v.toObject()));
    }
    if (m_events.isEmpty()) {
        seedEvents();
        saveEvents();
    }
}

void GEvenements::saveEvents() const
{
    QJsonArray arr;
    for (const Evenement &e : m_events)
        arr.append(toJson(e));
    QFile f(dataDir() + "/events.json");
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
}

void GEvenements::seedEvents()
{
    auto add = [this](const QString &nom, const QString &type, const QString &statut, const QString &date,
                      const QString &heure, const QString &lieu, const QString &client, const QString &desc) {
        Evenement e;
        e.id = nextId();
        e.nom = nom; e.type = type; e.statut = statut; e.date = date;
        e.heure = heure; e.lieu = lieu; e.client = client; e.description = desc;
        m_events.push_back(e);
    };
    add("Défilé de mode Printemps", "Défilé", "Programmé", "2025-05-10", "18:00", "Salle El Manar", "Client 1",
        "Défilé de mode pour la nouvelle collection printemps. Présence de plusieurs influenceurs et médias.");
    add("Atelier Couture", "Atelier", "Programmé", "2025-05-15", "15:00", "Atelier Vintage", "Client 2", "");
    add("Vente Privée", "Vente", "À venir", "2025-06-01", "18:00", "Boutique", "Client 3", "");
    add("Salon du Mariage", "Salon", "À venir", "2025-06-20", "09:00", "Parc des Expositions", "Client 4", "");
    add("Lancement Collection", "Lancement", "À venir", "2025-07-05", "11:00", "Hôtel Laico", "Client 5", "");
    add("Défilé d'Hiver", "Défilé", "Planifié", "2025-12-15", "18:00", "Salle El Mouradi", "Client 6", "");
    add("Journée Portes Ouvertes", "Autre", "Planifié", "2025-09-10", "10:00", "Atelier Vintage", "Client 7", "");
}

int GEvenements::nextId() const
{
    int m = 0;
    for (const Evenement &e : m_events)
        m = std::max(m, e.id);
    return m + 1;
}

const Evenement *GEvenements::findById(int id) const
{
    for (const Evenement &e : m_events)
        if (e.id == id)
            return &e;
    return nullptr;
}

// ======================================================================
//  Rafraîchissement de l'affichage
// ======================================================================
void GEvenements::refreshAll(int selectId)
{
    updateFilterCombos();
    fillTable(selectId >= 0 ? selectId : m_currentId);
    updateCards();
    updateBars();
    updateLieux();
    updateCalendarMarks();
}

QVector<Evenement> GEvenements::filteredSorted() const
{
    const QString q = ui->lineSearch->text().trimmed();
    QVector<Evenement> res;
    for (const Evenement &e : m_events) {
        if (!q.isEmpty()) {
            const QString hay = e.nom + " " + e.type + " " + e.lieu + " " + e.client;
            if (!hay.contains(q, Qt::CaseInsensitive))
                continue;
        }
        if (ui->comboStatut->currentIndex() > 0 && e.statut != ui->comboStatut->currentText()) continue;
        if (ui->comboType->currentIndex() > 0   && e.type   != ui->comboType->currentText())   continue;
        if (ui->comboLieu->currentIndex() > 0   && e.lieu   != ui->comboLieu->currentText())   continue;
        if (ui->comboClient->currentIndex() > 0 && e.client != ui->comboClient->currentText()) continue;
        res.push_back(e);
    }

    // ----- tri -----
    const int mode = ui->comboTri->currentIndex();
    const QDate today = QDate::currentDate();
    QHash<QString, int> perLieu;                       // = panneau « Événements par lieu »
    for (const Evenement &e : m_events)
        ++perLieu[e.lieu];
    QCollator col{ QLocale(QLocale::French) };         // tri alphabétique correct (é = e)
    col.setCaseSensitivity(Qt::CaseInsensitive);

    auto dist = [&](const Evenement &e) {
        const QDate d = QDate::fromString(e.date, kDateFmt);
        return d.isValid() ? int(qAbs(today.daysTo(d))) : INT_MAX / 2;
    };
    auto less = [&](const Evenement &a, const Evenement &b) -> bool {
        switch (mode) {
        case 0: case 1: {                               // Date (proche → lointaine)
            if (dist(a) != dist(b)) return dist(a) < dist(b);
            if (a.date != b.date)   return a.date < b.date;
            return a.heure < b.heure;
        }
        case 2:                                         // Lieu : le plus d'événements d'abord
            if (perLieu[a.lieu] != perLieu[b.lieu]) return perLieu[a.lieu] > perLieu[b.lieu];
            if (a.lieu != b.lieu) return col.compare(a.lieu, b.lieu) < 0;
            return a.date < b.date;
        case 3:                                         // Lieu A → Z
            if (a.lieu != b.lieu) return col.compare(a.lieu, b.lieu) < 0;
            return a.date < b.date;
        default:                                        // Nom A → Z / Z → A
            return col.compare(a.nom, b.nom) < 0;
        }
    };
    const bool reverse = (mode == 1 || mode == 5);
    std::stable_sort(res.begin(), res.end(), [&](const Evenement &a, const Evenement &b) {
        return reverse ? less(b, a) : less(a, b);
    });
    return res;
}

QWidget *GEvenements::makeBadge(const QString &statut)
{
    auto *lbl = new QLabel(statut);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setProperty("badge", badgeKey(statut));       // stylé par le QSS du .ui
    auto *wrap = new QWidget;
    wrap->setObjectName("badgeWrap");
    wrap->setStyleSheet("QWidget#badgeWrap { background: transparent; }");
    auto *lay = new QHBoxLayout(wrap);
    lay->setContentsMargins(4, 8, 4, 8);
    lay->addWidget(lbl);
    return wrap;
}

void GEvenements::fillTable(int selectId)
{
    QTableWidget *t = ui->tableEvents;
    m_visible = filteredSorted();

    const bool old = t->blockSignals(true);
    t->clearSelection();
    t->setRowCount(0);
    t->setRowCount(m_visible.size());
    int target = -1;
    for (int r = 0; r < m_visible.size(); ++r) {
        const Evenement &e = m_visible[r];
        const QStringList cells = { QString::number(e.id), e.nom, e.type, e.date, e.lieu };
        for (int c = 0; c < cells.size(); ++c)
            t->setItem(r, c, new QTableWidgetItem(cells[c]));
        t->setCellWidget(r, 5, makeBadge(e.statut));
        if (e.id == selectId)
            target = r;
    }
    if (target >= 0)
        t->selectRow(target);
    t->blockSignals(old);
}

void GEvenements::updateFilterCombos()
{
    struct Spec { QComboBox *combo; QString Evenement::*field; };
    const Spec specs[] = { { ui->comboLieu, &Evenement::lieu }, { ui->comboClient, &Evenement::client } };
    for (const Spec &s : specs) {
        const QString first = s.combo->count() ? s.combo->itemText(0) : QString();
        const QString cur = s.combo->currentText();
        QStringList values;
        for (const Evenement &e : m_events)
            if (!(e.*s.field).isEmpty() && !values.contains(e.*s.field))
                values << e.*s.field;
        values.sort(Qt::CaseInsensitive);

        const bool old = s.combo->blockSignals(true);
        s.combo->clear();
        s.combo->addItem(first);
        s.combo->addItems(values);
        s.combo->setCurrentIndex(qMax(0, s.combo->findText(cur)));
        s.combo->blockSignals(old);
    }
}

void GEvenements::updateCards()
{
    int aVenir = 0, programme = 0, planifie = 0;
    for (const Evenement &e : m_events) {
        if (e.statut == "À venir") ++aVenir;
        else if (e.statut == "Programmé") ++programme;
        else if (e.statut == "Planifié") ++planifie;
    }
    ui->lblTotalValue->setText(QString::number(m_events.size()));
    ui->lblAVenirValue->setText(QString::number(aVenir));
    ui->lblProgrammeValue->setText(QString::number(programme));
    ui->lblPlanifieValue->setText(QString::number(planifie));
}

void GEvenements::updateBars()
{
    // --- événements par type ---
    QHash<QString, int> byType;
    int maxT = 1;
    for (const Evenement &e : m_events)
        maxT = std::max(maxT, ++byType[e.type]);
    for (int i = 0; i < kTypes.size(); ++i) {
        auto *bar = findChild<QProgressBar *>(QString("barT%1").arg(i));
        auto *val = findChild<QLabel *>(QString("lblValT%1").arg(i));
        const int n = byType.value(kTypes[i], 0);
        if (bar) { bar->setMaximum(maxT + 1); bar->setValue(n); }
        if (val) val->setText(QString::number(n));
    }

    // --- évolution par mois (année affichée dans le calendrier) ---
    const int year = ui->calendar->yearShown();
    ui->lblEvolTitle->setText(QString("📈  Évolution des événements (%1)").arg(year));
    QVector<int> months(12, 0);
    for (const Evenement &e : m_events) {
        const QDate d = QDate::fromString(e.date, kDateFmt);
        if (d.isValid() && d.year() == year)
            ++months[d.month() - 1];
    }
    ui->chartEvolHost->setData(months, { "Jan", "Fév", "Mar", "Avr", "Mai", "Juin",
                                         "Juil", "Août", "Sep", "Oct", "Nov", "Déc" });
}

void GEvenements::updateLieux()
{
    QStringList order;
    QHash<QString, int> n;
    for (const Evenement &e : m_events) {
        if (e.lieu.isEmpty()) continue;
        if (!n.contains(e.lieu)) order << e.lieu;
        ++n[e.lieu];
    }
    std::stable_sort(order.begin(), order.end(), [&](const QString &a, const QString &b) { return n[a] > n[b]; });
    ui->listLieux->clear();
    for (const QString &l : order)
        ui->listLieux->addItem(QString("📍  %1   ·   %2 événement%3").arg(l).arg(n[l]).arg(n[l] > 1 ? "s" : ""));
}

QDate GEvenements::initialCalendarDate() const
{
    QList<QDate> dates;
    for (const Evenement &e : m_events) {
        const QDate d = QDate::fromString(e.date, kDateFmt);
        if (d.isValid()) dates << d;
    }
    if (dates.isEmpty()) return QDate::currentDate();
    std::sort(dates.begin(), dates.end());
    for (const QDate &d : std::as_const(dates))
        if (d >= QDate::currentDate()) return d;
    return dates.last();
}

void GEvenements::updateCalendarMarks()
{
    ui->calendar->setDateTextFormat(QDate(), QTextCharFormat());   // efface tout
    QTextCharFormat fmt;
    fmt.setBackground(QColor("#dce9f7"));
    fmt.setForeground(QColor("#17375e"));
    fmt.setFontWeight(QFont::Bold);
    for (const Evenement &e : m_events) {
        const QDate d = QDate::fromString(e.date, kDateFmt);
        if (d.isValid())
            ui->calendar->setDateTextFormat(d, fmt);
    }
}

// ======================================================================
//  Fiche événement
// ======================================================================
int GEvenements::selectedId() const
{
    const int row = ui->tableEvents->currentRow();
    if (row >= 0 && row < m_visible.size() && !ui->tableEvents->selectedItems().isEmpty())
        return m_visible[row].id;
    return -1;
}

void GEvenements::clearForm()
{
    m_currentId = -1;
    m_currentImage.clear();
    ui->lineNom->clear();
    ui->comboTypeForm->setCurrentIndex(0);
    ui->comboStatutForm->setCurrentIndex(0);
    ui->dateEdit->setDate(QDate::currentDate());
    ui->timeEdit->setTime(QTime(18, 0));
    ui->lineLieu->clear();
    ui->lineClient->clear();
    ui->textDescription->clear();
    setPreview(QString());
    ui->lblFicheTitle->setText("📅  Fiche événement");
}

void GEvenements::fillForm(const Evenement &e)
{
    ui->lineNom->setText(e.nom);
    ui->comboTypeForm->setCurrentText(e.type);
    ui->comboStatutForm->setCurrentText(e.statut);
    const QDate d = QDate::fromString(e.date, kDateFmt);
    ui->dateEdit->setDate(d.isValid() ? d : QDate::currentDate());
    const QTime t = QTime::fromString(e.heure, "HH:mm");
    ui->timeEdit->setTime(t.isValid() ? t : QTime(18, 0));
    ui->lineLieu->setText(e.lieu);
    ui->lineClient->setText(e.client);
    ui->textDescription->setPlainText(e.description);
    m_currentImage = e.image;
    setPreview(m_currentImage);
    ui->lblFicheTitle->setText(QString("📅  Fiche événement  #%1").arg(e.id));
    if (d.isValid())
        ui->calendar->setSelectedDate(d);
}

Evenement GEvenements::formData()
{
    Evenement e;
    e.nom = ui->lineNom->text().trimmed();
    e.type = ui->comboTypeForm->currentText();
    e.statut = ui->comboStatutForm->currentText();
    e.date = ui->dateEdit->date().toString(kDateFmt);
    e.heure = ui->timeEdit->time().toString("HH:mm");
    e.lieu = ui->lineLieu->text().trimmed();
    e.client = ui->lineClient->text().trimmed();
    e.description = ui->textDescription->toPlainText().trimmed();
    e.image = storeImage(m_currentImage);
    return e;
}

void GEvenements::setPreview(const QString &path)
{
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        ui->lblImagePreview->setPixmap(
            QPixmap(path).scaled(56, 56, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        ui->btnChoisirImage->setText(QFileInfo(path).fileName());
    } else {
        ui->lblImagePreview->setPixmap(QPixmap());
        ui->lblImagePreview->setText("🖼");
        ui->btnChoisirImage->setText("Choisir une image…");
    }
}

QString GEvenements::storeImage(const QString &path) const
{
    // copie l'affiche dans le dossier de données pour que le chemin reste valide
    if (path.isEmpty() || !QFileInfo::exists(path))
        return QString();
    const QString dir = dataDir() + "/images";
    QDir().mkpath(dir);
    const QString dest = dir + "/" + QFileInfo(path).fileName();
    if (QFileInfo(path).absoluteFilePath() != QFileInfo(dest).absoluteFilePath()) {
        QFile::remove(dest);
        QFile::copy(path, dest);
    }
    return dest;
}

// ======================================================================
//  Slots : boutons et widgets
// ======================================================================
void GEvenements::onSelectionChanged()
{
    const int id = selectedId();
    if (id < 0) return;
    if (const Evenement *e = findById(id)) {
        m_currentId = id;
        fillForm(*e);
    }
}

void GEvenements::onCalendarClicked(const QDate &date)
{
    // (le .ui met déjà à jour le champ Date) – sélectionne l'événement de ce jour s'il existe
    const QString iso = date.toString(kDateFmt);
    for (int r = 0; r < m_visible.size(); ++r) {
        if (m_visible[r].date == iso) {
            ui->tableEvents->selectRow(r);
            return;
        }
    }
}

void GEvenements::onCalendarPageChanged(int, int)
{
    updateBars();
}

void GEvenements::onAjouter()
{
    ui->tableEvents->clearSelection();
    clearForm();
    ui->lblFicheTitle->setText("📅  Nouvel événement");
    ui->lineNom->setFocus();
}

void GEvenements::onAnnuler()
{
    // le .ui vide d'abord les champs ; on restaure ensuite l'événement courant
    if (const Evenement *e = findById(m_currentId))
        fillForm(*e);
    else
        clearForm();
}

void GEvenements::onReinitialiser()
{
    for (QComboBox *c : { ui->comboStatut, ui->comboType, ui->comboLieu, ui->comboClient, ui->comboTri }) {
        const bool old = c->blockSignals(true);
        c->setCurrentIndex(0);
        c->blockSignals(old);
    }
    ui->lineSearch->clear();
    fillTable(m_currentId);
}

void GEvenements::modifierEvenement()
{
    if (selectedId() < 0 && m_currentId < 0) {
        QMessageBox::information(this, "Modifier", "Sélectionnez d'abord un événement dans le tableau.");
        return;
    }
    ui->lblFicheTitle->setText(QString("✏️  Modification  #%1").arg(m_currentId));
}

void GEvenements::supprimerEvenement()
{
    const int id = selectedId();
    const Evenement *e = findById(id);
    if (!e) {
        QMessageBox::information(this, "Supprimer", "Sélectionnez d'abord un événement dans le tableau.");
        return;
    }
    QMessageBox box(QMessageBox::Question, "Supprimer",
                    QString("Voulez-vous vraiment supprimer « %1 » ?").arg(e->nom),
                    QMessageBox::Yes | QMessageBox::No, this);
    box.setButtonText(QMessageBox::Yes, "Oui");
    box.setButtonText(QMessageBox::No, "Non");
    if (box.exec() != QMessageBox::Yes)
        return;

    for (int i = 0; i < m_events.size(); ++i) {
        if (m_events[i].id == id) { m_events.removeAt(i); break; }
    }
    saveEvents();
    clearForm();
    refreshAll(-1);
}

void GEvenements::enregistrerEvenement()
{
    Evenement e = formData();
    if (e.nom.isEmpty()) {
        QMessageBox::warning(this, "Champ obligatoire", "Le nom de l'événement est obligatoire.");
        ui->lineNom->setFocus();
        return;
    }
    if (e.lieu.isEmpty()) {
        QMessageBox::warning(this, "Champ obligatoire", "Le lieu est obligatoire.");
        ui->lineLieu->setFocus();
        return;
    }
    if (m_currentId < 0) {
        e.id = nextId();
        m_events.push_back(e);
    } else {
        e.id = m_currentId;
        for (Evenement &old : m_events)
            if (old.id == m_currentId) { old = e; break; }
    }
    m_currentId = e.id;
    saveEvents();
    refreshAll(e.id);
    ui->lblFicheTitle->setText(QString("📅  Fiche événement  #%1").arg(e.id));
    QMessageBox::information(this, "Enregistré", "L'événement a été enregistré avec succès.");
}

void GEvenements::choisirImage()
{
    const QString path = QFileDialog::getOpenFileName(this, "Choisir une image", QString(),
                                                      "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (path.isEmpty()) return;
    m_currentImage = path;
    setPreview(path);
}

void GEvenements::rechercher(const QString &)   { fillTable(m_currentId); }
void GEvenements::trierEvenements(int)          { fillTable(m_currentId); }
void GEvenements::filtrerEvenements(int)        { fillTable(m_currentId); }

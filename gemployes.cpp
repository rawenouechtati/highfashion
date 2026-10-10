#include "gemployes.h"
#include "ui_gemployes.h"
#include <QHeaderView>
#include <QBarSet>
#include <QBarSeries>
#include <QChart>
#include <QBarCategoryAxis>
#include <QValueAxis>

GEmployes::GEmployes(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GEmployes)
{
    ui->setupUi(this);
    connect(ui->navServices, &QPushButton::clicked, this, [this]() {
        if (!fenetreServices) fenetreServices = new GServices();
        fenetreServices->showMaximized();
        fenetreServices->raise();
    });

    connect(ui->navClients, &QPushButton::clicked, this, [this]() {
        if (!fenetreClients) fenetreClients = new GClients();
        fenetreClients->showMaximized();
        fenetreClients->raise();
    });

    ui->tableEmployes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QBarSet *set = new QBarSet("Services traités");
    *set << 9 << 5 << 11 << 14 << 4 << 7 << 8;

    QBarSeries *series = new QBarSeries();
    series->append(set);

    set->setColor(QColor("#1F3A4D"));
    set->setBorderColor(QColor("#1F3A4D"));

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Services traités par employé");
    chart->legend()->hide();

    QStringList employes = {"S. Guesmi", "N. Chebbi", "K. Fekih", "M. Ayari", "Y. Karray", "S. Trabelsi", "A. Sassi"};
    QBarCategoryAxis *axeX = new QBarCategoryAxis();
    axeX->append(employes);
    chart->addAxis(axeX, Qt::AlignBottom);
    series->attachAxis(axeX);

    QValueAxis *axeY = new QValueAxis();
    chart->addAxis(axeY, Qt::AlignLeft);
    series->attachAxis(axeY);

    ui->widgetGraphique->setChart(chart);
}

GEmployes::~GEmployes()
{
    delete ui;
}

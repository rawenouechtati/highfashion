#include <QChart>
#include <QChartView>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QVBoxLayout>

void MainWindow::creerDiagramme()
{
    // Données
    QStringList categories = {"0-99", "100-199", "200-299", "300-499", "500+"};
    QList<int> valeurs = {14, 18, 22, 9, 6};

    QColor bleu("#1F3A52");
    QColor rouge("#8B1E2D");

    // Barres
    auto *set = new QBarSet("Clients");
    for (int v : valeurs) *set << v;
    set->setColor(bleu);
    set->setBorderColor(bleu);
    set->setLabelColor(bleu);

    // Barre la plus haute en rouge
    int idxMax = std::distance(valeurs.begin(),
                               std::max_element(valeurs.begin(), valeurs.end()));
    set->selectBar(idxMax);
    set->setSelectedColor(rouge);

    auto *series = new QBarSeries;
    series->append(set);
    series->setBarWidth(0.6);
    series->setLabelsVisible(true);                                   // chiffres au-dessus
    series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

    // Graphique
    auto *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle("Répartition par points");
    chart->legend()->hide();
    chart->setBackgroundBrush(QColor("#FBF8F1"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // Axe X (catégories)
    auto *axeX = new QBarCategoryAxis;
    axeX->append(categories);
    chart->addAxis(axeX, Qt::AlignBottom);
    series->attachAxis(axeX);

    // Axe Y (0 à 24, graduations de 6 en 6)
    auto *axeY = new QValueAxis;
    axeY->setRange(0, 24);
    axeY->setTickCount(5);
    axeY->setLabelFormat("%d");
    axeY->setGridLineColor(QColor("#EDE6D3"));
    chart->addAxis(axeY, Qt::AlignLeft);
    series->attachAxis(axeY);

    // Affichage dans le widget
    auto *vue = new QChartView(chart);
    vue->setRenderHint(QPainter::Antialiasing);

    auto *layout = new QVBoxLayout(ui->widgetChart);
    layout->setContentsMargins(0, 0, 0, 0);
    v
    layout->addWidget(vue);
}s
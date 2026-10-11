#ifndef EVOLUTIONCHART_H
#define EVOLUTIONCHART_H

#include <QStringList>
#include <QVector>
#include <QWidget>

// Courbe lissée avec aire dégradée (style « Évolution des événements »).
// Dans Qt Designer : placer un QWidget, clic droit ▸ « Promouvoir en… »
// classe = EvolutionChart, fichier d'en-tête = evolutionchart.h
class EvolutionChart : public QWidget
{
    Q_OBJECT
public:
    explicit EvolutionChart(QWidget *parent = nullptr);

    void setData(const QVector<int> &values, const QStringList &labels);
    void setLineColor(const QColor &c) { m_color = c; update(); }

    QSize minimumSizeHint() const override { return { 200, 140 }; }
    QSize sizeHint() const override { return { 360, 200 }; }

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *) override;

private:
    QRectF plotRect() const;
    QPointF pointAt(int i, int top) const;

    QVector<int> m_values;
    QStringList  m_labels;
    QColor       m_color{ "#9b2335" };
    int          m_hover = -1;
};

#endif // EVOLUTIONCHART_H

#include "evolutionchart.h"

#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace {
// échelle verticale : pas entier, ≤ 5 graduations, avec un peu de marge en haut
void niceScale(int maxv, int &step, int &top)
{
    maxv = std::max(maxv, 1);
    step = std::max(1, int(std::ceil(maxv / 5.0)));
    top = int(std::ceil(double(maxv) / step)) * step;
    if (top <= maxv)
        top += step;
}
} // namespace

EvolutionChart::EvolutionChart(QWidget *parent)
    : QWidget(parent),
      m_values(12, 0),
      m_labels({ "Jan", "Fév", "Mar", "Avr", "Mai", "Juin", "Juil", "Août", "Sep", "Oct", "Nov", "Déc" })
{
    setMouseTracking(true);
}

void EvolutionChart::setData(const QVector<int> &values, const QStringList &labels)
{
    m_values = values;
    m_labels = labels;
    m_hover = -1;
    update();
}

QRectF EvolutionChart::plotRect() const
{
    return QRectF(30, 12, width() - 30 - 14, height() - 12 - 24);
}

QPointF EvolutionChart::pointAt(int i, int top) const
{
    const QRectF r = plotRect();
    const int n = m_values.size();
    const qreal x = r.left() + r.width() * i / std::max(1, n - 1);
    const qreal y = r.bottom() - r.height() * m_values[i] / top;
    return { x, y };
}

void EvolutionChart::paintEvent(QPaintEvent *)
{
    const int n = m_values.size();
    if (n < 2)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QFont f = font();
    f.setPixelSize(10);
    p.setFont(f);

    int step, top;
    niceScale(*std::max_element(m_values.begin(), m_values.end()), step, top);
    const QRectF r = plotRect();
    const QColor textColor("#6b7785");

    // --- grille + graduations (axe Y) ---
    for (int v = 0; v <= top; v += step) {
        const qreal y = r.bottom() - r.height() * v / top;
        p.setPen(QPen(QColor("#e8e5df"), 1));
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
        p.setPen(textColor);
        p.drawText(QRectF(0, y - 8, r.left() - 6, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(v));
    }

    // --- points + courbe lissée (sans dépassement sous zéro) ---
    QVector<QPointF> pts;
    for (int i = 0; i < n; ++i)
        pts << pointAt(i, top);

    // Interpolation cubique monotone (Fritsch–Carlson) : arrondie, sans dépassement
    QVector<qreal> m(n, 0.0);                          // pentes (en pixels)
    QVector<qreal> d(n - 1);                           // pentes sécantes
    for (int i = 0; i < n - 1; ++i)
        d[i] = (pts[i + 1].y() - pts[i].y()) / (pts[i + 1].x() - pts[i].x());
    m[0] = d[0];
    m[n - 1] = d[n - 2];
    for (int i = 1; i < n - 1; ++i)
        m[i] = (d[i - 1] * d[i] <= 0) ? 0.0 : 2.0 * d[i - 1] * d[i] / (d[i - 1] + d[i]);

    QPainterPath line(pts.first());
    for (int i = 0; i < n - 1; ++i) {
        const qreal h = (pts[i + 1].x() - pts[i].x()) / 3.0;
        line.cubicTo(pts[i].x() + h, pts[i].y() + m[i] * h,
                     pts[i + 1].x() - h, pts[i + 1].y() - m[i + 1] * h,
                     pts[i + 1].x(), pts[i + 1].y());
    }

    // --- aire dégradée sous la courbe ---
    QPainterPath area = line;
    area.lineTo(pts.last().x(), r.bottom());
    area.lineTo(pts.first().x(), r.bottom());
    area.closeSubpath();
    QLinearGradient grad(0, r.top(), 0, r.bottom());
    QColor c1 = m_color, c0 = m_color;
    c1.setAlpha(120);
    c0.setAlpha(0);
    grad.setColorAt(0.0, c1);
    grad.setColorAt(1.0, c0);
    p.fillPath(area, grad);

    p.setPen(QPen(m_color, 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(line);

    // --- libellés des mois (axe X) ---
    p.setPen(textColor);
    for (int i = 0; i < n && i < m_labels.size(); ++i)
        p.drawText(QRectF(pts[i].x() - 20, r.bottom() + 6, 40, 14), Qt::AlignHCenter | Qt::AlignTop, m_labels[i]);

    // --- repère de survol ---
    if (m_hover >= 0 && m_hover < n) {
        p.setPen(QPen(QColor(m_color.red(), m_color.green(), m_color.blue(), 90), 1, Qt::DashLine));
        p.drawLine(QPointF(pts[m_hover].x(), r.top()), QPointF(pts[m_hover].x(), r.bottom()));
    }

    // --- marqueurs ---
    for (int i = 0; i < n; ++i) {
        const bool hot = (i == m_hover);
        p.setPen(QPen(Qt::white, 1.5));
        p.setBrush(m_color);
        p.drawEllipse(pts[i], hot ? 5.5 : 3.6, hot ? 5.5 : 3.6);
    }

    // --- info-bulle ---
    if (m_hover >= 0 && m_hover < n) {
        const int v = m_values[m_hover];
        const QString txt = QString("%1 : %2 événement%3").arg(m_labels.value(m_hover)).arg(v).arg(v > 1 ? "s" : "");
        const QFontMetrics fm(p.font());
        QRectF tip(0, 0, fm.horizontalAdvance(txt) + 16, fm.height() + 8);
        tip.moveCenter(QPointF(pts[m_hover].x(), pts[m_hover].y() - 22));
        if (tip.top() < 0) tip.moveTop(pts[m_hover].y() + 10);
        if (tip.left() < 2) tip.moveLeft(2);
        if (tip.right() > width() - 2) tip.moveRight(width() - 2);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#17375e"));
        p.drawRoundedRect(tip, 6, 6);
        p.setPen(Qt::white);
        p.drawText(tip, Qt::AlignCenter, txt);
    }
}

void EvolutionChart::mouseMoveEvent(QMouseEvent *e)
{
    const int n = m_values.size();
    if (n < 2)
        return;
    const QRectF r = plotRect();
    const qreal t = (e->pos().x() - r.left()) / r.width() * (n - 1);
    const int idx = std::clamp(int(std::lround(t)), 0, n - 1);
    if (idx != m_hover) {
        m_hover = idx;
        update();
    }
}

void EvolutionChart::leaveEvent(QEvent *)
{
    m_hover = -1;
    update();
}

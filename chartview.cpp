#include "chartview.h"

#include <QtCore/QtMath>
#include <QtGui/QMouseEvent>
#include <QDir>
#include <QTextStream>

using CWorker = ChartView::DataWorker;

ChartView::ChartView(QWidget* parent):
    QChartView(new QChart(), parent)
{
    setRenderHint(QPainter::Antialiasing);
    setRubberBand(QChartView::RectangleRubberBand);

    chart()->setTitle("Centralities relation");

    pointedPoint = new QScatterSeries();
    pointedPoint->setName("Current point");
    pointedPoint->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    chart()->addSeries(pointedPoint);

    centralityScatter = new QScatterSeries();
    centralityScatter->setName("The relation between centralities");
    regressionLine = new QLineSeries();
    regressionLine->setName("Linear regression line");

    QObject::connect(centralityScatter, SIGNAL(hovered(QPointF, bool)),
                     this, SLOT(handleHoveredPoint(QPointF, bool)));
    QObject::connect(pointedPoint, SIGNAL(hovered(QPointF, bool)),
                     this, SLOT(unhandleHoveredPoint(QPointF, bool)));
    QObject::connect(this, SIGNAL(windowLoaded()), this, SLOT(setMarkerSize()));
}

ChartView::~ChartView() {
    delete pointedPoint;
    delete regressionLine;
    delete centralityScatter;
}

void ChartView::setChartItems() {
    auto data = worker.getData();
    foreach (QPointF point, *data) {
        *centralityScatter << point;
    }

    chart()->addSeries(centralityScatter);

    auto b = worker.calculateLinearRegression();
    this->setLinearRegressionLine(b);

    chart()->addSeries(regressionLine);
    chart()->createDefaultAxes();

    chart()->axisX()->setRange(0, 0.65);
    chart()->axisY()->setRange(0, 0.025);

    chart()->axisX()->setTitleText("Betweenness centrality");
    chart()->axisY()->setTitleText("Closeness centrality");
}

void ChartView::setLinearRegressionLine(COEF_TYPE b) {
    qreal x0 = 0;
    qreal x1 = 1;
    qreal y0 = std::get<0>(b)+std::get<1>(b)*x0;
    qreal y1 = std::get<0>(b)+std::get<1>(b)*x1;
    regressionLine->append(x0, y0);
    regressionLine->append(x1, y1);
}

void ChartView::setMarkerSize() {
    qint16 w = QChartView::size().width();
    qint16 h = QChartView::size().height();
    qint16 N = worker.getPointsCount();
    qreal canvasArea = w*h;
    this->markerSizeHint = qSqrt(2*qreal((canvasArea/(N*M_PI))));

    centralityScatter->setMarkerSize(markerSizeHint);
    pointedPoint->setMarkerSize(markerSizeHint*4);
}

void ChartView::unhandleHoveredPoint(const QPointF&, bool state) {
    if (state == false) {
        pointedPoint->clear();
    }
}

void ChartView::handleHoveredPoint(const QPointF &hoveredPoint, bool state){
    // Находим ближайшую точку из centralityScatter
    if (state == true) {
        QPointF closest(INT_MAX, INT_MAX);
        qreal distance(INT_MAX);

        foreach (QPointF currentPoint, centralityScatter->points()) {
            // Евклидово расстояние

            qreal currentDistance = qSqrt((currentPoint.x() - hoveredPoint.x())
                                          * (currentPoint.x() - hoveredPoint.x())
                                          + (currentPoint.y() - hoveredPoint.y())
                                          * (currentPoint.y() - hoveredPoint.y()));
            if (currentDistance < distance) {
                distance = currentDistance;
                closest = currentPoint;
            }
        }

        // Увеличиваем размер выбранной точки
        pointedPoint->clear();
        pointedPoint->append(closest);
        setToolTip(QString("<h3>ID: %1<br>%2 %3</h3>")
                    .arg(worker.getIdOf(closest))
                    .arg(closest.x())
                    .arg(closest.y()));
    }
}

void ChartView::resizeEvent(QResizeEvent *event) {
    setRenderHint(QPainter::HighQualityAntialiasing);
    QChartView::resizeEvent(event);
    setRenderHint(QPainter::Antialiasing);
    emit windowLoaded();
}

void ChartView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        QPoint angle = event->angleDelta();
        if (angle.y() > 0) {
            chart()->zoom(1.1);
        } else {
            chart()->zoom(0.9);
        }
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void ChartView::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Plus:
        chart()->zoomIn();
        break;
    case Qt::Key_Minus:
        chart()->zoomOut();
        break;
    case Qt::Key_Up:
        chart()->scroll(0, 10);
        break;
    case Qt::Key_Down:
        chart()->scroll(0, -10);
        break;
    case Qt::Key_Left:
        chart()->scroll(-10, 0);
        break;
    case Qt::Key_Right:
        chart()->scroll(10, 0);
        break;
    case Qt::Key_Escape:
        chart()->zoomReset();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}

bool ChartView::viewportEvent(QEvent *event) {
    if (event->type() == QEvent::TouchBegin) {
        m_isTouching = true;
        chart()->setAnimationOptions(QChart::NoAnimation);
    }
    return QChartView::viewportEvent(event);
}

void ChartView::mousePressEvent(QMouseEvent *event) {
    if (m_isTouching) {
        return;
    }

    QChartView::mousePressEvent(event);
}

void ChartView::mouseMoveEvent(QMouseEvent *event) {
    if (m_isTouching) {
        return;
    }

    QChartView::mouseMoveEvent(event);
}

void ChartView::mouseReleaseEvent(QMouseEvent *event) {
    if (m_isTouching) {
        m_isTouching = false;
    }

    chart()->setAnimationOptions(QChart::SeriesAnimations);
    QChartView::mouseReleaseEvent(event);
}

bool CWorker::readFile(QString filename) {
   QFile csvfile(QDir::currentPath() + "/job_test/" + filename);
   if (!csvfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
       return false;
   }

   dataMap = new QMap <qint16, QPointF>();
   QTextStream in(&csvfile);
   for (int i = 0; !csvfile.atEnd(); ++i) {
       QString raw_line = in.readLine();
       // not reading header info
       if (i != 0) {
           QStringList line = raw_line.split(',', QString::SkipEmptyParts);
           int id = line[0].toInt();
           qreal x = line[2].toFloat(); // betweenness
           qreal y = line[1].toFloat(); // closeness
           dataMap->insert(id, QPointF(x, y));
       }
   }

   return true;
}

ChartView::COEF_TYPE CWorker::calculateLinearRegression() {
    qreal xmean = 0, ymean = 0;
    qint16 N = dataMap->count();
    foreach(QPointF P, *dataMap) {
        xmean += P.x();
        ymean += P.y();
    }
    xmean /= N;
    ymean /= N;

    qreal numerator = 0, denominator = 0;
    foreach (QPointF P, *dataMap) {
        numerator += (P.x() - xmean)*(P.y() - ymean);
        denominator += (P.x() - xmean)*(P.x() - xmean);
    }

    qreal b1 = numerator/denominator;
    qreal b0 = ymean - b1*xmean;

    return COEF_TYPE(b0, b1);
}

CWorker::ID_TYPE CWorker::getIdOf(QPointF point) {
    return dataMap->key(point);
}

CWorker::~DataWorker() {
    delete dataMap;
}

CWorker::DATA_TYPE* CWorker::getData() {
    return dataMap;
}

qint16 CWorker::getPointsCount() {
    return dataMap->count();
}

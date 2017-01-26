#ifndef CHARTVIEW_H
#define CHARTVIEW_H

#include <QtGlobal>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QToolTip>

QT_CHARTS_USE_NAMESPACE

class ChartView : public QChartView
{
    Q_OBJECT
    typedef std::tuple <qreal, qreal>   COEF_TYPE;
public:
    explicit ChartView(QWidget* parent = nullptr);
    ~ChartView() override;
    void setChartItems();

    class DataWorker {
        typedef QMap <qint16, QPointF>      DATA_TYPE;
        typedef qint16                      ID_TYPE;
    public:
        DataWorker() = default;
        ~DataWorker();
        bool readFile(QString fileName);
        ID_TYPE getIdOf(QPointF point);
        DATA_TYPE* getData();
        qint16 getPointsCount();
        COEF_TYPE calculateLinearRegression();
    private:
        DATA_TYPE* dataMap;
    } worker;

signals:
    void windowLoaded();
private slots:
    void unhandleHoveredPoint(const QPointF& point, bool);
    void handleHoveredPoint(const QPointF& point, bool state);
    void setMarkerSize();
private:
    void setLinearRegressionLine(COEF_TYPE);
    QScatterSeries* centralityScatter;
    QScatterSeries* pointedPoint;
    QLineSeries* regressionLine;
    qint16 markerSizeHint;

    bool m_isTouching {false};
protected:
    void keyPressEvent(QKeyEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    bool viewportEvent(QEvent *event);
    void wheelEvent(QWheelEvent* event);
    void resizeEvent(QResizeEvent* event);
};

#endif // CHARTVIEW_H

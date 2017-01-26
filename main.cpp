#include "chartview.h"
#include <QMainWindow>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMainWindow window;
    ChartView chartView(&window);
    chartView.worker.readFile("centralities.csv");

    window.setCentralWidget(&chartView);
    window.show();
    window.resize(800, 800);

    chartView.setChartItems();

    return a.exec();
}

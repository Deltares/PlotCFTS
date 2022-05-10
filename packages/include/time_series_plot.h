#ifndef TIME_SERIES_PLOT_H
#define TIME_SERIES_PLOT_H

#include <QtGui/QScreen>
#include <QtWidgets/QWidget>
#include "qcustomplot.h"
#include "cf_time_series.h"


class TSPlot : public QMainWindow
{
    Q_OBJECT

private slots:
    void axisLabelDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part);
    void legendDoubleClick(QCPLegend* legend, QCPAbstractLegendItem* item);
    void titleDoubleClick(QMouseEvent*);
    void selectionChanged();
    void mousePress();
    void mouseWheel();
    void contextMenuRequest(QPoint pos);
    void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);
    void graphDoubleClicked(QCPAbstractPlottable *plottable, int dataIndex);
    void moveLegend();
    void contextMenuRescaleAxes();
    void contextMenuSaveAsPDF();
    void contextMenuRemoveSelectedGraph();

    void onMouseMove(QMouseEvent *);

public:
    TSPlot(QWidget *, QIcon, int);
    ~TSPlot();
    void add_to_plot(TSFILE *, QListWidget *, QListWidget *, QListWidget *, QComboBox *, QSpinBox *, QPushButton *);
    void add_data_to_plot();
    int get_active_plot_nr();
    void draw_plot(int, QString, QString);
    void draw_data(QVector<qreal>, QVector<qreal>);
    QWidget * get_parent();

private:
    void TimeSeriesGraph(int, int, int, int);
    QString set_yaxis_label(QString, QString, QString);
    QString get_yaxis_label(QString, QString);

    TSFILE * tsfile;
    QCustomPlot * customPlot;
    QIcon icon;
    int current_plot;
    int has_focus;
    QWidget * parent;
    QListWidget * lb_parameters;
    QListWidget * lb_locations;
    QListWidget * lb_times;
    QComboBox * cb_parloc;
    QSpinBox * sb_layer;
    QPushButton * pb_add_to_plot;
    QString tmp_label;
    QString m_yaxis_label;
    long _nr_y_axis_labels;
    long _nr_graphs;
    long _nr_max_ylabel;
    int plot_width;
    int plot_height;

    QMap<QString, int> _ylabel_dic;
};

#endif

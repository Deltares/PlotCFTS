#include "time_series_plot.h"
using namespace std;


TSPlot::TSPlot(QWidget * qparent, QIcon qicon, int qnr_plot)
{
    tsfile = nullptr;
    customPlot = new QCustomPlot(qparent);
    customPlot->setAttribute(Qt::WA_DeleteOnClose, true);
    icon = qicon;
    current_plot = qnr_plot;
    has_focus = -1;
    parent = qparent;

    // Center and resize application window; suppose that the taskbar
    // is at the bottom of the screen
    //
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    plot_width = screenGeometry.width();
    plot_height = screenGeometry.height();

    plot_width = (int)(0.495 * double(plot_width));
    plot_height = (int)(0.495 * double(plot_height)); // Do not count the taskbar pixels, assumed to be 1 percent of the screen
    if (plot_width > 2 * plot_height) 
    { 
        plot_width = 2 * plot_height;
    }
    else
    {
        plot_height = int(0.5 * double(plot_width));
    }

    QVBoxLayout * vl_main = new QVBoxLayout;
    vl_main->setSpacing(0);
    vl_main->setMargin(0);
    vl_main->addWidget(customPlot);

    parent->setLayout(vl_main);

    parent->resize(plot_width, plot_height);
    parent->show();

    qparent->setWindowIcon(icon);
    _nr_y_axis_labels = 0;
    _nr_graphs = 0;
    _nr_max_ylabel = 0;
}
TSPlot::~TSPlot()
{
    parent->deleteLater();
    delete customPlot;
}
void TSPlot::add_to_plot(TSFILE * qtsfile, QListWidget * qlb_parameters, QListWidget * qlb_locations, QListWidget * qlb_times, QComboBox * qcb_parloc, QSpinBox * qsb_layer, QPushButton * qpb_add_to_plot)
{
    tsfile = qtsfile;
    lb_parameters = qlb_parameters;
    lb_locations = qlb_locations;
    lb_times = qlb_times;
    cb_parloc = qcb_parloc;
    sb_layer = qsb_layer;
    pb_add_to_plot = qpb_add_to_plot;
}
void TSPlot::add_data_to_plot()
{
    int cb_index = tsfile->get_cb_parloc_index();
    int i_layer = sb_layer->value() - 1;

    _nr_max_ylabel = _ylabel_dic.size();
    for (int i_par = 0; i_par < lb_parameters->count(); i_par++)
    {
        if (lb_parameters->item(i_par)->isSelected())
        {
            for (int i_loc = 0; i_loc < lb_locations->count(); i_loc++)
            {
                if (lb_locations->item(i_loc)->isSelected())
                {
                    TimeSeriesGraph(cb_index, i_par, i_loc, i_layer);
                }
            }
        }
    }
    customPlot->rescaleAxes();
    customPlot->replot();
}
void TSPlot::draw_plot(int nr_plot, QString txt, QString fname)
{
    parent->setWindowTitle(QString("Plot %1: %2").arg(nr_plot + 1).arg(txt));
    customPlot->resize(this->plot_width, this->plot_height);
    customPlot->show();

    customPlot->plotLayout()->insertRow(0);
    QCPTextElement *title = new QCPTextElement(customPlot, "File: " + fname, QFont("sans", 9, QFont::Bold));
    customPlot->plotLayout()->addElement(0, 0, title);

    int cb_index = tsfile->get_cb_parloc_index();
    int i_layer = sb_layer->value() - 1;

    _nr_max_ylabel = 0;
    for (int i_par = 0; i_par < lb_parameters->count(); i_par++)
    {
        if (lb_parameters->item(i_par)->isSelected())
        {
            for (int i_loc = 0; i_loc < lb_locations->count(); i_loc++)
            {
                if (lb_locations->item(i_loc)->isSelected())
                {
                    TimeSeriesGraph(cb_index, i_par, i_loc, i_layer);
                }
            }
        }
    }
    customPlot->rescaleAxes();

    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes | QCP::iSelectLegend | QCP::iSelectPlottables);
    customPlot->axisRect()->setupFullAxesBox();

    customPlot->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(10);
    customPlot->legend->setFont(legendFont);
    customPlot->legend->setSelectedFont(legendFont);
    customPlot->legend->setSelectableParts(QCPLegend::spItems); // legend box shall not be selectable, only legend items

    // connect slot that ties some axis selections together (especially opposite axes):
    connect(customPlot, &QCustomPlot::selectionChangedByUser, this, &TSPlot::selectionChanged);
    // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
    connect(customPlot, &QCustomPlot::mousePress, this, &TSPlot::mousePress);
    connect(customPlot, &QCustomPlot::mouseWheel, this, &TSPlot::mouseWheel);

    // make bottom and left axes transfer their ranges to top and right axes:
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));

    // connect some interaction slots:
    connect(customPlot, &QCustomPlot::axisDoubleClick, this, &TSPlot::axisLabelDoubleClick);
    connect(customPlot, &QCustomPlot::legendDoubleClick, this, &TSPlot::legendDoubleClick);
    connect(title, &QCPTextElement::doubleClicked, this, &TSPlot::titleDoubleClick);

    // connect slot that shows a message in the status bar when a graph is clicked:
    connect(customPlot, &QCustomPlot::plottableClick, this, &TSPlot::graphClicked);
    connect(customPlot, &QCustomPlot::plottableDoubleClick, this, &TSPlot::graphDoubleClicked);

    // setup policy and connect slot for context menu popup:
    customPlot->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(customPlot, &QCustomPlot::customContextMenuRequested, this, &TSPlot::contextMenuRequest);

    connect(customPlot, &QCustomPlot::mouseMove, this, &TSPlot::onMouseMove);
    // connect(customPlot, SIGNAL(destroyed()), this, SLOT(destroyed()));
}
QWidget * TSPlot::get_parent()
{
    return this->parent;
}

int TSPlot::get_active_plot_nr()
{
    int selected_plot = -1;
    if (customPlot->focusWidget() != NULL)
    {
        selected_plot = this->current_plot;
    }
    return selected_plot;
}

void TSPlot::TimeSeriesGraph(int cb_index, int i_par, int i_loc, int i_layer)
{
    int nr_x_values = tsfile->get_count_times();
    _time struct_times = tsfile->get_times();
    vector<double> x_values = struct_times.time;
    int i_tsfile_par = -1;
    int i_tsfile_loc = -1;
    QListWidgetItem * sel_item;

    // i_par and i_loc taken from sorted listboxes
    QDateTime * RefDate = tsfile->get_reference_date();
    struct _parameter * param = tsfile->get_parameters(cb_index);
    int nr_parameters = tsfile->get_count_parameters(cb_index);
    struct _location * location = tsfile->get_locations(cb_index);
    int nr_locations = tsfile->get_count_locations(cb_index);

    sel_item = lb_parameters->item(i_par);
#if defined (DEBUG)
    QString janm = sel_item->text();
#endif
    for (long i = 0; i < nr_parameters; i++)
    {
        if (!strcmp(param[i].name, sel_item->text().toUtf8()))
        {
            i_tsfile_par = i;
            break;
        }
    }

    sel_item = lb_locations->item(i_loc);
#if defined (DEBUG)
    QString janm3 = sel_item->text();
#endif
    for (long i = 0; i < nr_locations; i++)
    {
        QString q_loc(*location[i].name); 
        QString s_name = sel_item->text();
        if (!q_loc.compare(s_name))
        {
            i_tsfile_loc = i;
            break;
        }
    }

    QString y_label_counter;
    QString yaxis_label = set_yaxis_label(param[i_tsfile_par].name, param[i_tsfile_par].unit, y_label_counter);
    customPlot->yAxis->setLabel(yaxis_label);

    vector<double> y_values = tsfile->get_time_series(cb_index, param[i_tsfile_par].name, i_tsfile_loc, i_layer);

    // x-axis
    QString xaxis_label = tsfile->get_xaxis_label();
    customPlot->xAxis->setLabel(xaxis_label);

    // configure bottom axis to show date instead of number:

    // Select part of time series, determine by the time interval selected in the listbox
    // if nothing is selected the whole series is used

    if (lb_times->currentRow() != -1)
    {
        int j = -1;
        for (long i = 0; i < lb_times->count(); i++)
        {
            QListWidgetItem * item = lb_times->item(i);
            if (item->isSelected())
            {
                j += 1;
                x_values[j] = x_values[i];
                y_values[j] = y_values[i];
                nr_x_values = j + 1;  // one-based
            }
        }
    }
    QSharedPointer<QCPAxisTickerDateTime> dateTimeTicker(new QCPAxisTickerDateTime);
    dateTimeTicker->setDateTimeFormat("hh:mm:ss\ndd MMM yyyy");
    dateTimeTicker->setDateTimeSpec(Qt::UTC);
    dateTimeTicker->setTickOrigin(RefDate->addSecs(x_values[0]));
    QString janm1 = RefDate->toString("yyyy-MM-dd hh:mm:ss.zzz");
    customPlot->xAxis->setTicker(dateTimeTicker);

    qint64 offset = RefDate->toSecsSinceEpoch();
    for (int i = 0; i < nr_x_values; i++)
    {
        x_values[i] = x_values[i] + (double) offset;
    }

    std::vector<double> xv(nr_x_values);
    for (int i = 0; i < nr_x_values; i++)
    {
        xv[i] = x_values[i];
    }
    QVector<qreal> x_val = QVector<qreal>::fromStdVector(xv);

    std::vector<double> yv(nr_x_values);
    for (int i = 0; i < nr_x_values; i++)
    {
        yv[i] = y_values[i];
    }
    QVector<qreal> y_val = QVector<qreal>::fromStdVector(yv);

    customPlot->addGraph();
    _nr_graphs = customPlot->graphCount();
    if (this->_nr_y_axis_labels == 1)
    {
        QString name = QString("%1").arg(*location[i_tsfile_loc].name).trimmed();
        customPlot->graph()->setName(name);
    }
    else
    {
        for (long i = 0; i < _nr_graphs; i++)
        {
            QCPGraph * grafiek = customPlot->graph(i);
            QCPPlottableLegendItem *plItem = customPlot->legend->itemWithPlottable(grafiek);
            QString new_name = plItem->plottable()->name();
            if (!new_name.startsWith("Y"))
            {
                new_name = QString("Y1: ") + new_name;
                plItem->plottable()->setName(new_name);
            }
        }
        // check if parameter name is already in y-axis label, find the y-axis label number 
        long y_label;
        QString parname = param[i_tsfile_par].name;
        QString dic_name;
        if (yaxis_label.contains(parname))
        {
            QStringList yaxis_list = yaxis_label.split('\n');
            long i = 0;  // yaxis_list.length();
            for (QStringList::Iterator it = yaxis_list.begin(); it != yaxis_list.end(); it++)
            {
                i += 1;
                QString name = QString(*it);
                if (QString(*it).contains(parname))
                {
                    y_label = i + _nr_max_ylabel;
                    y_label_counter = QString("Y%1: ").arg(y_label);
                    QStringList names = (*it).split(":");
                    if (names[0] != QString("Y%1").arg(y_label))
                    {
                        dic_name = QString("Y%1").arg(y_label);
                        y_label_counter = names[0] + QString(": ");
                    }
                }
            }
        }
        customPlot->graph()->setName(y_label_counter + QString("%1").arg(*location[i_tsfile_loc].name).trimmed());
    }
    QPen graphPen;
    // set colors
    if (fmod(_nr_graphs, 6) == 1) graphPen.setColor(QColor(0, 0, 255));
    if (fmod(_nr_graphs, 6) == 2) graphPen.setColor(QColor(255, 0, 0));
    if (fmod(_nr_graphs, 6) == 3) graphPen.setColor(QColor(0, 0, 0));
    if (fmod(_nr_graphs, 6) == 4) graphPen.setColor(QColor(255, 0, 255));
    if (fmod(_nr_graphs, 6) == 5) graphPen.setColor(QColor(0, 255, 0));
    if (fmod(_nr_graphs, 6) == 0) graphPen.setColor(QColor(0, 255,255)); // 255, 255, 0 == yellow
    //graphPen.setColor(QColor(rand() % 245 + 10, rand() % 245 + 10, rand() % 245 + 10));

    customPlot->graph()->setPen(graphPen);
    this->draw_data(x_val, y_val);


    //assert(!listVal.empty());
    //double xmin = *std::min_element(x_val.begin(), x_val.end());
    //double xmax = *std::max_element(x_val.begin(), x_val.end());
    //double ymin = *std::min_element(y_val.begin(), y_val.end());
    //double ymax = *std::max_element(y_val.begin(), y_val.end());

    //customPlot->xAxis->setRange(xmin, xmax);
    //customPlot->yAxis->setRange(ymin, ymax);
    x_val.clear();
    y_val.clear();
}
QString TSPlot::set_yaxis_label(QString parname, QString unit, QString y_label_counter)
{
    if (customPlot->graphCount() > 0)
    {
        QCPGraph * grafiek = customPlot->graph(0);
        QCPPlottableLegendItem *plItem = customPlot->legend->itemWithPlottable(grafiek);
        QString new_name = plItem->plottable()->name();
        if (!new_name.startsWith("Y") && _ylabel_dic.size()>1)
        {
            new_name = QString("Y1: ") + new_name;
            plItem->plottable()->setName(new_name);
            _ylabel_dic[QString("Y1")] = 1;
        }
    }
    // y-axis
    if (_nr_y_axis_labels == 0)
    {
        _nr_y_axis_labels += 1;
        yaxis_label = this->get_yaxis_label(parname, unit);
        _ylabel_dic[QString("Y1")] = 1;
    }
    else
    {
        if (yaxis_label.contains(parname))
        {
            // already in y_axis_label
            QStringList yaxis_list = yaxis_label.split('\n');
            long i = 0;  // yaxis_list.length();
            for (QStringList::Iterator it = yaxis_list.begin(); it != yaxis_list.end(); it++)
            {
                i += 1;
                QString name = QString(*it);
                if (QString(*it).contains(parname))
                {
                    long y_label = i + _nr_max_ylabel;
                    y_label_counter = QString("Y%1: ").arg(y_label);
                    QStringList names = (*it).split(":");
                    QString janm = names[0];
                    if (names.length() == 1)
                    {
                        _ylabel_dic[QString("Y%1").arg(y_label)] += 1;
                    }
                    else // if (names[0] != QString("Y%1").arg(y_label))
                    {
                        _ylabel_dic[names[0]] += 1;
                        y_label_counter = names[0];
                    }
                }
            }
        }
        else
        {
            // not in y_axis_label
            if (_nr_y_axis_labels == 1)
            {
                yaxis_label.insert(0, "Y1: ");
            }
            _nr_y_axis_labels = _ylabel_dic.size() + 1;
            QString dic_name = (QString("Y%1").arg(_nr_y_axis_labels));
            tmp_label.clear();
            tmp_label.append("\n");
            tmp_label.append(dic_name + QString(": ") + this->get_yaxis_label(parname, unit));
            _ylabel_dic[dic_name] = 1;
            yaxis_label.append(tmp_label);
        }
    }
    return yaxis_label;
}
QString TSPlot::get_yaxis_label(QString name, QString unit)
{
    return name.trimmed() + " [" + unit.trimmed() + "]";
}
void TSPlot::draw_data(QVector<qreal> x_val, QVector<qreal> y_val)
{
    customPlot->graph()->addData(x_val, y_val, true); // x_val is ascending sorted
}


void TSPlot::axisLabelDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part)
{
    // Set an axis label by double clicking on it
    if (part == QCPAxis::spAxisLabel) // only react when the actual axis label is clicked, not tick label or axis backbone
    {
        bool ok;
        QString newLabel = QInputDialog::getText(this->parent, "Axis label", "New axis label:", QLineEdit::Normal, axis->label(), &ok);
        if (ok)
        {
            axis->setLabel(newLabel);
            customPlot->replot();
        }
    }
}


void TSPlot::legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item)
{
    // Rename a graph by double clicking on its legend item
    Q_UNUSED(legend)
    if (item) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
    {
        QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
        bool ok;
        QString newName = QInputDialog::getText(this->parent, "Graph name", "New graph name:", QLineEdit::Normal, plItem->plottable()->name(), &ok);
        if (ok)
        {
            plItem->plottable()->setName(newName);
            customPlot->replot();
        }
    }
}

void TSPlot::titleDoubleClick(QMouseEvent * event)
{
    Q_UNUSED(event)
    if (QCPTextElement *title = qobject_cast<QCPTextElement*>(sender()))
    {
        // Set the plot title by double clicking on it
        bool ok;
        QString newTitle = QInputDialog::getText(this->parent, "Title name", "New plot title:", QLineEdit::Normal, title->text(), &ok);
        if (ok)
        {
            title->setText(newTitle);
            customPlot->replot();
        }
    }
}

void TSPlot::selectionChanged()
{
    /*
    normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
    the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
    and the axis base line together. However, the axis label shall be selectable individually.

    The selection state of the left and right axes shall be synchronized as well as the state of the
    bottom and top axes.

    Further, we want to synchronize the selection of the graphs with the selection state of the respective
    legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
    or on its legend item.
    */

    // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
    if (customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || customPlot->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
        customPlot->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || customPlot->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
    {
        customPlot->xAxis2->setSelectedParts(QCPAxis::spAxis | QCPAxis::spTickLabels);
        customPlot->xAxis->setSelectedParts(QCPAxis::spAxis | QCPAxis::spTickLabels);
    }
    // make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
    if (customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || customPlot->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
        customPlot->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || customPlot->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
    {
        customPlot->yAxis2->setSelectedParts(QCPAxis::spAxis | QCPAxis::spTickLabels);
        customPlot->yAxis->setSelectedParts(QCPAxis::spAxis | QCPAxis::spTickLabels);
    }

    // synchronize selection of graphs with selection of corresponding legend items:
    for (int i = 0; i<customPlot->graphCount(); ++i)
    {
        QCPGraph *graph = customPlot->graph(i);
        QCPPlottableLegendItem *item = customPlot->legend->itemWithPlottable(graph);
        if (item->selected() || graph->selected())
        {
            item->setSelected(true);
            graph->setSelection(QCPDataSelection(graph->data()->dataRange()));
        }
    }
}

void TSPlot::mousePress()
{
    QString * text1;
    // if an axis is selected, only allow the direction of that axis to be dragged
    // if no axis is selected, both directions may be dragged

    if (customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        customPlot->axisRect()->setRangeDrag(customPlot->xAxis->orientation());
    else if (customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        customPlot->axisRect()->setRangeDrag(customPlot->yAxis->orientation());
    else
        customPlot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);

    text1 = new QString("Add to Plot");
    //sprintf(count, "%d", this->current_plot+1);
    //text1->append(count);
    pb_add_to_plot->setText(*text1);
}

void TSPlot::mouseWheel()
{
    // if an axis is selected, only allow the direction of that axis to be zoomed
    // if no axis is selected, both directions may be zoomed

    if (customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        customPlot->axisRect()->setRangeZoom(customPlot->xAxis->orientation());
    else if (customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        customPlot->axisRect()->setRangeZoom(customPlot->yAxis->orientation());
    else
        customPlot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
}

void TSPlot::moveLegend()
{
    if (QAction* contextAction = qobject_cast<QAction*>(sender())) // make sure this slot is really called by a context menu action, so it carries the data we need
    {
        bool ok;
        int dataInt = contextAction->data().toInt(&ok);
        if (ok)
        {
            customPlot->axisRect()->insetLayout()->setInsetAlignment(0, (Qt::Alignment)dataInt);
            customPlot->replot();
        }
    }
}

void TSPlot::graphClicked(QCPAbstractPlottable *plottable, int dataIndex)
{
    // since we know we only have QCPGraphs in the plot, we can immediately access interface1D()
    // usually it's better to first check whether interface1D() returns non-zero, and only then use it.
    double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
    QString message = QString("Clicked on graph '%1' at data point #%2 with value %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
    //parent->statusBar()->showMessage(message, 2500); // only for QMainWindow
    //parent->setStatusTip(message);
    //parent->setStatusTip(message);
}
void TSPlot::graphDoubleClicked(QCPAbstractPlottable *plottable, int dataIndex)
{
    double dataValue = plottable->interface1D()->dataMainValue(dataIndex);

    QList<QDateTime> qdt_times = tsfile->get_qdt_times();
    QString qdt = qdt_times[dataIndex].toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString message = QString("Location: %1\nTime: %2\nValue: %3").arg(plottable->name()).arg(qdt).arg(dataValue);
    QMessageBox::information(parent, NULL, message);
}

void TSPlot::contextMenuRequest(QPoint pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (customPlot->legend->selectTest(pos, false) >= 0) // context menu on legend requested
    {
        menu->addAction("Move to top left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop | Qt::AlignLeft));
        menu->addAction("Move to top center", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop | Qt::AlignHCenter));
        menu->addAction("Move to top right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop | Qt::AlignRight));
        menu->addAction("Move to bottom right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom | Qt::AlignRight));
        menu->addAction("Move to bottom left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom | Qt::AlignLeft));
    }
    else  // general context menu on graphs requested
    {
        QAction * location_name= new QAction();
        menu->addAction(location_name);
        menu->addSeparator();
        location_name->setVisible(false);

        for (int i = 0; i<customPlot->graphCount(); ++i)
        {
            QCPGraph *graph = customPlot->graph(i);
            if (graph->selected())
            {
                location_name->setVisible(true);
                location_name->setText(QString("Legend item: %1").arg(graph->name()));
                break;
            }
        }

        menu->addAction("Rescale axes", this, SLOT(contextMenuRescaleAxes()));
        menu->addAction("Remove selected graph", this, SLOT(contextMenuRemoveSelectedGraph()));
        menu->addAction("Print to PDF", this, SLOT(contextMenuSaveAsPDF()));
    }



    menu->popup(customPlot->mapToGlobal(pos));
}

void TSPlot::contextMenuRescaleAxes()
{
    customPlot->rescaleAxes();
    customPlot->replot();
}
void TSPlot::contextMenuSaveAsPDF()
{
    // select first a filename, no actions need to be performed when cancel is pressed

    QString s = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename = s.append(".pdf");
    QFileDialog * fd = new QFileDialog();
    fd->setWindowTitle("Save PDF file");
    fd->setNameFilter("PDF-format (*.pdf)");
    fd->selectFile(filename);
    fd->setAcceptMode(QFileDialog::AcceptSave);
    if (fd->exec() != QDialog::Accepted)
    {
        return;
    }
    QStringList * QFilenames = new QStringList;
    *QFilenames = fd->selectedFiles();
    for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
        filename = *it;
    }
    delete fd;
    customPlot->savePdf(filename, this->plot_width, this->plot_height, QCP::epNoCosmetic, QString("JanM"), QString("PlotCFTS"));
}

void TSPlot::contextMenuRemoveSelectedGraph()
{
    if (customPlot->selectedGraphs().size() > 0)
    {
        _nr_graphs = customPlot->graphCount();
        QString dic_name;

        QCPGraph * ind = customPlot->selectedGraphs().first();
        QString legend_name = ind->name();  // ie legend name
        QStringList dic_names = legend_name.split(":");
        dic_name = dic_names[0];
        QStringList names = this->yaxis_label.split('\n');
        yaxis_label.clear();
        long j = 0;
        for (QStringList::iterator it = names.begin(); it != names.end(); it++)
        {
            j++;
            QString name = *it;
            long add_ylabel = 1;
            if (!dic_name.contains("Y") && _ylabel_dic[dic_name] > 0)
            {
                add_ylabel = 0;  // there was just one label, so remove it
            }
            if (name.contains(dic_name))
            {
                long k = _ylabel_dic[dic_name];
                _ylabel_dic[dic_name] -= 1;
                k = _ylabel_dic[dic_name];
                if (_ylabel_dic[dic_name] == 0)
                {
                    add_ylabel = 0;
                }
            }
            if (add_ylabel == 0)
            {
                // do not add yaxis-label name to the yaxis_label
                j--;
            }
            else
            {
                if (j != 1)
                {
                    yaxis_label.append("\n");
                }
                yaxis_label.append(*it);
            }
        }
        // Zoek in naam naar voorvoegsel Y? en verwijder de daarbij behorende yaxis_label
        customPlot->removeGraph(customPlot->selectedGraphs().first());
        _nr_graphs = customPlot->graphCount();
        // Update yaxis_label and legend
        //set_yaxis_label(name, unit);
        //set_legend();
        _nr_y_axis_labels -= 1;
        customPlot->yAxis->setLabel(this->yaxis_label);
        customPlot->replot();

        if (_nr_graphs == 0)
        {
            _ylabel_dic.clear();
            _nr_y_axis_labels = 0;
        }
    }
}

void TSPlot::onMouseMove(QMouseEvent *event)
{
    int i_time;

    double x = customPlot->xAxis->pixelToCoord(event->pos().x());
    double y = customPlot->yAxis->pixelToCoord(event->pos().y());
    // convert x to date time
    QList<QDateTime> qdt_times = tsfile->get_qdt_times();
    QDateTime * RefDate = tsfile->get_reference_date();
    _time struct_time = tsfile->get_times();
    vector<double> times = struct_time.time;
    int nr_times = tsfile->get_count_times();
    i_time = -1;

    qint64 offset = RefDate->toSecsSinceEpoch();
    if (x < times[0] + offset ||
        x > times[nr_times-1] + offset )
    {
        // nothing
    }
    else
    {
        for (int i = 1; i < nr_times; i++)
        {
            if (times[i-1] + offset > x && x < times[i] + offset)
            {
                double alpha = (x - times[i - 1] - offset) / (times[i] - times[i - 1]);
                double times_aver = (1.0 - alpha) * times[i - 1] +  alpha * times[i];
                QDateTime qdt_aver = RefDate->addSecs(times_aver);
                QString qdt = qdt_aver.toString("hh:mm:ss.zzz dd MMM yyyy");
                customPlot->setToolTipDuration(5000);
                customPlot->setToolTip(QString("%1, %2").arg(qdt).arg(y));
                break;
            }
            else
            {
                customPlot->setToolTipDuration(0);
                customPlot->setToolTip(QString(""));
            }
        }
    }
}

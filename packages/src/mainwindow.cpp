#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;

#if defined(WIN32) || defined(WIN64)
#  define strdup _strdup
#endif
#include "qcustomplot.h"
#include "mainwindow.h"
#include "cf_time_series.h"
#include "plot_cf_time_series_version.h"
#include "program_arguments.h"
#include "time_series_plot.h"
#include "netcdf.h"

QString selectedFilter;
QVBoxLayout * showFilenameLayout;
QGridLayout * showMetaLayout;
QHBoxLayout * showPLTLayout;
QHBoxLayout * showPlotLayout;
QHBoxLayout * showComboBoxLayout;

QDir _exec_dir;
QDir _startup_dir;
char * fileTypeString;
bool canceled;
QIcon * icon;
double * times;

QCPAxis * janm;
int dim_model_wide;
int dim_at_location;
struct _global_attributes * global_attributes;

MainWindow::MainWindow(QDir exec_dir_in, QDir startup_dir_in)
{
#include "vsi.xpm"
    char * prg_name = getprogramstring_plot_cf_time_series();
    char * vers_nr = getversionstring_plot_cf_time_series();
    mainWidget = new QWidget;
    mainWidget->setAttribute(Qt::WA_DeleteOnClose, true);
    setCentralWidget(mainWidget);
    setWindowTitle(tr("%1 %2").arg(prg_name).arg(vers_nr));
    icon = new QIcon(QPixmap( vsi ));
    setWindowIcon(*icon);

    dim_model_wide = -1;
    dim_at_location = -1;
    nr_plot = -1;
    fil_index = -1;
    m_pre_selection = false;  // default no pre selection of parameters and locations

    QDesktopWidget *d = QApplication::desktop();
    int scridx = d->primaryScreen();
    int width = d->screenGeometry(scridx).width();  // returns screen width
    int height = d->screenGeometry(scridx).height(); // returns screen height
    width = (int)(0.4 * double(width));
    height = (int)(0.6 * double(height)); // Do not count the taskbar pixels, assumed to be 1 percent of the screen

    resize(width, height);
    mainWidget->setGeometry(0, 0, width, height);

    int w = d->screenGeometry(scridx).width();  // returns screen width
    int h = d->screenGeometry(scridx).height(); // returns screen height

    w = (int)(0.5 * double(w) - 0.5 * double(width));
    h = (int)(0.5 * double(h) - 0.15 * double(height) - 0.5 * double(height));
    move(w, h);

    _exec_dir = exec_dir_in;
    _startup_dir = startup_dir_in;

    createMenus();
    createStatusBar();

    createFilenameList();
    createComboBox();
    createPLTList();
    createPlotButton();
    createDisplayMeta();

    QVBoxLayout * vl_main = new QVBoxLayout;
    vl_main->addLayout(showFilenameLayout);
    vl_main->addLayout(showComboBoxLayout);
    vl_main->addLayout(showPLTLayout);
    vl_main->addLayout(showPlotLayout);
    vl_main->addLayout(showMetaLayout);
    mainWidget->setLayout(vl_main);

    pb_plot->setFocus();
    pb_plot->setDefault(true);

    free(prg_name); prg_name = NULL;
    free(vers_nr); vers_nr = NULL;
}

void MainWindow::createActions()
{
// File menu
    openAct = new QAction(QIcon(":/images/new.png"), tr("&Open"), this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Open Climate and Forecast compliant Time Series file"));
    openAct->setEnabled(true);
    connect(openAct, SIGNAL(triggered()), this, SLOT(openFile()));

    closeAct = new QAction(tr("&Close"), this);
    closeAct->setShortcut(tr("Ctrl+C"));
    closeAct->setStatusTip(tr("Close all opened files"));
    closeAct->setEnabled(true);
    connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));

    openPreSelect = new QAction(tr("&Open Pre Selection"), this);
    //preSelect->setShortcut(tr("Ctrl+C"));
    openPreSelect->setStatusTip(tr("Open file with preselected parameters and locations"));
    openPreSelect->setEnabled(false);
    connect(openPreSelect, SIGNAL(triggered()), this, SLOT(OpenPreSelection()));

    savePreSelect = new QAction(tr("&Save Pre Selection"), this);
    //preSelect->setShortcut(tr("Ctrl+C"));
    savePreSelect->setStatusTip(tr("Save preselected parameters and locations"));
    savePreSelect->setEnabled(true);
    connect(savePreSelect, SIGNAL(triggered()), this, SLOT(SavePreSelection()));

    exitAct = new QAction(tr("&Exit"), this);
    exitAct->setShortcut(tr("Ctrl+X"));
    exitAct->setStatusTip(tr("Exit PlotCFTS program"));
    exitAct->setEnabled(true);
    connect(exitAct, SIGNAL(triggered()), this, SLOT(exit()));

    // Export menu
    ecsvAct = new QAction(tr("&Export to CSV"), this);
    ecsvAct->setShortcut(tr("Ctrl+E"));
    ecsvAct->setStatusTip(tr("Export selection to Comma Separated Value file"));
    ecsvAct->setEnabled(false);
    connect(ecsvAct, SIGNAL(triggered()), this, SLOT(ExportToCSV()));

    // About menu
    showUserManualAct = new QAction(tr("&User Manual"), this);
    showUserManualAct->setStatusTip(tr("Opens the User Manual (pdf-format)"));
    showUserManualAct->setEnabled(true);
    connect(showUserManualAct, SIGNAL(triggered()), this, SLOT(ShowUserManual()));
    
    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    aboutAct->setEnabled(true);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createMenus()
{
    createActions();

    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAct);
    fileMenu->addAction(closeAct);

    fileMenu->addSeparator();
    fileMenu->addAction(openPreSelect);
    fileMenu->addAction(savePreSelect);

    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    exportMenu = menuBar()->addMenu(tr("&Export"));
    exportMenu->addAction(ecsvAct);
    
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(showUserManualAct);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutAct);
}

void MainWindow::createStatusBar()
{
    char * text = (char *)malloc(160 + 1);
    text[0] = '\0';
    strcat(text, getprogramstring_plot_cf_time_series());
    strcat(text, " compiled: ");
    strcat(text, "" __DATE__ ", ");
    strcat(text, "" __TIME__ "");
    QLabel * label = new QLabel(QString(text), this);
    label->setFrameShape(QFrame::Panel);
    label->setFrameShadow(QFrame::Sunken);
    statusBar()->addWidget(label);

    //char * text1 = getfullversionstring_qcustomplot();
    char * text1 = strdup("QCustomPlot version 2.0.1 (25 juni 2018)");
    text[0] = '\0';
    strcat(text, text1);
    label = new QLabel(QString(text), this);
    label->setFrameShape(QFrame::Panel);
    label->setFrameShadow(QFrame::Sunken);
    statusBar()->addWidget(label);

    pgBar = new QProgressBar();
    pgBar->hide();
    pgBar->setMaximum(1000);
    pgBar->setValue(0);
    pgBar->setMaximumWidth(150);
    statusBar()->addPermanentWidget(pgBar, 150);
    free(text);
}

void MainWindow::openFile()
{
    FILE_TYPE ftype;

    QString fname;
    QString * str = new QString();
    QFileDialog * fd = new QFileDialog();
    QStringList * list = new QStringList();

    str->clear();
    str->append("TSFILE netCDF");
    str->append(" (*.nc)");
    list->append(*str);

    fd->setWindowTitle("Open Time Series file");
    fd->setNameFilters(*list);
    fd->selectNameFilter(list->at(0));

    canceled = fd->exec() != QDialog::Accepted;
    if (!canceled)
    {
        QStringList * QFilenames = new QStringList();
        QFilenames->append(fd->selectedFiles());
        for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
            fname = *it;
        }
        ftype = HISTORY;
        openFile(fname, ftype);
    }
    delete fd;
    delete str;
}

void MainWindow::openFile(QFileInfo ncfile, FILE_TYPE type)
{
    int ncid;
    char * fname = strdup(ncfile.absoluteFilePath().toUtf8());
    int status = nc_open(fname, NC_NOWRITE, &ncid);
    (void)nc_close(ncid);
    if (status != NC_NOERR)
    {
        QMessageBox::warning(this, QObject::tr("Warning"), QObject::tr("Cannot open netCDF file:\n%1\nThis filename is not supported by this Climate and Forecast time series plot program.").arg(ncfile.absoluteFilePath()));
        return;
    }
    // If file already opened, do not open it again.
    // test is performed on absolutePath
    for (int i = 0; i < lb_filenames->count(); i++)
    {
        if ( &(this->openedUgridFile[i]) != NULL)
        {
            if (QString(this->openedUgridFile[i]->fname.absoluteFilePath()) == QString(ncfile.absoluteFilePath()))
            {
                QMessageBox::warning(this, QObject::tr("Warning"), QObject::tr("File is already opened:\n%1").arg(ncfile.absoluteFilePath()));
                return;
            }
        }
    }
    fil_index += 1;
    if (fil_index == 0)
    {
        this->openedUgridFile = (TSFILE **)malloc(sizeof(TSFILE *) * (fil_index + 1));
    }
    else
    {
        this->openedUgridFile = (TSFILE **)realloc(this->openedUgridFile, sizeof(TSFILE *) * (fil_index + 1));
    }
    TSFILE * tsfile = new TSFILE(ncfile, type);
    this->openedUgridFile[fil_index] = tsfile;

    updateFileListBox(tsfile);
}


void MainWindow::ExportToCSV()
{
    int i_tsfile_par = -1;

    int fil_index = this->lb_filenames->currentRow();
    TSFILE * tsfile = this->openedUgridFile[fil_index];

    int cb_index = this->cb_par_loc->currentIndex();
    struct _parameter * parameter = tsfile->get_parameters(cb_index);
    struct _location * location = tsfile->get_locations(cb_index);

    int nr_layers = this->sb_layer->maximum();
    int i_layer = this->sb_layer->value() - 1;  // used as array index (ie zero based)

    double *** y_values;

    // select first a filename, no actions need to be performed when cancel is pressed

    QString s = QDateTime::currentDateTime().toString("yyyy-MM-dd_hhmmss");
    QString filename = s.append(".csv");
    QFileDialog * fd = new QFileDialog();
    fd->setWindowTitle("Export to CSV file");
    fd->setNameFilter("Comma Separated Value (*.csv)");
    fd->setDirectory(_startup_dir);
    fd->selectFile(filename);
    fd->setConfirmOverwrite(true);
    fd->setAcceptMode(QFileDialog::AcceptSave);
    if (fd->exec() != QDialog::Accepted)
    {
        return;
    }
    QStringList * QFilenames = new QStringList();
    QFilenames->append(fd->selectedFiles());
    for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
        filename = *it;
    }
    delete fd;

    char * csv_filename = strdup(filename.toUtf8());

    // get selected parameter
    long nr_pars = 0;
    long * pars = (long *)malloc(sizeof(long) * 1);
    pars[0] = -1;
    for (long i = 0; i < lb_parameters->count(); i++)
    {
        QListWidgetItem * item = lb_parameters->item(i);
        if (item->isSelected())
        {
            pars = (long *)realloc(pars, sizeof(long *) * (nr_pars + 1));
            for (long j = 0; j < lb_parameters->count(); j++)
            {
                QString *tmp_name = new QString(parameter[j].name);
                if (!tmp_name->compare(this->lb_parameters->item(i)->text().toUtf8()))
                {
                    pars[nr_pars] = j;
                    nr_pars = nr_pars + 1;
                    break;
                }
                delete tmp_name;
            }
        }
    }
    // get selected locations
    long nr_locs = 0;
    long * locs = (long *)malloc(sizeof(long) * 1);
    locs[0] = -1;
    for (long i = 0; i < lb_locations->count(); i++)
    {
        QListWidgetItem * item = lb_locations->item(i);
        if (item->isSelected())
        {
            locs = (long *)realloc(locs, sizeof(long *) * (nr_locs + 1));
            for (long j = 0; j < lb_locations->count(); j++)
            {
                if (!location[j].name->compare(this->lb_locations->item(i)->text().toUtf8()))
                {
                    locs[nr_locs] = j;
                    nr_locs = nr_locs + 1;
                    break;
                }
            }
        }
    }
    // get selected times
    long nr_tims = 0;
    long * tims = (long *)malloc(sizeof(long) * 1);
    if (lb_times->currentRow() != -1)
    {
        for (int i = 0; i < lb_times->count(); i++)
        {
            QListWidgetItem * item = lb_times->item(i);
            if (item->isSelected())
            {
                tims = (long *)realloc(tims, sizeof(long) * (nr_tims + 1));
                tims[nr_tims] = i;
                nr_tims = nr_tims + 1;
            }
        }
    }
    else
    {
        nr_tims = lb_times->count();
        tims = (long *)realloc(tims, sizeof(long) * (nr_tims));
        for (int i = 0; i < lb_times->count(); i++)
        {
            tims[i] = i;
        }
    }

    ofstream fpo;
    fpo.open(csv_filename);
    int i_rec = 1;
    QString name1(getfullversionstring_plot_cf_time_series() + 4);
    fpo << i_rec << " Created by:, " <<  name1.trimmed().replace(",", ";").toStdString() << endl;  // Skip the @(#) string in the result of getfullversionstring_plot_cf_time_series()
    i_rec++;
    fpo << i_rec << " Results taken from file:, " << strdup(tsfile->fname.absoluteFilePath().toUtf8()) << endl;

    // List the meta-data
    global_attributes = tsfile->get_global_attributes();
    for (int i = 0; i < global_attributes->count; i++)
    {
        i_rec++;
        QString name1(global_attributes->attribute[i]->name);
        QString name2(global_attributes->attribute[i]->cvalue);
        fpo << i_rec << " " << name1.trimmed().replace(",", ";").toStdString() << ", " << name2.trimmed().replace(",", ";").toStdString() << endl;
    }

    i_rec++;
    fpo << i_rec << " ------------------" << endl;
    i_rec++;
    fpo << i_rec << " Parameters: " << nr_pars;  // No endl, list of stations is given
    for (int j = 0; j < nr_pars; j++)
    {
        for (int i = 0; i < nr_locs; i++)
        {
            QString q_name(parameter[pars[j]].name);
            if (nr_layers != 0)
            {
                fpo << ", " << strdup(q_name.toUtf8().trimmed().replace(",", ";")) << " [" << parameter[pars[j]].unit << "], " << "layer: " << i_layer + 1;
            }
            else {
                fpo << ", " << strdup(q_name.toUtf8().trimmed().replace(",", ";")) << " [" << parameter[pars[j]].unit << "]";
            }
        }
    }
    fpo << endl;

    i_rec++;
    fpo << i_rec << " Locations : " << nr_locs;  // No endl, list of stations is given
    for (int j = 0; j < nr_pars; j++)
    {
        for (int i = 0; i < nr_locs; i++)
        {
            QString q_loc(*location[i].name);
            fpo << ", " << strdup(q_loc.toUtf8().trimmed().replace(",", ";"));
        }
    }
    fpo << endl;
    i_rec++;
    fpo << i_rec << " Time steps: " << nr_tims << endl;
    i_rec++;
    fpo << i_rec << " ----------------- " << endl;
    fpo << "DateTime";
    for (int j = 0; j < nr_pars; j++)
    {
        QString q_name(parameter[pars[j]].name);
        for (int i = 0; i < nr_locs; i++)
        {
            QString q_loc(*location[locs[i]].name);
            fpo << ", " << strdup(q_name.toUtf8().trimmed().replace(",", ";")) << " --- " << strdup(q_loc.toUtf8().trimmed().replace(",",";"));  // In a csv file, the names do not allow commas
        }
    }
    fpo << endl;

    // allocate memory for the y-values
    y_values = (double ***)malloc(sizeof(double **) * nr_pars);
    double * d = (double *)malloc(sizeof(double) * nr_pars * nr_locs * nr_tims);
    for (int j = 0; j < nr_pars; j++)
    {
        y_values[j] = (double **)malloc(sizeof(double *) * nr_locs);
        for (int i = 0; i < nr_locs; i++)
        {
            y_values[j][i] = d + j * nr_locs * nr_tims + i * nr_tims;
        }
    }

    for (int j = 0; j < nr_pars; j++)
    {
        {
            i_tsfile_par = pars[j];

            for (int i = 0; i < nr_locs; i++)
            {
                int i_par_loc = this->cb_par_loc->currentIndex();
                y_values[j][i] = tsfile->get_time_series(i_par_loc, parameter[pars[j]].name, locs[i], i_layer);
            }
        }
    }

    char * datumtijd = (char *)malloc(sizeof(char) * (4 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 2 + 4 + 1));  //  19+4 +1 = 24 = "yyyy-MM-dd hh:mm:ss.zzz"
    QList<QDateTime> qdt_times = tsfile->get_qdt_times();
    double dt = qdt_times.at(0).msecsTo(qdt_times.at(1));
    for (int k = 0; k < nr_tims; k++)
    {
        QString qdt;
        if (dt < 1000.)  // milli seconds
        {
            qdt = qdt_times[tims[k]].toString("yyyy-MM-dd hh:mm:ss.zzz");
        }
        else
        {
            qdt = qdt_times[tims[k]].toString("yyyy-MM-dd hh:mm:ss");
        }
        strcpy(datumtijd, qdt.toUtf8());

        fpo << datumtijd;
        for (int j = 0; j < nr_pars; j++)
        {
            for (int i = 0; i < nr_locs; i++)
            {
                fpo << std::scientific << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << ", " << y_values[j][i][tims[k]];  // maximal precision
            }
        }
        fpo << endl;
    }

    for (int j = 0; j < nr_pars; j++)
    {
        free(y_values[j]); y_values[j] = NULL;
    }
    fpo.close();

    // delete location_name //    todo QString **  = tsfile->get_location_names();
    free(csv_filename); csv_filename = NULL;
    free(y_values); y_values = NULL;
    free(d); d = NULL;
    free(datumtijd);
    free(pars); pars = NULL;
    free(locs); locs = NULL;
    free(tims); tims = NULL;
}

void MainWindow::OpenPreSelection()
{
    QString fname;
    QString * str = new QString();
    QFileDialog * fd = new QFileDialog();
    QStringList * list = new QStringList();

    str->clear();
    str->append("Pre-selection");
    str->append(" (*_presel.json)");
    list->append(*str);

    fd->setWindowTitle("Open file with pre-selections for parameters and locations");
    fd->setNameFilters(*list);
    fd->selectNameFilter(list->at(0));

    canceled = fd->exec() != QDialog::Accepted;
    if (!canceled)
    {
        QStringList * QFilenames = new QStringList();
        QFilenames->append(fd->selectedFiles());
        for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
            fname = *it;
        }
        TSFILE * tsfile = this->openedUgridFile[fil_index];
        OpenPreSelection(tsfile, fname);
    }
    delete fd;
    delete str;
}

void MainWindow::OpenPreSelection(TSFILE * tsfile, QFileInfo json_file)
{
    int status = -1;
    char * fname = strdup(json_file.absoluteFilePath().toUtf8());
    READ_JSON * pt_pre_selection = new READ_JSON(fname);
    int cnt = tsfile->get_count_par_loc();
    for (int cb_index = 0; cb_index < cnt; cb_index++)
    {
        string name = string(tsfile->get_par_loc_long_name(cb_index));
        status = pt_pre_selection->find(name);
        if (status == 1)  // name is not in the json file
        {
            struct _parameter * parameter = tsfile->get_parameters(cb_index);
            long nr_parameters = tsfile->get_count_parameters(cb_index);
            struct _location *  location = tsfile->get_locations(cb_index);
            long nr_locations = tsfile->get_count_locations(cb_index);
            for (long i = 0; i < nr_parameters; i++)
            {
                parameter[i].pre_selected = 1;
                tsfile->put_parameter(cb_index, i, parameter);
            }
            for (long i = 0; i < nr_locations; i++)
            {
                location[i].pre_selected = 1;
                tsfile->put_location(cb_index, i, location);
            }
            continue;
        }

        string values = name + ".location.name";
        vector<string> pre_locations;
        status = pt_pre_selection->get(values, pre_locations);
        if (pre_locations.size() != 0)
        {
            // set pre-selection for locations
            struct _location * location = tsfile->get_locations(cb_index);
            for (int j = 0; j < tsfile->get_count_locations(cb_index); j++)
            {
                for (int k = 0; k < pre_locations.size(); k++)
                {
                    string loc = location[j].name->toStdString();
                    if (pre_locations[k] == loc)  // compare string with string
                    {
                        location[j].pre_selected = 1;
                        tsfile->put_location(cb_index, j, location);
                        m_pre_selection = true;
                    }
                }
            }
        }

        values = name + ".parameter.name";
        vector<string> pre_parameters;
        status = pt_pre_selection->get(values, pre_parameters);
        if (pre_parameters.size() != 0)
        {
            // set pre-selection for parameters
            struct _parameter * parameter = tsfile->get_parameters(cb_index);
            for (int j = 0; j < tsfile->get_count_parameters(cb_index); j++)
            {
                for (int k = 0; k < pre_parameters.size(); k++)
                {
                    string param(parameter[j].name);
                    if (pre_parameters[k] == param)  // compare string with string
                    {
                        parameter[j].pre_selected = 1;
                        tsfile->put_parameter(cb_index, j, parameter);
                        m_pre_selection = true;
                    }
                }
            }
        }
    }
}

void MainWindow::SavePreSelection()
{
    int fil_index = this->lb_filenames->currentRow();
    TSFILE * tsfile = this->openedUgridFile[fil_index];

    // select first a filename, no actions need to be performed when cancel is pressed
    QString bname = tsfile->fname.baseName();
    QString filename = bname.append("_presel.json");
    QFileDialog * fd = new QFileDialog();
    fd->setWindowTitle("Save pre selection to file");
    fd->setNameFilter("pre-selection (*.json)");
    fd->setDirectory(_startup_dir);
    fd->selectFile(filename);
    fd->setConfirmOverwrite(true);
    fd->setAcceptMode(QFileDialog::AcceptSave);
    if (fd->exec() != QDialog::Accepted)
    {
        return;
    }
    QStringList * QFilenames = new QStringList();
    QFilenames->append(fd->selectedFiles());
    for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
        filename = *it;
    }
    delete fd;

    char * json_preselect_filename = strdup(filename.toUtf8());

    // make the property tree (pt)
    boost::property_tree::ptree pt_root, pt_param, pt_location;
    //for (int cb_index = 0; cb_index < nr_par_loc; cb_index++)
    {
        int cb_index = this->cb_par_loc->currentIndex();
        struct _parameter * parameter = tsfile->get_parameters(cb_index);
        long nr_parameters = tsfile->get_count_parameters(cb_index);
        struct _location *  location = tsfile->get_locations(cb_index);
        long nr_locations = tsfile->get_count_locations(cb_index);
        // get selected parameter
        long nr_pars = 0;
        long * pars = (long *)malloc(sizeof(long) * 1);
        pars[0] = -1;
        for (long i = 0; i < nr_parameters; i++)
        {
            parameter[i].pre_selected = 0;
            tsfile->put_parameter(cb_index, i, parameter);
        }
        for (long i = 0; i < lb_parameters->count(); i++)
        {
            QListWidgetItem * item = lb_parameters->item(i);
            if (item->isSelected())
            {
                pars = (long *)realloc(pars, sizeof(long *) * (nr_pars + 1));
                for (long j = 0; j < nr_parameters; j++)
                {
                    QString *q_name = new QString(parameter[j].name);
                    QString s_name = this->lb_parameters->item(i)->text();
                    if (!q_name->compare(s_name))
                    {
                        parameter[j].pre_selected = 1;
                        tsfile->put_parameter(cb_index, j, parameter);
                        pars[nr_pars] = j;
                        nr_pars = nr_pars + 1;
                        break;
                    }
                }
            }
        }
        // get selected locations
        long nr_locs = 0;
        long * locs = (long *)malloc(sizeof(long) * 1);
        locs[0] = -1;
        for (long i = 0; i < nr_locations; i++)
        {
            location[i].pre_selected = 0;
            tsfile->put_location(cb_index, i, location);
        }
        for (long i = 0; i < lb_locations->count(); i++)
        {
            QListWidgetItem * item = lb_locations->item(i);
            if (item->isSelected())
            {
                locs = (long *)realloc(locs, sizeof(long *) * (nr_locs + 1));
                for (long j = 0; j < nr_locations; j++)
                {
                    QString q_name(*location[j].name);
                    QString s_name = this->lb_locations->item(i)->text();
                    if (!q_name.compare(s_name))
                    {
                        location[j].pre_selected = 1;
                        tsfile->put_location(cb_index, j, location);
                        locs[nr_locs] = j;
                        nr_locs = nr_locs + 1;
                        break;
                    }
                }
            }
        }

        QString cb_name = this->cb_par_loc->currentText();
        QString tmp;

        for (int j = 0; j < nr_pars; j++)
        {
            QString q_name(parameter[pars[j]].name);
            boost::property_tree::ptree elem;
            elem.put<string>("name", strdup(q_name.toUtf8().trimmed()));
            pt_param.push_back(std::make_pair("", elem));
        }
        tmp = cb_name + ".parameter";
        pt_root.put_child(tmp.toStdString(), pt_param);
    
        for (int j = 0; j < nr_locs; j++)
        {
            QString q_name(*location[locs[j]].name);
            boost::property_tree::ptree elem;
            elem.put<string>("name", strdup(q_name.toUtf8().trimmed()));
            pt_location.push_back(std::make_pair("", elem));
        }
        tmp = cb_name + ".location";
        pt_root.put_child(tmp.toStdString(), pt_location);
    }
    boost::property_tree::write_json(json_preselect_filename, pt_root);
}

void MainWindow::updateFileListBox(TSFILE * tsfile)
{
    int cnt;
    QString * text1;
    char count[11];

    if (tsfile != NULL)
    {
        lb_filenames->addItem(tsfile->fname.fileName().toUtf8());
    }
    cnt = lb_filenames->count();
    if (cnt == 0)
    {
        cb_add_to_plot_prefix->setEnabled(false);
        cb_add_to_plot->setEnabled(false);
        pb_add_to_plot->setEnabled(false);
        openPreSelect->setEnabled(false);
        this->fil_index = -1;
    }
    else
    {
        openPreSelect->setEnabled(true);
    }

    text1 = new QString("Filenames (count ");
    sprintf(count, "%d", lb_filenames->count());
    text1->append(count);
    text1->append(")");
    gb_filenames->setTitle(*text1);

    lb_filenames->setCurrentRow(cnt - 1);  // select the last read file
}

void MainWindow::updateListBoxes(TSFILE * tsfile)
{
    int nr_parameters;
    int nr_locations;
    int nr_times;
    struct _parameter * param;
    struct _location * location;
    char count[11];
    QString * text1;
    QList<QDateTime> qdt_times;
    int cb_indx;
    int cnt;

    // fill the combobox and select the first one
    cb_indx = this->cb_par_loc->currentIndex();
    this->cb_par_loc->blockSignals(true);
    this->cb_par_loc->clear();
    this->cb_par_loc->blockSignals(false);
    if (tsfile == NULL)
    {
        cnt = 0;
    }
    else
    {
        cnt = tsfile->get_count_par_loc();
        lb_filenames->setToolTip(tsfile->fname.canonicalFilePath());
    }
    cb_indx = cb_indx >= cnt-1 ? cnt-1 : cb_indx;
    cb_par_loc->setEnabled(false);
    if (cnt >= 0)
    {
        cb_par_loc->setEnabled(true);
    }

    for (int i = 0; i < cnt; i++)
    {
        char * name = tsfile->get_par_loc_long_name(i);
        this->cb_par_loc->addItem(QString(name).trimmed());
    }
    cb_indx = max(0, cb_indx);
    this->cb_par_loc->setCurrentIndex(cb_indx);

    // List the parameters
    if (tsfile == NULL)
    {
        nr_parameters = 0;
    }
    else
    {
        nr_parameters = tsfile->get_count_parameters(cb_indx);
        param = tsfile->get_parameters(cb_indx);
    }

    pb_plot->setEnabled(true);
    if (nr_parameters == 0)
    {
        pb_plot->setEnabled(false);
    }
    lb_parameters->blockSignals(true);
    lb_parameters->clear();
    for (int i = 0; i < nr_parameters; i++)
    {
        if (!m_pre_selection || param[i].pre_selected == 1)
        {
            lb_parameters->addItem(QString(param[i].name));
        }
    }
    lb_parameters->blockSignals(false);
    lb_parameters->setCurrentRow(0);

    text1 = new QString("Parameter (count ");
    sprintf(count, "%d", lb_parameters->count());
    text1->append(count);
    text1->append(")");
    gb_parameters->setTitle(*text1);

    // List the locations
    if (tsfile == NULL)
    {
        nr_locations = 0;
    }
    else
    {
        nr_locations = tsfile->get_count_locations(cb_indx);
        location = tsfile->get_locations(cb_indx);
    }

    lb_locations->blockSignals(true);
    lb_locations->clear();
    for (int i = 0; i < nr_locations; i++)
    {
        if (!m_pre_selection || location[i].pre_selected == 1)
        {
            lb_locations->addItem(*location[i].name);
        }
    }
    lb_locations->blockSignals(false);
    lb_locations->setCurrentRow(0);

    text1 = new QString("Location (count ");
    sprintf(count, "%d", lb_locations->count());
    text1->append(count);
    text1->append(")");
    gb_locations->setTitle(*text1);

    // List the times
    if (tsfile == NULL)
    {
        nr_times = 0;
    }
    else
    {
        nr_times = tsfile->get_count_times();
        qdt_times = tsfile->get_qdt_times();
    }
    lb_times->blockSignals(true);
    lb_times->clear();
    double dt = 0;
    if (nr_times >= 2)
    {
        dt = qdt_times.at(0).msecsTo(qdt_times.at(1));
    }
    for (int i = 0; i < nr_times; i++)
    {
        if (dt < 1000.)
        {
            lb_times->addItem(qdt_times[i].toString("yyyy-MM-dd hh:mm:ss.zzz"));
        }
        else
        {
            lb_times->addItem(qdt_times[i].toString("yyyy-MM-dd hh:mm:ss"));
        }
    }
    lb_times->blockSignals(false);

    text1 = new QString("Time series (count ");
    sprintf(count, "%d", lb_times->count());
    text1->append(count);
    text1->append(")");
    gb_times->setTitle(*text1);

    // List the meta-data
    if (tsfile != NULL)
    {
        global_attributes = tsfile->get_global_attributes();
        for (int i = 0; i < global_attributes->count; i++)
        {
            QStandardItem * itm = new QStandardItem(global_attributes->attribute[i]->name);
            table_model->setItem(i, 0, itm);
            QStandardItem * itm2 = new QStandardItem(global_attributes->attribute[i]->cvalue);
            table_model->setItem(i, 1, itm2);
        }
    }
    else
    {
        table_model->setRowCount(1);
        table_model->setColumnCount(2);
        QStandardItem * itm = new QStandardItem("");
        table_model->setItem(0, 0, itm);
        QStandardItem * itm2 = new QStandardItem("");
        table_model->setItem(0, 1, itm2);
    }
    table->resizeRowsToContents();
    table->resizeColumnsToContents();
    // Enable the export menu item
    if (lb_parameters->count() > 0 && lb_locations->count() && lb_times->count())
    {
        ecsvAct->setEnabled(true);
    }
    else
    {
        ecsvAct->setEnabled(false);
    }
}

void MainWindow::closeFile()
{
    openAct->setEnabled(true);
}

void MainWindow::about()
{
   char * text;
   char * source_url;
   QString * qtext;
   QString * qsource_url;
   QString * msg_text;
   text = getversionstring_plot_cf_time_series();
   qtext = new QString(text);
   source_url = getsourceurlstring_plot_cf_time_series();
   qsource_url = new QString(source_url);

   msg_text = new QString("Deltares\n");
   msg_text->append("Plot Climate and Forecast compliant Time Series\n");
   msg_text->append(qtext);
   msg_text->append("\nSource: ");
   msg_text->append(qsource_url);
   QMessageBox::about(this, tr("About"), *msg_text);

}

void MainWindow::ShowUserManual()
{
     char pdf_reader[1024];
     int spawn_err = 0;

     QString manual_path = _exec_dir.absolutePath();
     QString user_manual = manual_path + "/doc/" + QString("plotcfts_um.pdf");
     QByteArray manual = user_manual.toUtf8();
     char * pdf_document = manual.data();

     FILE *fp = fopen(pdf_document, "r");
     if (fp != NULL)
     {
          fclose(fp);
          long res = (long) FindExecutableA((LPCSTR)pdf_document, NULL, (LPSTR)pdf_reader);
          if (res >= 32)
          {
               spawn_err = QT_SpawnProcess(NO_WAIT_MODE, pdf_reader, &pdf_document);
          }
     }
	 else
	 {
		 QMessageBox::warning(NULL, QObject::tr("Warning"), QObject::tr("Cannot open file: %1").arg(user_manual));
	 }
}
/* @@-------------------------------------------------
Function:   QT_SpawnProcess
Author:     Jan Mooiman
Function:   Start an process, main program will wait or continue
depending on argument waiting
Context:    -
-------------------------------------------------------*/
int MainWindow::QT_SpawnProcess(int waiting, char * prgm, char ** args)
{
     int i;
     int status;
     QStringList argList;

     status = -1;  /* set default status as not started*/

     QProcess * proc = new QProcess();

     i = 0;
     while (args[i] != NULL && i < 1)  // just one argument allowed
     {
          argList << args[i];
          i++;
     }
     proc->start(prgm, argList);

     if (proc->state() == QProcess::NotRunning)
     {
          delete proc;
          return status;
          // error handling
     }
     //
     // Process sstarted succesfully
     //
     status = 0;
     if (waiting == WAIT_MODE)
     {
          proc->waitForFinished();
          delete proc;
     }
     return status;
}

void MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::restoreOverrideCursor();

    statusBar()->showMessage(tr("File loaded"), 2000);
}

void MainWindow::dragEnterEvent(QDragEnterEvent * event)
{
    if (event->mimeData()->hasFormat("text/uri-list"))
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent * event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
    {
        return;
    }
    QStringList fnames;
    fnames.reserve(urls.size());
    foreach(const QUrl &url, urls)
    {
        fnames.append(url.toLocalFile());
    }
    QString filename;
    for (int i = 0; i < fnames.size(); i++)
    {
        filename = fnames.at(i);
        if (filename.isEmpty())
        {
            return;
        }
        openFile(filename, HISTORY);
    }
}


void MainWindow::createFilenameList()
{
    gb_filenames = new QGroupBox(tr("Filenames"));

    lb_filenames = new QListWidget;
    lb_filenames->setSortingEnabled(false);
    lb_filenames->sortItems(Qt::AscendingOrder);
    //lb_filenames->setSizeAdjustPolicy(QListWidget::AdjustToContents);
    lb_filenames->setMaximumHeight(66);
    lb_filenames->setContextMenuPolicy(Qt::CustomContextMenu);

    QSizePolicy sp = lb_filenames->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Minimum);
    //sp.setVerticalStretch(1);
    lb_filenames->setSizePolicy(sp);

    QVBoxLayout * vb_filenames = new QVBoxLayout;
    vb_filenames->addWidget(lb_filenames);
    gb_filenames->setLayout(vb_filenames);

    showFilenameLayout = new QVBoxLayout;
    showFilenameLayout->addWidget(gb_filenames);

    gb_filenames->setTitle(QString("Filenames (count 0)"));

    connect(lb_filenames, SIGNAL(itemClicked(QListWidgetItem *))            , this, SLOT(lb_filenames_clicked(QListWidgetItem *)));
    connect(lb_filenames, SIGNAL(itemSelectionChanged())                    , this, SLOT(lb_filenames_selection_changed()));
	connect(lb_filenames, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenuFileListBox(const QPoint &)));
}
void MainWindow::lb_filenames_clicked(QListWidgetItem * item)
{
    Q_UNUSED(item);
}

void MainWindow::lb_filenames_selection_changed()
{
    int fil_curr = lb_filenames->currentRow();
    if (fil_curr < 0)
    {
        return;
    }
    TSFILE * tsfile = this->openedUgridFile[fil_curr];
	lb_filenames->setToolTip(tsfile->fname.canonicalFilePath());
	long ierr = tsfile->read(this->pgBar);
    // Preselection file
    QString bname = tsfile->fname.baseName();
    QFileInfo filename = bname.append("_presel.json");
    if (filename.exists())
    {
        //QMessageBox::information(this, QObject::tr("Information"), QObject::tr("File exist:\n%1").arg(filename.absoluteFilePath()));
        OpenPreSelection(tsfile, filename);
    }
    if (ierr == 0)
    {
        updateListBoxes(tsfile);
    }
    else
    {
        lb_filenames->blockSignals(true);
        delete this->openedUgridFile[fil_index];
        this->openedUgridFile[fil_index] = NULL;
        lb_filenames->takeItem(fil_index);
        lb_filenames->setToolTip("");
        updateFileListBox(NULL);
        lb_filenames->blockSignals(false);

        fil_index--;
    }
    pgBar->setValue(1000);
    this->pgBar->hide();
}

void MainWindow::createDisplayMeta()
{
    char *   text;
    text = (char *)malloc(sizeof(char) * (41 + 1));

    showMetaLayout = new QGridLayout();
    QGroupBox * gb_meta = new QGroupBox("Global Attributes");
    gb_meta->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    table = new QTableView();
    long rows = 1;
    long cols = 2;
    int i, j;
    table_model = new QStandardItemModel(rows, cols, this);
    table_model->setHeaderData(0, Qt::Horizontal, tr("Attribute"));
    table_model->setHeaderData(1, Qt::Horizontal, tr("Attribute value"));

    table->setModel(table_model);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: rgb(240, 240, 240) }");
    table->horizontalHeader()->setFrameStyle(QFrame::Sunken);
    table->verticalHeader()->hide();
    table->setMaximumHeight(100);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    table->setShowGrid(true);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setWordWrap(true);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    //table->setStyleSheet("QTableView { background-color: rgb(235, 235, 235); }");

    for (j = 0; j < cols; j++)
    {
        for (i = 0; i < rows; i++)
        {
            QStandardItem * itm = new QStandardItem();
            itm->setText(" ");
            table_model->setItem(i, j, itm);
        }
    }

    QLayout * vl_table = new QVBoxLayout;
    vl_table->addWidget(table);
    gb_meta->setLayout(vl_table);
    showMetaLayout->addWidget(gb_meta, 0, 0);
}

void MainWindow::createPLTList()
{
    createParameterList();
    createLocationList();
    createTimeList();

    showPLTLayout = new QHBoxLayout;
    showPLTLayout->addWidget(gb_parameters);
    showPLTLayout->addWidget(gb_locations);
    showPLTLayout->addWidget(gb_times);
}


void MainWindow::createParameterList()
{
    gb_parameters = new QGroupBox(tr("Parameters"));

    lb_parameters = new QListWidget;
    lb_parameters->setSelectionMode(QAbstractItemView::ExtendedSelection);
    lb_parameters->setSortingEnabled(true);
    lb_parameters->sortItems(Qt::AscendingOrder);
    lb_parameters->setSizeAdjustPolicy(QListWidget::AdjustToContents);

    QVBoxLayout * vb_parameters = new QVBoxLayout;
    vb_parameters->addWidget(lb_parameters);
    gb_parameters->setLayout(vb_parameters);

    gb_parameters->setTitle(QString("Parameter (count 0)"));

    connect(lb_parameters, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(lb_parameter_clicked(QListWidgetItem *)));
    connect(lb_parameters, SIGNAL(itemSelectionChanged()), this, SLOT(lb_parameter_selection_changed()));
}
void MainWindow::lb_parameter_clicked(QListWidgetItem * item)
{
    Q_UNUSED(item);
}
void MainWindow::lb_parameter_selection_changed()
{
    pb_plot->setDefault(false);

    int fil_index = lb_filenames->currentRow();
    TSFILE * tsfile = this->openedUgridFile[fil_index];
    int cb_index = this->cb_par_loc->currentIndex();
    struct _parameter * param = tsfile->get_parameters(cb_index);

    QModelIndex index = lb_parameters->currentIndex();
    char * name = strdup(index.data(Qt::DisplayRole).toString().toUtf8());
    int par_index = lb_parameters->currentRow();
    for (int i = 0; i < lb_parameters->count(); i++)
    {
        if (!strcmp(name, param[i].name))
        {
            par_index = i;
        }
    }
    bool setVisible = param[par_index].ndim == 3;  // Third dimension is the layer
    if (setVisible)
    {

        int maxVal = param[par_index].dim_val[2];
        this->layerLabelPrefix->setText(tr("Layer"));
        this->layerLabelSuffix->setText(tr("[1, %1]").arg(maxVal));
        this->sb_layer->setRange(1, maxVal);
        this->sb_layer->setValue(1);
        this->layerLabelPrefix->setEnabled(true);
        this->layerLabelSuffix->setEnabled(true);
        this->sb_layer->setEnabled(true);
    }
    else
    {
        this->layerLabelPrefix->setText(tr("Layer"));
        this->layerLabelSuffix->setText(tr("[0, 0]"));
        this->sb_layer->setRange(0, 0);
        this->sb_layer->setValue(0);
        this->layerLabelPrefix->setEnabled(false);
        this->layerLabelSuffix->setEnabled(false);
        this->sb_layer->setEnabled(false);
        this->sb_layer->setValue(0);
    }
}


void MainWindow::createLocationList()
{
    gb_locations = new QGroupBox(tr("Locations"));

    lb_locations = new QListWidget;
    lb_locations->setSelectionMode(QAbstractItemView::ExtendedSelection);
    lb_locations->setSortingEnabled(true);
    lb_locations->sortItems(Qt::AscendingOrder);
    lb_locations->setSizeAdjustPolicy(QListWidget::AdjustToContents);
    lb_locations->setUniformItemSizes(true);

    QVBoxLayout * vb_locations = new QVBoxLayout;
    vb_locations->addWidget(lb_locations);
    gb_locations->setLayout(vb_locations);

    gb_locations->setTitle(QString("Location (count 0)"));

    connect(lb_locations, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(lb_location_clicked(QListWidgetItem *)));
    connect(lb_locations, SIGNAL(itemSelectionChanged()), this, SLOT(lb_location_selection_changed()));
}
void MainWindow::lb_location_clicked(QListWidgetItem * item)
{
    pb_plot->setDefault(false);
    Q_UNUSED(item);
}
void MainWindow::lb_location_selection_changed()
{
}

void MainWindow::createTimeList()
{
    gb_times = new QGroupBox(tr("Time series"));

    lb_times = new QListWidget;
    lb_times->setSelectionMode(QAbstractItemView::ContiguousSelection);
    lb_times->setSortingEnabled(false);
    lb_times->sortItems(Qt::AscendingOrder);
    lb_times->setSizeAdjustPolicy(QListWidget::AdjustToContents);
    lb_times->setUniformItemSizes(true);

    QVBoxLayout * vb_times = new QVBoxLayout;
    vb_times->addWidget(lb_times);

    gb_times->setLayout(vb_times);

    gb_times->setTitle(QString("Time series (count 0)"));

    connect(lb_times, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(lb_times_clicked(QListWidgetItem *)));
    connect(lb_times, SIGNAL(itemSelectionChanged()), this, SLOT(lb_times_selection_changed()));
}
void MainWindow::lb_times_clicked(QListWidgetItem * item)
{
    pb_plot->setDefault(false);
    Q_UNUSED(item);
}
void MainWindow::lb_times_selection_changed()
{
}

void MainWindow::createPlotButton()
{
    showPlotLayout = new QHBoxLayout();

    gb_plotting = new QGroupBox(tr("Plotting"));

    QGridLayout * hl_plotting0 = new QGridLayout();
    QHBoxLayout * hl_plotting1 = new QHBoxLayout();
    QHBoxLayout * hl_plotting2 = new QHBoxLayout();

    pb_plot = new QPushButton();
    pb_plot->setText("New Plot");
    pb_plot->setEnabled(false);

    cb_add_to_plot_prefix = new QLabel(this);
    cb_add_to_plot_prefix->setText(QString("     Add to Plot: "));
    cb_add_to_plot_prefix->setEnabled(false);

    cb_add_to_plot = new QComboBox();
    cb_add_to_plot->setEnabled(false);

    pb_add_to_plot = new QPushButton();
    pb_add_to_plot->setEnabled(false);
    pb_add_to_plot->setText("Add");

    hl_plotting1->addWidget(pb_plot);
    hl_plotting2->addWidget(cb_add_to_plot_prefix);
    hl_plotting2->addWidget(cb_add_to_plot);
    hl_plotting2->addWidget(pb_add_to_plot);
    hl_plotting2->addStretch(1);

    hl_plotting0->addLayout(hl_plotting1, 0, 0);
    hl_plotting0->addLayout(hl_plotting2, 0, 1);
    gb_plotting->setLayout(hl_plotting0);
    showPlotLayout->addWidget(gb_plotting);

    connect(pb_plot, SIGNAL(clicked()), this, SLOT(plot_button_clicked()));
    connect(pb_add_to_plot, SIGNAL(clicked()), this, SLOT(plot_button_add_clicked()));
}

void MainWindow::createComboBox()
{
    showComboBoxLayout = new QHBoxLayout();
    QGroupBox * gb_combobox = new QGroupBox(tr("Select time series"));
    QHBoxLayout * cb_layout = new QHBoxLayout();
    QHBoxLayout * cb_group = new QHBoxLayout();

    cb_par_loc = new QComboBox();
    cb_par_loc->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    cb_par_loc->setEnabled(false);
    cb_group->addWidget(cb_par_loc);

    QHBoxLayout * sp_group = new QHBoxLayout();
    layerLabelPrefix = new QLabel(tr("Layer"));
    layerLabelSuffix = new QLabel(tr("[0,0]"));
    this->layerLabelPrefix->setEnabled(false);
    this->layerLabelSuffix->setEnabled(false);

    sb_layer = new QSpinBox();
    sb_layer->setRange(0, 0);
    sb_layer->setEnabled(false);

    sp_group->addWidget(layerLabelPrefix);
    sp_group->addWidget(sb_layer);
    sp_group->addWidget(layerLabelSuffix);

    cb_layout->addLayout(cb_group);
    cb_layout->addLayout(sp_group);
    cb_layout->addStretch(1);

    gb_combobox->setLayout(cb_layout);
    showComboBoxLayout->addWidget(gb_combobox);

    connect(cb_par_loc, SIGNAL(activated(int)), this, SLOT(cb_clicked()));
}

void MainWindow::cb_clicked()
{
    int fil_index = lb_filenames->currentRow();
    TSFILE * tsfile = this->openedUgridFile[fil_index];
    updateListBoxes(tsfile);
}

void MainWindow::plot_button_clicked()
{
    int fil_index = lb_filenames->currentRow();
    TSFILE * tsfile = this->openedUgridFile[fil_index];

    QFileInfo fi(tsfile->fname);
    QString base_name = fi.fileName();

    pb_plot->setDefault(false);
    pb_plot->setAutoDefault(false);

    nr_plot += 1;

    // Compose window title
    QString txt;
    int cnt = 0;
    for (int i_par = 0; i_par < lb_parameters->count(); i_par++)
    {
        QListWidgetItem * item_par = lb_parameters->item(i_par);
        if (item_par->isSelected())
        {
            if (cnt == 0)
            {
                cnt += 1;
                txt = lb_parameters->item(i_par)->text();
            }
            else
            {
                txt = txt + QString(" - ") + lb_parameters->item(i_par)->text();
            }
        }
    }

    if (nr_plot == 0)
    {
        this->cplot = (TSPlot **)malloc(sizeof(TSPlot *) * (nr_plot + 1));
    }
    else
    {
        this->cplot = (TSPlot **)realloc(this->cplot, sizeof(TSPlot *) * (nr_plot + 1));
    }

    QWidget * qw = new QWidget();
    connect(qw, SIGNAL(destroyed()), this, SLOT(destroyed()));

    qw->setAttribute(Qt::WA_DeleteOnClose, true);
    this->cplot[nr_plot] = new TSPlot(qw, *icon, nr_plot);
    this->cplot[nr_plot]->add_to_plot(tsfile, lb_parameters, lb_locations, lb_times, cb_par_loc, sb_layer, pb_add_to_plot);
    this->cplot[nr_plot]->draw_plot(nr_plot, txt, base_name);

    update_cb_add_to_plot();

}
void MainWindow::update_cb_add_to_plot()
{
    char count[11];
    long cnt;

    cnt = 0;
    for (int i = 0; i < nr_plot + 1; i++)
    {
        if (this->cplot[i] != NULL)
        {
            cnt++;
        }
    }

    if (cnt <= 0)
    {
        cb_add_to_plot_prefix->setEnabled(false);
        cb_add_to_plot->setEnabled(false);
        cb_add_to_plot->clear();
        pb_add_to_plot->setEnabled(false);
    }
    else
    {
        cb_add_to_plot_prefix->setEnabled(true);
        cb_add_to_plot->setEnabled(true);
        pb_add_to_plot->setEnabled(true);
        cb_add_to_plot->clear();
        cnt = -1;
        for (int i = 0; i < nr_plot + 1; i++)
        {
            sprintf(count, "%d", i + 1);
            if (this->cplot[i] != NULL)
            {
                cb_add_to_plot->addItem(QString(count).trimmed());
                cnt++;
            }
        }
        cb_add_to_plot->setCurrentIndex(cnt);
    }
}
void MainWindow::destroyed()
{
    QObject* obj = sender();
    QWidget * objj;
    for (int i = 0; i < nr_plot+1; i++)
    {
        if (this->cplot[i] != NULL)
        {
            objj = this->cplot[i]->get_parent();
            if (obj == objj)
            {
                this->cplot[i] = NULL;
            }
        }
    }
    update_cb_add_to_plot();
}
void MainWindow::plot_button_add_clicked()
{
    int selected_plot = -1;

    int i_file = lb_filenames->currentRow();
    TSFILE * tsfile = this->openedUgridFile[i_file];

    QString text = cb_add_to_plot->currentText();
    selected_plot = text.toInt() - 1;
    if (selected_plot >= 0 && selected_plot <= nr_plot)
    {
        this->cplot[selected_plot]->add_to_plot(tsfile, lb_parameters, lb_locations, lb_times, cb_par_loc, sb_layer, pb_add_to_plot);
        this->cplot[selected_plot]->add_data_to_plot();
    }
}

void MainWindow::contextMenuFileListBox(const QPoint &pos)
{
    QPoint item = lb_filenames->mapToGlobal(pos);
    QMenu submenu;
    int nr_files = lb_filenames->count();
    if (nr_files == 0) { return; }

    int i_file = lb_filenames->indexAt(pos).row();
    TSFILE * tsfile = this->openedUgridFile[i_file];

    submenu.addAction("Toggle pre-selection of parameter(s) and locations(s)");
    submenu.addAction("Close and Delete file from list");
    QAction* rightClickItem = submenu.exec(item);
    if (rightClickItem && rightClickItem->text().contains("pre-selection"))
    {
        m_pre_selection = !m_pre_selection;
        updateListBoxes(tsfile);
    }
    if (rightClickItem && rightClickItem->text().contains("Delete"))
    {
        for (int j = i_file; j < nr_files-1; j++)
        {
            this->openedUgridFile[j] = this->openedUgridFile[j+1];
        }
        this->openedUgridFile[nr_files] = NULL;
        nr_files -= 1;
        fil_index -= 1;
        lb_filenames->blockSignals(true);
        lb_filenames->takeItem(i_file);
        lb_filenames->blockSignals(false);
        if (nr_files > 0)
        {
            lb_filenames->setCurrentRow(i_file);
            tsfile = this->openedUgridFile[i_file];
            long ierr = tsfile->read(this->pgBar);
            if (ierr == 0)
            {
                char count[11];
                QString * text1 = new QString("Filenames (count ");
                sprintf(count, "%d", lb_filenames->count());
                text1->append(count);
                text1->append(")");
                gb_filenames->setTitle(*text1);

                updateListBoxes(tsfile);
            }
            pgBar->setValue(1000);
            this->pgBar->hide();
        }
        else
        {
            // clean combobox, spinners, listboxes and meta data
            lb_filenames->setToolTip("");

            sb_layer->setRange(0, 0);
            sb_layer->setEnabled(false);
            this->layerLabelSuffix->setText(tr("[0, 0]"));

            updateFileListBox(NULL);
            updateListBoxes(NULL);
            this->cb_par_loc->setEnabled(false);
        }
    }
}
void MainWindow::close()
{
    int nr_files = lb_filenames->count();
    lb_filenames->blockSignals(true);
    for (int i_file = nr_files-1; i_file>=0; i_file--)
    {
        TSFILE * tsfile = this->openedUgridFile[i_file];
        delete tsfile;
        this->openedUgridFile[i_file] = NULL;
        lb_filenames->takeItem(i_file);
    }
    lb_filenames->setToolTip("");
    lb_filenames->blockSignals(false);
    // clean combobox, spinners, listboxes and meta data
    sb_layer->setRange(0, 0);
    sb_layer->setEnabled(false);
    this->layerLabelSuffix->setText(tr("[0, 0]"));

    updateFileListBox(NULL);
    updateListBoxes(NULL);
    this->cb_par_loc->setEnabled(false);

    // delete all plots
    for (int i_plot = 0; i_plot < nr_plot + 1; i_plot++)
    {
        TSPlot * tsplot = this->cplot[i_plot];
        delete tsplot;
        this->cplot[i_plot] = NULL;
    }


}
void MainWindow::exit()
{
    QApplication::closeAllWindows();
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    this->exit();
}

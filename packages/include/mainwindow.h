/****************************************************************************
**
** Copyright (C) 2004-2006 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** Licensees holding a valid Qt License Agreement may use this file in
** accordance with the rights, responsibilities and obligations
** contained therein.  Please consult your licensing agreement or
** contact sales@trolltech.com if any conditions of this licensing
** agreement are not clear to you.
**
** Further information about Qt licensing is available at:
** http://www.trolltech.com/products/qt/licensing.html or by
** contacting info@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//#include <boost/lexical_cast.hpp>
//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>

#include <QtCore/QString>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QFileInfo>
#include <QtCore/QModelIndex>
#include <QtCore/QStringList>

#include <QtGui/QAction>
#include <QtGui/QtGui>
#include <QtGui/QBrush>
#include <QtGui/QIcon>
#include <QtGui/QDrag>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QPaintDevice>
#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>

#include "cf_time_series.h"
#include "qcustomplot.h"
#include "time_series_plot.h"
#include "read_json.h"

#define    WAIT_MODE 1
#define NO_WAIT_MODE 0

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QDir, QDir);

protected:
    void dragEnterEvent(QDragEnterEvent *);
    void dropEvent(QDropEvent *);

public slots:
    void openFile(QFileInfo, FILE_TYPE);

private slots:
    void openFile();
    void closeFile();
    void ExportToCSV();
    void OpenPreSelection();
    void OpenPreSelection(TSFILE *, QFileInfo);
    void SavePreSelection();
    void about();
    void ShowUserManual();
    void close();
    void exit();
    void closeEvent(QCloseEvent *);

    void contextMenuFileListBox(const QPoint &);

    void lb_filenames_clicked(QListWidgetItem *);
    void lb_filenames_selection_changed();

    void lb_parameter_clicked(QListWidgetItem *);
    void lb_parameter_selection_changed();
    void lb_location_clicked(QListWidgetItem *);
    void lb_location_selection_changed();
    void lb_times_clicked(QListWidgetItem *);
    void lb_times_selection_changed();

    void plot_button_clicked();
    void plot_button_add_clicked();
    void cb_clicked(int);

    void destroyed();

private:
    void createActions();
    void createMenus();
    void createFilenameList();
    void createPLTList();
    void createParameterList();
    void createLocationList();
    void createTimeList();
    void createPlotButton();
    void createComboBox();
    void createDisplayMeta();
    void createStatusBar();


    void loadFile(const QString &fileName);
    void updateListBoxes(TSFILE *);
    void updateFileListBox(TSFILE *);
    void update_cb_add_to_plot(void);
    void DisplayFileName(char *, char *);
    char * get_basename(char *);
    int QT_SpawnProcess(int, char *, char **);

    struct _program_arguments * prg_arg;

    int nr_plot;
    int m_fil_index;
    bool m_pre_selection;

    TSPlot ** cplot;
    QCustomPlot * customPlot;
    TSFILE ** openedUgridFile;

//  QT Stuff
    QWidget * mainWidget;
    QProgressBar * pgBar;

    QComboBox * cb_par_loc;
    QSpinBox * sb_layer;
    QLabel * layerLabelPrefix;
    QLabel * layerLabelSuffix;

    QListWidget * lb_filenames;
    QListWidget * lb_parameters;
    QListWidget * lb_locations;
    QListWidget * lb_times;

//  Menubar item
    QMenu * fileMenu;
    QAction * openAct;
    QAction * closeAct;
    QAction * openPreSelect;
    QAction * savePreSelect;
    QAction * exitAct;

    QMenu * exportMenu;
    QAction * ecsvAct;
    
    QMenu * helpMenu;
    QAction * aboutAct;
    QAction * showUserManualAct;

//  Groupboxes
    QGroupBox   * gb_filenames;
    QGroupBox   * gb_parameters;
    QGroupBox   * gb_locations;
    QGroupBox   * gb_times;
    QGroupBox   * gb_plotting;

//  Push buttons
    QPushButton * pb_plot;
    QPushButton * pb_add_to_plot;
    QComboBox * cb_add_to_plot;
    QLabel * cb_add_to_plot_prefix;

// Global attributes data
    QTableView * table;
    QStandardItemModel * table_model;
};

#endif

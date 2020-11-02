
#include <QtWidgets/QApplication>

#include <string.h>
#include <stdio.h>
#if defined(WIN32) || defined(WIN64)
#  include <direct.h>
#else
#  include <unistd.h>
#endif

#if defined(WIN32) || defined(WIN64)
#  define strdup _strdup
#  define strcat strcat_s
#  define getcwd _getcwd
#  define FILE_NAME_MAX _MAX_PATH
#else
#  define FILE_NAME_MAX FILENAME_MAX
#endif

#include "mainwindow.h"
#include "cf_time_series.h"
#include "program_arguments.h"

char * get_dirname(char *);
void GetArguments(long, char **, struct _program_arguments *);
void set_theme(QApplication *, QDir);

struct _program_arguments m_prg_arg;

/*
* commandline arguments
* --ncfile    NetCDF file
*/

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, true);

    QDir exec_dir = QFileInfo(argv[0]).absolutePath();
    QDir startup_dir = QDir::currentPath(); // getcwd(current_dir, FILE_NAME_MAX);
    if (!exec_dir.exists())
    {
		exec_dir = startup_dir;
    }

    (void)GetArguments(argc, argv, &m_prg_arg);
    set_theme(&app, exec_dir);

    MainWindow * mainWin = new MainWindow(exec_dir, startup_dir);
    mainWin->setAcceptDrops(true);
    mainWin->show();

    app.processEvents();

    if (m_prg_arg.ncfile != nullptr && m_prg_arg.ncfile->exists() && m_prg_arg.type != UNKNOWN)
    {
        mainWin->openFile(*m_prg_arg.ncfile, m_prg_arg.type);
    }

    return app.exec();
}
//------------------------------------------------------------------------------
char * get_dirname(char * filename)
{
    char * str =  strdup (filename);
    size_t length = strlen(filename);
    for (size_t i=length; i<0;  i--)
    {
        if (str[i]=='\\' || str[i]=='/')
        {
            str[i] = '\0';
            return str;
        }
    }
    return str;
}
//------------------------------------------------------------------------------
/* @@ GetArguments
*
* Scan command-line arguments
*/
void GetArguments(long argc,                             /* I Number of command line arguments */
                  char ** argv,                          /* I Pointers to these arguments */
                  struct _program_arguments * prg_arg)   /* O Propeties structure */
                                                         /* Returns nothing */
{
    long  i;
    /*
    * Initialize the properties structure
    */
    prg_arg->ncfile = nullptr;
    prg_arg->type = HISTORY;

    i = 0;
    while (i < argc)
    {
        if (strcmp(argv[i], "--ncfile") == 0)
        {
            i = i + 1;
            if (i < argc) prg_arg->ncfile = new QFileInfo(argv[i]);
        }
        else if (i != 0)
        {
            i = argc;
        }
        i = i + 1;
    }
    return;
}
void set_theme(QApplication * app, QDir exec_dir)
{
    QPalette palette;
    long theme = 1;
    switch(theme)
    {
    case 1:
        {
            app->setStyle(QStyleFactory::create("Fusion"));
            QFile styleFile(exec_dir.absolutePath() + "/style.qss" );
            bool succes = styleFile.open(QFile::ReadOnly);
            if (succes)
            {
                QString style(styleFile.readAll());
                app->setStyleSheet(style);
            }
        }
        break;
    case 2:
        // Delft3D steelblue3 style
        app->setStyle(QStyleFactory::create("Fusion"));

        palette.setColor(QPalette::Window, QColor(79, 148, 205));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(79, 148, 205));
        palette.setColor(QPalette::AlternateBase, QColor(79, 148, 205));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(79, 148, 205));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Dark, QColor(35, 35, 35));
        palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
        palette.setColor(QPalette::Link, QColor(42, 130, 218));

        palette.setColor(QPalette::Highlight, QColor(131, 180, 220));
        palette.setColor(QPalette::HighlightedText, Qt::white);

        palette.setColor(QPalette::Disabled, QPalette::Button, QColor(63, 119, 165));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(55, 103, 143));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(55, 103, 143));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(55, 103, 143));
        palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));

        app->setPalette(palette);

        break;
    case 3:
        // Dark style
        app->setStyle(QStyleFactory::create("Fusion"));

        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(180, 180, 180));

        palette.setColor(QPalette::Highlight, QColor(180, 180, 180));
        palette.setColor(QPalette::HighlightedText, Qt::black);

        app->setPalette(palette);
        break;
    case 4:
        // Default windows style, experiment colour settings
        app->setStyle(QStyleFactory::create("WindowsXP"));

        palette.setColor(QPalette::Base, Qt::green);  // background listboxes, tables
        palette.setColor(QPalette::Background, Qt::white);
        palette.setColor(QPalette::Shadow, Qt::white);
        palette.setColor(QPalette::Window, Qt::yellow); // background window, groupbox lines changed
        palette.setColor(QPalette::WindowText, Qt::darkCyan);
        palette.setColor(QPalette::AlternateBase, Qt::white);
        palette.setColor(QPalette::ToolTipBase, Qt::blue);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::darkBlue);  // text on listbox, combo box, spinner
        palette.setColor(QPalette::Button, Qt::red);
        palette.setColor(QPalette::ButtonText, Qt::darkBlue); // text on button
        palette.setColor(QPalette::BrightText, Qt::white);

        palette.setColor(QPalette::Highlight, Qt::darkBlue); // highlighter in spinner
        palette.setColor(QPalette::HighlightedText, Qt::white); // highlighted text in spinner

        palette.setColor(QPalette::Disabled, QPalette::Base, QColor(0, 255, 0));
        palette.setColor(QPalette::Disabled, QPalette::Button, QColor(63, 119, 165));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(55, 103, 143));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(55, 103, 143));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(55, 103, 143));

        app->setPalette(palette);

        break;
    default:
        break;
    }
}
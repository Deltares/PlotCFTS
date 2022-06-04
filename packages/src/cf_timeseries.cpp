#include <iostream>
#include <fstream>
#include <iomanip>

#include <QtCore/QDateTime>
#include <QtCore/QDate>
#include <QtCore/QFileInfo>
#include <QtCore/QTime>
#include <QtCore/qtextcodec.h>
#include <QtCore/qdebug.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>

#include <QtGui/QIcon>

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

#include "cf_time_series.h"
#include "netcdf.h"

TSFILE::TSFILE(QFileInfo filename, FILE_TYPE ftype)
{
    this->tsfilefilename = (char *) malloc(sizeof(char) * (FILENAME_MAX + 1));

    this->fname = filename;
    this->type = ftype;
    this->tsfilefilename = strdup(this->fname.absoluteFilePath().toUtf8());
    m_nr_par_loc = 0;
    m_time_var_name = NULL;
    datetime_ntimes = 0;
    m_pre_selection = false;
    m_cb_parloc_index = -1;
}
TSFILE::~TSFILE()
{
    delete tsfilefilename;
    delete[] m_time_var_name;
    for (int i = 0; i < this->global_attributes->count; i++)
    {
        if (this->global_attributes->attribute[i]->name != NULL) free(this->global_attributes->attribute[i]->name); this->global_attributes->attribute[i]->name = NULL;
        if (this->global_attributes->attribute[i]->name != NULL) free(this->global_attributes->attribute[i]->cvalue); this->global_attributes->attribute[i]->cvalue = NULL;
        //char ** svalue;  // todo
        if (this->global_attributes->attribute[i]->ivalue != NULL) free(this->global_attributes->attribute[i]->ivalue); this->global_attributes->attribute[i]->ivalue = NULL;
        if (this->global_attributes->attribute[i]->rvalue != NULL) free(this->global_attributes->attribute[i]->rvalue); this->global_attributes->attribute[i]->rvalue = NULL;
        if (this->global_attributes->attribute[i]->dvalue != NULL) free(this->global_attributes->attribute[i]->dvalue); this->global_attributes->attribute[i]->dvalue = NULL;
    }
    this->global_attributes->count = 0;
    this->global_attributes = NULL;

    for (int i = 0; i < m_nr_par_loc; i++)
    {
        delete[] par_loc[i]->location_var_name;
        par_loc[i]->location_var_name = NULL;
        delete[] par_loc[i]->location_long_name;
        par_loc[i]->location_long_name = NULL;
        delete[] par_loc[i]->location_dim_name;
        par_loc[i]->location_dim_name = NULL;

        // delete parameters
        for (int j = 0; j < par_loc[i]->nr_parameters; j++)
        {
            for (int k = 0; k < par_loc[i]->parameter[j]->ndim; k++)
            {
                delete[] par_loc[i]->parameter[j]->dim_name[k];
                par_loc[i]->parameter[j]->dim_name[k] = NULL;
            }
            par_loc[i]->parameter[j]->dim_name = NULL;

            delete[] par_loc[i]->parameter[j]->name;
            par_loc[i]->parameter[j]->name = NULL;
            delete[] par_loc[i]->parameter[j]->unit;
            par_loc[i]->parameter[j]->unit = NULL;
        }
        delete par_loc[i]->parameter;
        par_loc[i]->nr_parameters = 0;

        // delete locations
        par_loc[i]->nr_locations = 0;
        // delete times (it is a list)
        for (auto iter = m_times.qdt_time.begin(); iter != m_times.qdt_time.end(); ++iter)
        {
            m_times.qdt_time.erase(iter);
        }
    }
    if (m_nr_par_loc != 0)
    {
        delete[] par_loc;
        par_loc = NULL;
        m_nr_par_loc = 0;
    }
}

long TSFILE::read(QProgressBar * pgBar)
{
    int status;

    pgBar->show();
    pgBar->setValue(0);
    status = nc_open(this->tsfilefilename, NC_NOWRITE, &this->m_ncid);
    if (status != NC_NOERR)
    {
        QMessageBox::warning(NULL, QObject::tr("Warning"), QObject::tr("Cannot open file: %1").arg(this->tsfilefilename));
        return 1;
    }
    pgBar->setValue(100);
    this->read_times(pgBar, 100, 600);
    pgBar->setValue(600);
    this->read_global_attributes();

    pgBar->setValue(700);
    if (this->type == HISTORY) {
        // read a history file
        this->read_locations();
        pgBar->setValue(800);
        this->read_parameters();
        pgBar->setValue(900);
    }
    else
    {
        QMessageBox::warning(NULL, QObject::tr("Warning"), QObject::tr("Only HISTORY is yet supported"));
    }
    if (m_time_var_name == NULL)
    {
        QMessageBox::warning(NULL, QObject::tr("Error"), QString("No time variable found in file:\n%1.\nFile will not be read.").arg(this->tsfilefilename));
        return 1;
    }
    status = nc_close(this->m_ncid);

    if (status != NC_NOERR) {
        // handle_error(status);
    }
    return 0;
}

void TSFILE::read_times(QProgressBar * pgBar, long pgBar_start, long pgBar_end)
{
    // Look for variable with unit like: "seconds since 2017-02-25 15:26:00"
    int ndims, nvars, natts, nunlimited;
    int status;
    char * var_name;
    int dimids;
    int nr_time_series = 0;
    double fraction;
    double dt = 0.0;

    pgBar->setValue(pgBar_start);

    // look for time variable
    std::string time_unit_string;
    std::string time_std_name;
    int time_var = -1;
    QStringList date_time;

    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i_var = 0; i_var < nvars; ++i_var)
    {
        int error_code = get_attribute(m_ncid, i_var, "units", &time_unit_string);
        if (error_code == 0 && time_unit_string.find("since") != -1)
        {
            QString units = QString::fromStdString(time_unit_string).replace("T", " ");  // "seconds since 1970-01-01T00:00:00" changed into "seconds since 1970-01-01 00:00:00"
            date_time = units.split(" ");
            if (date_time.at(1).toUtf8() == "since")
            {
                time_var = i_var;
            }
            error_code = get_attribute(m_ncid, i_var, "standard_name", &time_std_name);
            if (error_code == 0 && time_std_name == "time")
            {
                time_var = i_var;
                break;
            }
        }
    }

    var_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));

    // now it is the time variable, can only be detected by the "seconds since 1970-01-01T00:00:00" character string
    // retrieve the long_name, standard_name -> var_name for the xaxis label
    std::string label;
    status = nc_inq_var(this->m_ncid, time_var, var_name, NULL, &ndims, &dimids, &natts);
    status = get_attribute(this->m_ncid, time_var, "long_name", &label);
    if (status == NC_NOERR)
    {
        this->xaxis_label = QString::fromStdString(label);
    }
    else
    {
        this->xaxis_label = QString(var_name);
    }

    // retrieve the time series
    nr_time_series += 1;
    if (nr_time_series > 1)
    {
        QMessageBox::warning(NULL, QObject::tr("Warning"), QObject::tr("Support of just one time series, only the first one will be supported.\nPlease mail to jan.mooiman@deltares.nl"));
    }
    m_time_var_name = strdup(var_name);
    status = nc_inq_dimlen(this->m_ncid, dimids, &datetime_ntimes);
    // ex. date_time = "seconds since 2017-02-25 15:26:00"   year, yr, day, d, hour, hr, h, minute, min, second, sec, s and all plural forms
    datetime_units = date_time.at(0);

    QDate date = QDate::fromString(date_time.at(2), "yyyy-MM-dd");
    QTime time = QTime::fromString(date_time.at(3), "hh:mm:ss");
    this->RefDate = new QDateTime(date, time, Qt::UTC);
#if defined(DEBUG)
    QString janm1 = this->RefDate->toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString janm2 = this->RefDate->toUTC().toString("yyyy-MM-dd hh:mm:ss.zzz");
#endif
    double * times_c = (double *)malloc(sizeof(double)*datetime_ntimes);
    status = nc_get_var_double(this->m_ncid, time_var, times_c);
    for (int i = 0; i < datetime_ntimes; i++)
    {
        m_times.times.push_back(times_c[i]);
        m_times.pre_selected.push_back(0);  // not pre selected
    }
    free(times_c);
    times_c = nullptr;
    if (datetime_ntimes >= 2)
    {
        dt = m_times.times[1] - m_times.times[0];
    }
    fraction = double(pgBar_start);
    pgBar->setValue(int(fraction));

    if (datetime_units.contains("sec") ||
        datetime_units.trimmed() == "s")  // seconds, second, sec, s
    {
        this->xaxis_unit = QString("sec");
    }
    else if (datetime_units.contains("min"))  // minutes, minute, min
    {
        this->xaxis_unit = QString("min");
    }
    else if (datetime_units.contains("h"))  // hours, hour, hrs, hr, h
    {
        this->xaxis_unit = QString("hour");
    }
    else if (datetime_units.contains("d"))  // days, day, d
    {
        this->xaxis_unit = QString("day");
    }

    for (int j = 0; j < datetime_ntimes; j++)
    {
        if (fmod(j, int(0.01 * double(datetime_ntimes))) == 0)
        {
            fraction = double(pgBar_start) + double(j) / double(datetime_ntimes) * (double(pgBar_end) - double(pgBar_start));
            pgBar->setValue(int(fraction));
        }
        if (datetime_units.contains("sec") ||
            datetime_units.trimmed() == "s")  // seconds, second, sec, s
        {
            if (dt < 1.0)
            {
                m_times.qdt_time.append(this->RefDate->addMSecs(1000. * m_times.times[j]));  // milli seconds as smallest time unit
            }
            else
            {
                m_times.qdt_time.append(this->RefDate->addSecs(m_times.times[j]));  // seconds as smallest time unit
            }
        }
        else if (datetime_units.contains("min"))  // minutes, minute, min
        {
            m_times.times[j] = m_times.times[j] * 60.0;
            m_times.qdt_time.append(this->RefDate->addSecs(m_times.times[j]));
        }
        else if (datetime_units.contains("h"))  // hours, hour, hrs, hr, h
        {
            m_times.times[j] = m_times.times[j] * 3600.0;
            m_times.qdt_time.append(this->RefDate->addSecs(m_times.times[j]));
        }
        else if (datetime_units.contains("d"))  // days, day, d
        {
            m_times.times[j] = m_times.times[j] * 24.0 * 3600.0;
            m_times.qdt_time.append(this->RefDate->addSecs(m_times.times[j]));
        }
        // times[j] is now defined in seconds
#if defined (DEBUG)
        if (dt < 1.0)
        {
            QString janm = m_times.qdt_time[j].toString("yyyy-MM-dd hh:mm:ss.zzz");
        }
        else
        {
            QString janm = m_times.qdt_time[j].toString("yyyy-MM-dd hh:mm:ss");
        }
#endif
    }
    free(var_name); var_name = NULL;
    pgBar->setValue(pgBar_end);
}

long TSFILE::get_count_times()
{
    return (long) this->datetime_ntimes;
}
_time_series TSFILE::get_times()
{
    return this->m_times;
}
void TSFILE::put_times(_time_series times)
{
    this->m_times = times;
    return;
}

QDateTime * TSFILE::get_reference_date()
{
    return this->RefDate;
}
QList<QDateTime> TSFILE::get_qdt_times()  // qdt: Qt Date Time
{
    return this->m_times.qdt_time;
}


QString TSFILE::get_xaxis_label(void)
{
    return this->xaxis_label.trimmed() + " [" + this->xaxis_unit.trimmed() + " UTC]";
}
void TSFILE::read_parameters()
{
    // Read the parameters with dimension nr_times only (that is a time series without location, so model wide) This is a special case!
    // Read the parameters with coordinate as defined by the variables with attribute timeseries_id
    int ndims, nvars, natts, nunlimited;
    int status;
    size_t length;
    char * dim_name;
    char * var_name;
    char * parameter_name;
    char * unit;

    size_t * par_dim;
    long i_par_loc = -1;
    long i_param = -1;

    if (this->m_time_var_name == NULL)
    {
        return;
    }

    m_nr_parameters = -1;

    var_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    dim_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));

    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i_var = 0; i_var < nvars; i_var++)
    {
        long * par_dim_ids;  // parameter dimensions

        length = (size_t) -1;
        i_par_loc = -1;
        status = nc_inq_var(this->m_ncid, i_var, var_name, NULL, &ndims, NULL, &natts);

        par_dim = (size_t *)malloc(sizeof(long *)*ndims);
        par_dim_ids = (long *)malloc(sizeof(long *)*ndims);
        status = nc_inq_vardimid(this->m_ncid, i_var, (int*)par_dim_ids);

        // check on the time dimension
        bool is_time_series = false;
        for (int i = 0; i < ndims; i++)
        {
            status = nc_inq_dim(this->m_ncid, par_dim_ids[i], dim_name, &par_dim[i]);
            if (!strcmp(this->m_time_var_name, dim_name))
            {
                is_time_series = true;
                break;
            }
        }
        if (!is_time_series)
        {
            //there is no time-dimension, so this variable is not a time series
            continue;
        }

        std::string coord;
        status = get_attribute(this->m_ncid, i_var, "coordinates", &coord);

        if (status != NC_NOERR)
        {
            // Look for variables with (time, location) dimension, 
            // the location is determined by the cf_role="timeseries_id"
            int var_found = 0;
            for (int i = 0; i < ndims; i++)
            {
                status = nc_inq_dim(this->m_ncid, par_dim_ids[i], dim_name, &par_dim[i]);
                if (!strcmp(dim_name, this->m_time_var_name))
                {
                    var_found += 1;
                }
                for (int j = 0; j < m_nr_par_loc; j++)
                {
                    if (!strcmp(dim_name, this->par_loc[j]->location_dim_name))
                    {
                        coord = this->par_loc[j]->location_var_name;
                        var_found += 1;
                    }
                }
                status = -1;
                if (var_found == 2)
                { 
                    status = NC_NOERR;
                }
            }
        }
        if (status == NC_NOERR)
        {
            for (int i = 0; i < m_nr_par_loc; i++)
            {
                std::string crd(this->par_loc[i]->location_var_name);
                if (coord.find(crd) != -1)
                {
                    i_par_loc = i;
                    i_param = this->par_loc[i_par_loc]->nr_parameters;
                    this->par_loc[i_par_loc]->nr_parameters += 1;
                    m_nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
                    break;
                }
            }
            if (i_par_loc == -1 ||  i_param == -1)
            {
                continue;
            }

            // add this parameter
            status = nc_inq_var(this->m_ncid, i_var, var_name, NULL, &ndims, NULL, &natts);
            this->par_loc[i_par_loc]->ndim = ndims;
            m_nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
            ensure_capacity_parameters(i_par_loc, ndims);

            this->par_loc[i_par_loc]->parameter[i_param]->ndim = ndims;
            for (long j = 0; j < ndims; j++)
            {
                status = nc_inq_dim(this->m_ncid, par_dim_ids[j], dim_name, &par_dim[j]);
                this->par_loc[i_par_loc]->parameter[i_param]->dim_id[j] = par_dim_ids[j];
                this->par_loc[i_par_loc]->parameter[i_param]->dim_val[j] = (long)par_dim[j];
                this->par_loc[i_par_loc]->parameter[i_param]->dim_name[j] = strdup(dim_name);
            }
            length = (size_t) -1;
            status = nc_inq_attlen(this->m_ncid, i_var, "long_name", &length);
            if (status == NC_NOERR)
            {
                std::string tmp;
                status = get_attribute(this->m_ncid, i_var, "long_name", &tmp);
                parameter_name = strdup(tmp.c_str());
            }
            else
            {
                parameter_name = strdup(var_name);
            }
            m_yaxis_label = QString(var_name);

            this->par_loc[i_par_loc]->parameter[i_param]->name = strdup(parameter_name);
            status = nc_inq_attlen(this->m_ncid, i_var, "units", &length);
            if (status == NC_NOERR)
            {
                std::string tmp;
                status = get_attribute(this->m_ncid, i_var, "units", &tmp);
                this->par_loc[i_par_loc]->parameter[i_param]->unit = strdup(tmp.c_str());
            }
            else
            {
                this->par_loc[i_par_loc]->parameter[i_param]->unit = strdup("?");
            }
            free(parameter_name); parameter_name = NULL;
        }
        else
        {
            // no coordinates attribute found, so search for variables with only the time-dimension and add the variable to the parameter list for location "Model wide"
            status = nc_inq_var(this->m_ncid, i_var, var_name, NULL, &ndims, NULL, &natts);
            status = nc_inq_dim(this->m_ncid, par_dim_ids[0], dim_name, &par_dim[0]);  // just one dimension, so index == 0
            if (ndims == 1 && !strcmp(m_time_var_name, dim_name)) // The dimension is the 'time' dimension
            {
                for (int i = 0; i < m_nr_par_loc; i++)
                {
                    char * crd = strdup(this->par_loc[i]->location_var_name);
                    if (strstr("model_wide", (const char *)crd))
                    {
                        i_par_loc = i;
                        i_param = this->par_loc[i_par_loc]->nr_parameters;
                        this->par_loc[i_par_loc]->nr_parameters += 1;
                        m_nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
                        delete crd;
                        break;
                    }
                    delete crd;
                }
                if (i_par_loc == -1 || i_param == -1)
                {
                    continue;
                }
                // add this variable to the parameter list
                this->par_loc[i_par_loc]->ndim = ndims;
                ensure_capacity_parameters(i_par_loc, ndims);

                this->par_loc[i_par_loc]->parameter[i_param]->ndim = ndims;
                this->par_loc[i_par_loc]->parameter[i_param]->dim_id[0] = par_dim_ids[0];
                this->par_loc[i_par_loc]->parameter[i_param]->dim_val[0] = (long)par_dim[0];
                this->par_loc[i_par_loc]->parameter[i_param]->dim_name[0] = strdup(dim_name);

                // because the coordinates attribute was not found; try long_name else use var_name
                length = (size_t) -1;
                status = nc_inq_attlen(this->m_ncid, i_var, "long_name", &length);
                if (status == NC_NOERR)
                {
                    char * parameter_name_c = (char *)malloc(sizeof(char) * (length + 1));
                    status = nc_get_att(this->m_ncid, i_var, "long_name", parameter_name_c);
                    parameter_name_c[length] = '\0';
                    parameter_name = strdup(trim(parameter_name_c));
                    free(parameter_name_c);
                    parameter_name_c = nullptr;
                }
                else
                {
                    parameter_name = strdup(var_name);
                }
                m_yaxis_label = QString(var_name);

                this->par_loc[i_par_loc]->parameter[i_param]->name = strdup(parameter_name);
                status = nc_inq_attlen(this->m_ncid, i_var, "units", &length);
                if (status == NC_NOERR)
                {
                    unit = (char *)malloc(sizeof(char) * (length + 1));
                    status = nc_get_att(this->m_ncid, i_var, "units", unit);
                    unit[length] = '\0';
                    this->par_loc[i_par_loc]->parameter[i_param]->unit = strdup(unit);
                    free(unit); unit = nullptr;
                }
                else
                {
                    this->par_loc[i_par_loc]->parameter[i_param]->unit = strdup("?");
                }
            }
            else if (ndims == 2)
            {
                int accepted = 0;
                int i_par_dim = 0;
                status = nc_inq_var(this->m_ncid, i_var, var_name, NULL, &ndims, NULL, &natts);
                for (int i = 0; i < ndims; i++)
                {
                    status = nc_inq_dim(this->m_ncid, par_dim_ids[i], dim_name, &par_dim[i]);

                    if (!strcmp(this->m_time_var_name, dim_name))
                    {
                        accepted += 1;
                    }
                    else if (par_dim[i] == 1)
                    {
                        accepted += 1;
                        i_par_dim = i;
                    }
                }
                if (accepted == 2)
                {
                    ndims = 1; // second dimension is 1, so consider only the first dimension
                    for (int i = 0; i < m_nr_par_loc; i++)
                    {
                        char * crd = strdup(this->par_loc[i]->location_var_name);
                        if (strstr("model_wide", (const char *)crd))
                        {
                            i_par_loc = i;
                            i_param = this->par_loc[i_par_loc]->nr_parameters;
                            this->par_loc[i_par_loc]->nr_parameters += 1;
                            m_nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
                            delete crd;
                            break;
                        }
                        delete crd;
                    }
                    if (i_par_loc == -1 || i_param == -1)
                    {
                        continue;
                    }
                    // add this variable to the parameter list
                    this->par_loc[i_par_loc]->ndim = ndims;
                    ensure_capacity_parameters(i_par_loc, ndims);

                    this->par_loc[i_par_loc]->parameter[i_param]->ndim = ndims;
                    this->par_loc[i_par_loc]->parameter[i_param]->dim_id[0] = par_dim_ids[i_par_dim];
                    this->par_loc[i_par_loc]->parameter[i_param]->dim_val[0] = (long)par_dim[i_par_dim];
                    this->par_loc[i_par_loc]->parameter[i_param]->dim_name[0] = strdup(dim_name);

                    // because the coordinates attribute was not found; try long_name else use var_name
                    length = (size_t) -1;
                    status = nc_inq_attlen(this->m_ncid, i_var, "long_name", &length);
                    if (status == NC_NOERR)
                    {
                        char * parameter_name_c = (char *)malloc(sizeof(char) * (length + 1));
                        status = nc_get_att(this->m_ncid, i_var, "long_name", parameter_name_c);
                        parameter_name_c[length] = '\0';
                        parameter_name = trim(parameter_name_c);
                        free(parameter_name_c);
                        parameter_name_c = nullptr;
                    }
                    else
                    {
                        parameter_name = strdup(var_name);
                    }

                    this->par_loc[i_par_loc]->parameter[i_param]->name = strdup(parameter_name);
                    status = nc_inq_attlen(this->m_ncid, i_var, "units", &length);
                    if (status == NC_NOERR)
                    {
                        unit = (char *)malloc(sizeof(char) * (length + 1));
                        status = nc_get_att(this->m_ncid, i_var, "units", unit);
                        unit[length] = '\0';
                        this->par_loc[i_par_loc]->parameter[i_param]->unit = strdup(unit);
                        free(unit); unit = nullptr;
                    }
                    else
                    {
                        this->par_loc[i_par_loc]->parameter[i_param]->unit = strdup("?");
                    }
                }
            }
        }
        free(par_dim); par_dim = NULL;
        free(par_dim_ids); par_dim_ids = NULL;
    }
    free(dim_name); dim_name = NULL;
    free(var_name); var_name = NULL;
}
long TSFILE::get_count_parameters(long i_par_loc)
{
    if (this->par_loc[i_par_loc]->nr_parameters == 0)
    {
        QMessageBox::warning(NULL, QObject::tr("Warning"), QObject::tr("No parameters found in file: %1").arg(this->tsfilefilename));
    }
    return this->par_loc[i_par_loc]->nr_parameters;
}
struct _parameter * TSFILE::get_parameters(long i_par_loc)
{
    struct _parameter * param;
    m_nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
    param = (struct _parameter *) malloc(sizeof(struct _parameter) * m_nr_parameters);
    for (long i = 0; i < m_nr_parameters; i++)
    {
        param[i].name = strdup(this->par_loc[i_par_loc]->parameter[i]->name);
        param[i].unit = strdup(this->par_loc[i_par_loc]->parameter[i]->unit);
        param[i].ndim = this->par_loc[i_par_loc]->parameter[i]->ndim;
        param[i].dim_id = (long *)malloc(sizeof(long) * param[i].ndim);
        param[i].dim_val = (long *)malloc(sizeof(long) * param[i].ndim);
        param[i].dim_name = (char **)malloc(sizeof(char *) * param[i].ndim);
        for (long j = 0; j < param[i].ndim; j++)
        {
            param[i].dim_id[j] = this->par_loc[i_par_loc]->parameter[i]->dim_id[j];
            param[i].dim_val[j] = this->par_loc[i_par_loc]->parameter[i]->dim_val[j];
            param[i].dim_name[j] = strdup(this->par_loc[i_par_loc]->parameter[i]->dim_name[j]);
        }
        param[i].pre_selected = this->par_loc[i_par_loc]->parameter[i]->pre_selected;
    }
    return param;
}

long TSFILE::put_parameter(long i_par_loc, long i, struct _parameter * param)
{
    long status = 0;

    this->par_loc[i_par_loc]->parameter[i]->name = strdup(param[i].name);
    this->par_loc[i_par_loc]->parameter[i]->unit = strdup(param[i].unit);
    this->par_loc[i_par_loc]->parameter[i]->ndim = param[i].ndim;
    for (long j = 0; j < param[i].ndim; j++)
    {
        this->par_loc[i_par_loc]->parameter[i]->dim_id[j] = param[i].dim_id[j];
        this->par_loc[i_par_loc]->parameter[i]->dim_val[j] = param[i].dim_val[j];
        this->par_loc[i_par_loc]->parameter[i]->dim_name[j] = strdup(param[i].dim_name[j]);
    }
    this->par_loc[i_par_loc]->parameter[i]->pre_selected = param[i].pre_selected;

    return status;
}

void TSFILE::read_locations()
{
    // look for variable with cf_role == timeseries_id, that are the locations
    int ndims, nvars, natts, nunlimited;
    long status;
    long location_name_varid;
    char * cf_role;
    char * dim_name;
    char * var_name;
    size_t length;
    size_t mem_length;
    long i_par_loc = -1;
    long * sn_dims; // station names dimensions
    bool model_wide_found = false;
    bool model_wide_exist = false;

    if (this->m_time_var_name == NULL)
    {
        return;
    }

    m_nr_par_loc = 0;

    dim_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    var_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));

    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    // cf_role = timeseries_id
    for (long i_var = 0; i_var < nvars; i_var++)
    {
        length = (size_t) -1;
        status = nc_inq_attlen(this->m_ncid, i_var, "cf_role", &length);
        if (status == NC_NOERR)
        {
            cf_role = (char *)malloc(sizeof(char) * (length + 1));
            status = nc_get_att(this->m_ncid, i_var, "cf_role", cf_role);
            cf_role[length] = '\0';

            if (!strcmp(cf_role, "timeseries_id"))
            {
                nc_type nc_type;
                location_name_varid = i_var;
                status = nc_inq_var(this->m_ncid, location_name_varid, var_name, &nc_type, &ndims, NULL, &natts);
                for (int i = 0; i < m_nr_par_loc; i++)
                {
                    if (!strcmp(this->par_loc[i]->location_var_name, var_name))
                    {
                        model_wide_found = true;  // voeg geen nieuwe stations toe, want model_wide zit er al in.
                        i_par_loc = i;
                        break;
                    }
                    if (!strcmp(this->par_loc[i]->location_var_name, "model_wide"))
                    {
                        // current var_name is not model_wide, if there is a model_wide location swap it to the last par_loc index
                        model_wide_exist = true;
                        i_par_loc = i;
                    }
                }

                if (!model_wide_found)
                {
                    m_nr_par_loc += 1;
                    ensure_capacity_par_loc(m_nr_par_loc);
                    if (model_wide_exist)
                    {
                        struct _par_loc * tmp = this->par_loc[i_par_loc];
                        this->par_loc[i_par_loc] = this->par_loc[m_nr_par_loc - 1];
                        this->par_loc[m_nr_par_loc - 1] = tmp;
                    }
                    else
                    {
                        i_par_loc = m_nr_par_loc - 1;
                    }
                }
                this->par_loc[i_par_loc]->location_var_name = strdup(var_name);
                length = (size_t) -1;
                status = nc_inq_attlen(this->m_ncid, location_name_varid, "long_name", &length);
                if (status == NC_NOERR)
                {
                    char * long_name = (char *)malloc(sizeof(char) * (length + 1));
                    status = nc_get_att(this->m_ncid, location_name_varid, "long_name", long_name);
                    long_name[length] = '\0';
                    this->par_loc[i_par_loc]->location_long_name = strdup(long_name);
                    //free(long_name);
                }
                else
                {
                    this->par_loc[i_par_loc]->location_long_name = strdup(var_name);
                }

                sn_dims = (long *)malloc(sizeof(long *)*ndims);
                status = nc_inq_vardimid(this->m_ncid, location_name_varid, (int*)sn_dims);

                mem_length = 1;
                name_len = (size_t) -1;
                for (long j = 0; j < ndims; j++)
                {
                    length = (size_t) -1;
                    status = nc_inq_dim(this->m_ncid, sn_dims[j], dim_name, &length);

                    if (strstr(dim_name, "len") || name_len == -1 && j == 1)  // second dimension is the string length if not already set
                    {
                        name_len = length;
                    }
                    else
                    {
                        this->par_loc[i_par_loc]->location_dim_name = strdup(dim_name);
                        this->m_nr_locations = (long)length;
                    }
                    mem_length = mem_length * length;
                }
                ensure_capacity_locations(i_par_loc, m_nr_locations);
                // reading 64 strings for each location, length of string??
                if (nc_type == NC_STRING)
                {
                    char ** location_strings = (char **)malloc(sizeof(char *) * (mem_length)+1);
                    status = nc_get_var_string(this->m_ncid, location_name_varid, location_strings);
                    QString janm = QString("JanM");
                    for (int k = 0; k < m_nr_locations; k++)
                    {
                        janm = QString("");
                        for (int k2 = 0; k2 < name_len; k2++)
                        {
                            QTextCodec *codec2 = QTextCodec::codecForName("UTF-8");
                            QString str = codec2->toUnicode(*(location_strings + k * name_len + k2));
                            janm = janm + str;
                        }
                        this->par_loc[i_par_loc]->location[k]->name = new QString(janm);
                    }
                    free(location_strings);
                }
                else if (nc_type == NC_CHAR)
                {
                    char * location_chars = (char *)malloc(sizeof(char *) * (mem_length)+1);
                    status = nc_get_var_text(this->m_ncid, location_name_varid, location_chars);
                    char * janm = (char *)malloc(sizeof(char)*(name_len + 1));
                    janm[name_len] = '\0';
                    for (int k = 0; k < m_nr_locations; k++)
                    {
                        (void)strncpy_s(janm, name_len + 1, location_chars + k * name_len, name_len);
                        this->par_loc[i_par_loc]->location[k]->name = new QString(janm);
                    }
                    free(janm);
                    janm = nullptr;
                    free(location_chars);
                    location_chars = nullptr;
                }
                else
                {
                    // trying to read unsupported variable
                }

                this->par_loc[i_par_loc]->nr_locations = this->m_nr_locations;
                free(sn_dims);
            }
            free(cf_role);
        }
        else
        {
            // no cf_role == timeseries_id found, so search for variables with only the timedimension and add the location with name "model wide"
            model_wide_found = false;
            location_name_varid = i_var;
            status = nc_inq_var(this->m_ncid, location_name_varid, var_name, NULL, &ndims, NULL, &natts);
            if (ndims == 1)
            {
                sn_dims = (long *)malloc(sizeof(long)*ndims);
                status = nc_inq_vardimid(this->m_ncid, location_name_varid, (int*)sn_dims);
                status = nc_inq_dim(this->m_ncid, sn_dims[0], dim_name, &length);
                if (!strcmp(this->m_time_var_name, dim_name))
                {
                    for (int i = 0; i < m_nr_par_loc; i++)
                    {
                        if (!strcmp(this->par_loc[i]->location_var_name, "model_wide"))
                        {
                            model_wide_found = true;
                        }
                    }
                    if (!model_wide_found)
                    {
                        m_nr_par_loc += 1;
                        i_par_loc = m_nr_par_loc - 1;
                        ensure_capacity_par_loc(m_nr_par_loc);
                        this->par_loc[i_par_loc]->location_var_name = strdup("model_wide");
                        this->par_loc[i_par_loc]->location_long_name = strdup("Model wide");
                        this->par_loc[i_par_loc]->location_dim_name = strdup("");
                        this->par_loc[i_par_loc]->nr_locations = 1;
                        ensure_capacity_locations(i_par_loc, 1);
                        this->par_loc[i_par_loc]->location[0]->name = new QString("Model wide");
                    }
                }
                free(sn_dims);
            }
            else if (ndims == 2)  //two dimensions, one is the time and the other has dimesion 1.
            {
                int accepted = 0;
                sn_dims = (long *)malloc(sizeof(long *)*ndims);
                status = nc_inq_vardimid(this->m_ncid, location_name_varid, (int*)sn_dims);
                for (int i = 0; i < ndims; i++)
                {
                    status = nc_inq_dim(this->m_ncid, sn_dims[i], dim_name, &length);

                    if (!strcmp(this->m_time_var_name, dim_name))
                    {
                        accepted += 1;
                    }
                    else if (length == 1 && ndims == 1)
                    {
                        accepted += 1;
                    }
                }
                if (accepted == 2)
                {
                    for (int i = 0; i < m_nr_par_loc; i++)
                    {
                        if (!strcmp(this->par_loc[i]->location_var_name, "model_wide"))
                        {
                            model_wide_found = true;
                        }
                    }
                    if (!model_wide_found)
                    {
                        m_nr_par_loc += 1;
                        i_par_loc = m_nr_par_loc - 1;
                        ensure_capacity_par_loc(m_nr_par_loc);
                        this->par_loc[i_par_loc]->location_var_name = strdup("model_wide");
                        this->par_loc[i_par_loc]->location_long_name = strdup("Model wide");
                        this->par_loc[i_par_loc]->nr_locations = 1;
                        this->par_loc[i_par_loc]->location[0]->name = new QString("Model wide");
                    }
                }
                free(sn_dims);
            }
        }
    }
    free(dim_name);
    free(var_name);
}

long TSFILE::get_count_par_loc()
{
    return this->m_nr_par_loc;
}
char * TSFILE::get_par_loc_long_name(long i_par_loc)
{
    return this->par_loc[i_par_loc]->location_long_name;
}

long TSFILE::get_count_locations(long i_par_loc)
{
    return this->par_loc[i_par_loc]->nr_locations;
}

struct _location *  TSFILE::get_locations(long i_par_loc)
{
    struct _location * location;
    long nr_loc = this->par_loc[i_par_loc]->nr_locations;
    location = (struct _location *) malloc(sizeof(struct _location) * nr_loc);
    for (long i = 0; i < nr_loc; i++)
    {
        location[i].name = this->par_loc[i_par_loc]->location[i]->name;
        location[i].pre_selected = this->par_loc[i_par_loc]->location[i]->pre_selected;
    }

    return location;
}

long TSFILE::put_location(long i_par_loc, long i, struct _location * location)
{
    long status = 0;
    this->par_loc[i_par_loc]->location[i]->name = location[i].name;
    this->par_loc[i_par_loc]->location[i]->pre_selected = location[i].pre_selected;

    return status;
}

void TSFILE::read_global_attributes()
{
    long status;
    int ndims, nvars, natts, nunlimited;
    nc_type att_type;
    size_t att_length;

    char * att_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    att_name_c[0] = '\0';

    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);

    this->global_attributes = (_global_attributes *) malloc(sizeof(_global_attribute));
    this->global_attributes->count = natts;
    this->global_attributes->attribute = (_global_attribute **) malloc(sizeof(_global_attribute *) * natts);
    for (long i = 0; i < natts; i++)
    {
        this->global_attributes->attribute[i] = (_global_attribute *)malloc(sizeof(_global_attribute));
    }

    for (long i = 0; i < natts; i++)
    {
        status = nc_inq_attname(this->m_ncid, NC_GLOBAL, i, att_name_c);
        status = nc_inq_att(this->m_ncid, NC_GLOBAL, att_name_c, &att_type, &att_length);
        this->global_attributes->attribute[i]->name = strdup(att_name_c);
        this->global_attributes->attribute[i]->type = NC_CHAR;
        this->global_attributes->attribute[i]->length = (int) att_length;
        this->global_attributes->attribute[i]->cvalue = NULL;
        //global_attributes->attribute[i]->svalue = NULL;
        this->global_attributes->attribute[i]->ivalue = NULL;
        this->global_attributes->attribute[i]->rvalue = NULL;
        this->global_attributes->attribute[i]->dvalue = NULL;
        if (att_type == NC_CHAR)
        {
            char * att_value_c = (char *)malloc(sizeof(char) * (att_length + 1));
            att_value_c[0] = '\0';
            status = nc_get_att_text(this->m_ncid, NC_GLOBAL, att_name_c, att_value_c);
            att_value_c[att_length] = '\0';
            this->global_attributes->attribute[i]->cvalue = strdup(att_value_c);
            free(att_value_c);
            att_value_c = nullptr;
        }
    }
}
struct _global_attributes * TSFILE::get_global_attributes()
{
    return this->global_attributes;
}

std::vector<double> TSFILE::get_time_series(long cb_index, char * param_name, long loc_id, long layer_id)
{
    int ndims, nvars, natts, nunlimited;
    nc_type nc_type;
    long status;
    long par_id = -1;
    long j;
    long nr_loc;
    int nr_layers;
    size_t length;
    double * y_array = nullptr;
    size_t mem_length;
    char * dim_name;
    char * var_name;
    char * tmp_name;

    long * var_dims; // dimensions of variable
    dim_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    var_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));

    status = nc_open(this->tsfilefilename, NC_NOWRITE, &this->m_ncid);
    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i = 0; i < nvars; i++)
    {
        // look for the var_id of the nc_variable with long_name "parameter"
        length = (size_t) -1;
        status = nc_inq_attlen(this->m_ncid, i, "long_name", &length);
        if (status == NC_NOERR)
        {
            tmp_name = (char *)malloc(sizeof(char) * (length + 1));
            tmp_name[length] = '\0';
            status = nc_get_att(this->m_ncid, i, "long_name", tmp_name);
            tmp_name = trim(tmp_name);
            if (!strcmp(param_name, tmp_name))
            {
                par_id = i;
                break;
            }
            free(tmp_name); tmp_name = NULL;
        }
        else
        {
            // try var_name
            status = nc_inq_var(this->m_ncid, i, var_name, NULL, NULL, NULL, NULL);
            if (!strcmp(param_name, var_name))
            {
                par_id = i;
                break;
            }
        }
    }

    // retrieve the data belonging to the parameter variable
    status = nc_inq_var(this->m_ncid, par_id, var_name, &nc_type, &ndims, NULL, &natts);

    var_dims = (long *)malloc(sizeof(long) * ndims);
    status = nc_inq_vardimid(this->m_ncid, par_id, (int *)var_dims);

    nr_loc = this->get_count_locations(cb_index);
    nr_layers = 1;
    
    mem_length = 1;
    for (int i = 0; i < ndims; i++)
    {
        status = nc_inq_dim(this->m_ncid, var_dims[i], dim_name, &length);
        if (i == 2) nr_layers = (int) length;  // third dimension is the layer
        mem_length = mem_length * length;
    }

    if (nc_type == NC_DOUBLE)
    {
        double att_value;
        status = nc_get_att_double(this->m_ncid, par_id, "_FillValue", &att_value);

        y_array = (double*)malloc(sizeof(double) * mem_length);
        status = nc_get_var_double(this->m_ncid, par_id, y_array);
        for (int i = 0; i < mem_length; ++i)
        {
            if (y_array[i] == att_value) { y_array[i] = std::numeric_limits<double>::quiet_NaN(); }
        }
    }
    else if (nc_type == NC_FLOAT)
    {
        float att_value;
        status = nc_get_att_float(this->m_ncid, par_id, "_FillValue", &att_value);

        float * y_array_s = (float *)malloc(sizeof(float) * mem_length);
        status = nc_get_var_float(this->m_ncid, par_id, y_array_s);
        y_array = (double*)malloc(sizeof(double) * mem_length);
        for (int i = 0; i < mem_length; ++i)
        {
            if (y_array_s[i] == att_value) { y_array_s[i] = std::numeric_limits<float>::quiet_NaN(); }
            y_array[i] = (double)y_array_s[i];
        }
        free(y_array_s);
        y_array_s = nullptr;
    }
    else if (nc_type == NC_INT)
    {
        int att_value;
        status = nc_get_att_int(this->m_ncid, par_id, "_FillValue", &att_value);

        int* y_array_s = (int*)malloc(sizeof(int) * mem_length);
        status = nc_get_var_int(this->m_ncid, par_id, y_array_s);
        y_array = (double*)malloc(sizeof(double) * mem_length);
        for (int i = 0; i < mem_length; ++i)
        {
            if (y_array_s[i] == att_value) { y_array_s[i] = std::numeric_limits<int>::quiet_NaN(); }
            y_array[i] = (double)y_array_s[i];
        }
        free(y_array_s);
        y_array_s = nullptr;
    }
    else
    {
        QMessageBox::warning(NULL, QObject::tr("Warning"), QString("Data type \"%1\" not supported").arg(nc_type));
        std::vector<double> tmp;
        return tmp;
    }
    //select colum loc_id from variable "parameter"
    j = -1;
    long ii_layer = fmax(0, layer_id);
    m_y_values.clear();
    m_y_values.reserve(this->datetime_ntimes);
    int ii;
    for (int i_times = 0; i_times < this->datetime_ntimes; i_times++)
    {
        j = j + 1;
        ii = ii_layer + nr_layers * loc_id + nr_layers * nr_loc * i_times;
        m_y_values.push_back(y_array[ii]);
    }

    free(y_array); y_array = nullptr;
    free(var_dims); var_dims= nullptr;
    free(dim_name); dim_name = nullptr;
    free(var_name); var_name = nullptr;

    status = nc_close(this->m_ncid);
    if (status != NC_NOERR) {
        // handle_error(status);
    }
    return m_y_values;
}

bool TSFILE::get_pre_selection()
{
    return this->m_pre_selection;
}
void TSFILE::put_pre_selection(bool selection)
{
    this->m_pre_selection = selection;
}

int TSFILE::get_cb_parloc_index()
{
    return this->m_cb_parloc_index;
}
void TSFILE::put_cb_parloc_index(int selection)
{
    this->m_cb_parloc_index = selection;
}

void TSFILE::ensure_capacity_par_loc(long nr_par_loc)
{
    long i_par_loc = nr_par_loc-1;
    if (nr_par_loc == 1)
    {
        // make room for the first set of parameters and locations
        this->par_loc = (struct _par_loc **)malloc(sizeof(struct _par_loc *) * (i_par_loc + 1));
    }
    else
    {
        this->par_loc = (struct _par_loc **)realloc(this->par_loc, sizeof(struct _par_loc *) * (i_par_loc + 1));
    }
    this->par_loc[i_par_loc] = (struct _par_loc *) malloc(sizeof(struct _par_loc) * 1);
    this->par_loc[i_par_loc]->location_var_name = NULL;
    this->par_loc[i_par_loc]->location_long_name = NULL;
    this->par_loc[i_par_loc]->location_dim_name = NULL;
    this->par_loc[i_par_loc]->parameter = NULL;
    this->par_loc[i_par_loc]->location = NULL;
    this->par_loc[i_par_loc]->ndim = 0;
    this->par_loc[i_par_loc]->nr_parameters = 0;
    this->par_loc[i_par_loc]->nr_locations = 0;
    this->par_loc[i_par_loc]->pre_selection = false;
}
void TSFILE::ensure_capacity_parameters(long i_par_loc, long ndims)
{
    long i_param;
    if (m_nr_parameters == 1)
    {
        this->par_loc[i_par_loc]->parameter = (struct _parameter **) malloc(sizeof(struct _parameter *) * m_nr_parameters);
    }
    else if (m_nr_parameters > 1)
    {
        this->par_loc[i_par_loc]->parameter = (struct _parameter **) realloc(this->par_loc[i_par_loc]->parameter, sizeof(struct _parameter *) * m_nr_parameters);
    }
    i_param = this->par_loc[i_par_loc]->nr_parameters - 1;
    this->par_loc[i_par_loc]->parameter[i_param] = (struct _parameter *) malloc(sizeof(struct _parameter) * 1);
    this->par_loc[i_par_loc]->parameter[i_param]->dim_id = (long *)malloc(sizeof(long *) * ndims);
    this->par_loc[i_par_loc]->parameter[i_param]->dim_val = (long *)malloc(sizeof(long *) * ndims);
    this->par_loc[i_par_loc]->parameter[i_param]->dim_name = (char **)malloc(sizeof(char *) * ndims);
    //this->par_loc[i_par_loc]->parameter[i_param]->pre_selected = 0;
}
void TSFILE::ensure_capacity_locations(long i_par_loc, long nr_locations)
{
    this->par_loc[i_par_loc]->location = (struct _location **) malloc(sizeof(struct _location *) * nr_locations);
    for (int i = 0; i < nr_locations; i++)
    {
        this->par_loc[i_par_loc]->location[i] = (struct _location *) malloc(sizeof(struct _location) * 1);
        this->par_loc[i_par_loc]->location[i]->name  = new QString("");
        //this->par_loc[i_par_loc]->location[i]->pre_selected = 0;
    }
}

int TSFILE::get_attribute(int ncid, int i_var, char* att_name, char** att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(ncid, i_var, att_name, &length);
    *att_value = (char*)malloc(sizeof(char) * (length + 1));
    *att_value[0] = '\0';
    if (status != NC_NOERR)
    {
        *att_value = '\0';
    }
    else
    {
        status = nc_get_att(ncid, i_var, att_name, *att_value);
        att_value[0][length] = '\0';
    }
    return status;
}
//------------------------------------------------------------------------------
int TSFILE::get_attribute(int ncid, int i_var, char* att_name, std::string* att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(ncid, i_var, att_name, &length);
    if (status != NC_NOERR)
    {
        *att_value = "";
    }
    else
    {
        char* tmp_value = (char*)malloc(sizeof(char) * (length + 1));
        tmp_value[0] = '\0';
        status = nc_get_att(ncid, i_var, att_name, tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
    }
    return status;
}
//------------------------------------------------------------------------------
int TSFILE::get_attribute(int ncid, int i_var, std::string att_name, std::string* att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(ncid, i_var, att_name.c_str(), &length);
    if (status != NC_NOERR)
    {
        *att_value = "";
    }
    else
    {
        char* tmp_value = (char*)malloc(sizeof(char) * (length + 1));
        tmp_value[0] = '\0';
        status = nc_get_att(ncid, i_var, att_name.c_str(), tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
    }
    return status;
}
//------------------------------------------------------------------------------
int TSFILE::get_attribute(int ncid, int i_var, char* att_name, double* att_value)
{
    int status = -1;

    status = nc_get_att_double(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int TSFILE::get_attribute(int ncid, int i_var, char* att_name, int* att_value)
{
    int status = -1;

    status = nc_get_att_int(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int TSFILE::get_attribute(int ncid, int i_var, char* att_name, long* att_value)
{
    int status = -1;

    status = nc_get_att_long(ncid, i_var, att_name, att_value);

    return status;
}
std::string TSFILE::trim(std::string str)
{
    boost::algorithm::trim(str);
    return str;
}
char* TSFILE::trim(char* string)
{
    char* s;
    /*
     * Remove first trailing whitespaces and than leading
     */
    for (s = string + strlen(string) - 1;
        (*s == ' ' || *s == '\t' || *s == '\n');
        s--);
    *(s + 1) = '\0';
    for (s = string; *s == ' ' || *s == '\t'; s++);
    (void)strcpy_s(string, strlen(string) + 1, s);
    return string;
}

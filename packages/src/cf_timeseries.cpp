#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;


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
    nr_par_loc = 0;
    time_var_name = NULL;
    datetime_ntimes = 0;
}
TSFILE::~TSFILE()
{
    delete tsfilefilename;
    delete[] time_var_name;
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

    for (int i = 0; i < nr_par_loc; i++)
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
        for (int j = 0; j < par_loc[i]->nr_locations; j++)
        {
            delete par_loc[i]->location[j];
            par_loc[i]->location[j] = NULL;
        }
        delete par_loc[i]->location;
        par_loc[i]->nr_locations = 0;
        // delete times (it is a list)
        for (auto iter = qdt_times.begin(); iter != qdt_times.end(); ++iter)
        {
            qdt_times.erase(iter);
        }
    }
    if (nr_par_loc != 0)
    {
        delete[] par_loc;
        par_loc = NULL;
        nr_par_loc = 0;
    }
}

long TSFILE::read(QProgressBar * pgBar)
{
    int status;

    pgBar->show();
    pgBar->setValue(0);
    status = nc_open(this->tsfilefilename, NC_NOWRITE, &this->ncid);
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
    if (time_var_name == NULL)
    {
        QMessageBox::warning(NULL, QObject::tr("Error"), QString("No time variable found in file:\n%1.\nFile will not be read.").arg(this->tsfilefilename));
        return 1;
    }
    status = nc_close(this->ncid);

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
    char * c_units;
    size_t length;
    int dimids;
    int nr_time_series = 0;
    double fraction;
    double dt;

    pgBar->setValue(pgBar_start);
    var_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    status = nc_inq(this->ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i_var = 0; i_var < nvars; i_var++)
    {
        length = -1;
        status = nc_inq_attlen(this->ncid, i_var, "units", &length);
        if (status == NC_NOERR)
        {
            c_units = (char *)malloc(sizeof(char) * (length + 1));
            c_units[length] = '\0';
            status = nc_get_att(this->ncid, i_var, "units", c_units);
            QString units = QString(c_units).replace("T", " ");  // "seconds since 1970-01-01T00:00:00" changed into "seconds since 1970-01-01 00:00:00"
            date_time = units.split(" ");
            if (date_time.count()>=2)
            {
                if (!strcmp("since", date_time.at(1).toUtf8()))
                {
                    // now it is the time variable, can only be detected by the "seconds since 1970-01-01T00:00:00" character string
                    // retrieve the long_name, standard_name -> var_name for the xaxis label
                    length = -1;
                    status = nc_inq_var(this->ncid, i_var, var_name, NULL, &ndims, &dimids, &natts);
                    status = nc_inq_attlen(this->ncid, i_var, "long_name", &length);
                    if (status == NC_NOERR)
                    {
                        char * c_label = (char *)malloc(sizeof(char) * (length + 1));
                        c_label[length] = '\0';
                        status = nc_get_att(this->ncid, i_var, "long_name", c_label);
                        this->xaxis_label = QString(c_label);
                        free(c_label); c_label = NULL;
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
                        continue;
                    }
                    time_var_name = strdup(var_name);
                    status = nc_inq_dimlen(this->ncid, dimids, &datetime_ntimes);
                    // ex. date_time = "seconds since 2017-02-25 15:26:00"   year, yr, day, d, hour, hr, h, minute, min, second, sec, s and all plural forms
                    datetime_units = date_time.at(0);

                    QDate date = QDate::fromString(date_time.at(2), "yyyy-MM-dd");
                    QTime time = QTime::fromString(date_time.at(3), "hh:mm:ss");
                    this->RefDate = new QDateTime(date, time);
#if defined(DEBUG)
                    QString janm = date.toString("yyyy-MM-dd");
                    char * janm1 = strdup(janm.toUtf8());
                    janm = time.toString();
                    char * janm2 = strdup(janm.toUtf8());
                    janm = this->RefDate->toString("yyyy-MM-dd hh:mm:ss.zzz");
                    char * janm3 = strdup(janm.toUtf8());
#endif
                    times = (double *)malloc(sizeof(double)*datetime_ntimes);
                    status = nc_get_var_double(this->ncid, i_var, times);
                    if (datetime_ntimes >= 2)
                    {
                        dt = times[1] - times[0];
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
                        if (fmod(j, int(0.01*double(datetime_ntimes))) == 0)
                        {
                            fraction = double(pgBar_start) + double(j) / double(datetime_ntimes) * (double(pgBar_end) - double(pgBar_start));
                            pgBar->setValue(int(fraction));
                        }
                        if (datetime_units.contains("sec") ||
                            datetime_units.trimmed() == "s")  // seconds, second, sec, s
                        {
                            if (dt < 1.0)
                            {
                                qdt_times.append(this->RefDate->addMSecs(1000.*times[j]));  // milli seconds as smallest time unit
                            }
                            else
                            {
                                qdt_times.append(this->RefDate->addSecs(times[j]));  // seconds as smallest time unit
                            }
                        }
                        else if (datetime_units.contains("min"))  // minutes, minute, min
                        {
                            times[j] = times[j] * 60.0;
                            qdt_times.append(this->RefDate->addSecs(times[j]));
                        }
                        else if (datetime_units.contains("h"))  // hours, hour, hrs, hr, h
                        {
                            times[j] = times[j] * 3600.0;
                            qdt_times.append(this->RefDate->addSecs(times[j]));
                        }
                        else if (datetime_units.contains("d"))  // days, day, d
                        {
                            times[j] = times[j] * 24.0 * 3600.0;
                            qdt_times.append(this->RefDate->addSecs(times[j]));
                        }
                        // time[j] is now defined in seconds
#if defined (DEBUG)
                        if (dt < 1.0)
                            QString janm = qdt_times[j].toString("yyyy-MM-dd hh:mm:ss.zzz");
                        else
                            QString janm = qdt_times[j].toString("yyyy-MM-dd hh:mm:ss");
                        char * janm3 = strdup(janm.toUtf8());
#endif
                    }
                    break;
                }
            }
            free(c_units);
        }
    }
    free(var_name); var_name = NULL;
    pgBar->setValue(pgBar_end);
}
long TSFILE::get_count_times()
{
    return (long) this->datetime_ntimes;
}

double * TSFILE::get_times()
{
    return this->times;
}
QList<QDateTime> TSFILE::get_qdt_times()  // qdt: Qt Date Time
{
    return this->qdt_times;
}
quint64 TSFILE::ReferenceDatemSecsSinceEpoch()
{
    return this->RefDate->toMSecsSinceEpoch();
}
QString TSFILE::get_xaxis_label(void)
{
    return this->xaxis_label.trimmed() + " [" + this->xaxis_unit.trimmed() + "]";
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
    char * coord = NULL;

    size_t * par_dim;
    long i_par_loc = -1;
    long i_param = -1;

    if (this->time_var_name == NULL)
    {
        return;
    }

    this->nr_parameters = -1;

    var_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    dim_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));

    status = nc_inq(this->ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i_var = 0; i_var < nvars; i_var++)
    {
        long * par_dim_ids;  // parameter dimensions

        length = -1;
        i_par_loc = -1;
        status = nc_inq_var(this->ncid, i_var, var_name, NULL, &ndims, NULL, &natts);

        par_dim = (size_t *)malloc(sizeof(long *)*ndims);
        par_dim_ids = (long *)malloc(sizeof(long *)*ndims);
        status = nc_inq_vardimid(this->ncid, i_var, (int*)par_dim_ids);

        // check on the time dimension
        bool is_time_series = false;
        for (int i = 0; i < ndims; i++)
        {
            status = nc_inq_dim(this->ncid, par_dim_ids[i], dim_name, &par_dim[i]);
            if (!strcmp(this->time_var_name, dim_name))
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

        status = nc_inq_attlen(this->ncid, i_var, "coordinates", &length);
        if (status == NC_NOERR)
        {
            coord = (char *)malloc(sizeof(char) * (length + 1));
            status = nc_get_att(this->ncid, i_var, "coordinates", coord);
            coord[length] = '\0';
        }

        if (status != NC_NOERR)
        {
            // Look for variables with (time, location) dimension, 
            // the location is determined by the cf_role="timeseries_id"
            int var_found = 0;
            for (int i = 0; i < ndims; i++)
            {
                status = nc_inq_dim(this->ncid, par_dim_ids[i], dim_name, &par_dim[i]);
                if (!strcmp(dim_name, this->time_var_name))
                {
                    var_found += 1;
                }
                for (int j = 0; j < nr_par_loc; j++)
                {
                    if (!strcmp(dim_name, this->par_loc[j]->location_dim_name))
                    {
                        coord = strdup(this->par_loc[j]->location_var_name);
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
            for (int i = 0; i < nr_par_loc; i++)
            {
                char * crd = strdup(this->par_loc[i]->location_var_name);
                if (strstr(coord, (const char *)crd))
                {
                    i_par_loc = i;
                    i_param = this->par_loc[i_par_loc]->nr_parameters;
                    this->par_loc[i_par_loc]->nr_parameters += 1;
                    this->nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
                    delete crd;
                    break;
                }
                delete crd;
            }
            if (i_par_loc == -1 ||  i_param == -1)
            {
                continue;
            }

            // add this parameter
            status = nc_inq_var(this->ncid, i_var, var_name, NULL, &ndims, NULL, &natts);
            this->par_loc[i_par_loc]->ndim = ndims;
            this->nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
            ensure_capacity_parameters(i_par_loc, ndims);

            this->par_loc[i_par_loc]->parameter[i_param]->ndim = ndims;
            for (long j = 0; j < ndims; j++)
            {
                status = nc_inq_dim(this->ncid, par_dim_ids[j], dim_name, &par_dim[j]);
                this->par_loc[i_par_loc]->parameter[i_param]->dim_id[j] = par_dim_ids[j];
                this->par_loc[i_par_loc]->parameter[i_param]->dim_val[j] = (long)par_dim[j];
                this->par_loc[i_par_loc]->parameter[i_param]->dim_name[j] = strdup(dim_name);
            }
            length = -1;
            status = nc_inq_attlen(this->ncid, i_var, "long_name", &length);
            if (status == NC_NOERR)
            {
                char * parameter_name_c = (char *)malloc(sizeof(char) * (length + 1));
                status = nc_get_att(this->ncid, i_var, "long_name", parameter_name_c);
                parameter_name_c[length] = '\0';
                parameter_name = strdup(StripWhiteSpaces(parameter_name_c));
                free(parameter_name_c);
                parameter_name_c = nullptr;
            }
            else
            {
                parameter_name = strdup(var_name);
            }
            this->yaxis_label = QString(var_name);

            this->par_loc[i_par_loc]->parameter[i_param]->name = strdup(parameter_name);
            status = nc_inq_attlen(this->ncid, i_var, "units", &length);
            if (status == NC_NOERR)
            {
                unit = (char *)malloc(sizeof(char) * (length + 1));
                status = nc_get_att(this->ncid, i_var, "units", unit);
                unit[length] = '\0';
                this->par_loc[i_par_loc]->parameter[i_param]->unit = strdup(unit);
                free(unit); unit = NULL;
            }
            else
            {
                this->par_loc[i_par_loc]->parameter[i_param]->unit = strdup("?");
            }
            free(parameter_name); parameter_name = NULL;
            free(coord); coord = NULL;
        }
        else
        {
            // no coordinates attribute found, so search for variables with only the time-dimension and add the variable to the parameter list for location "Model wide"
            status = nc_inq_var(this->ncid, i_var, var_name, NULL, &ndims, NULL, &natts);
            status = nc_inq_dim(this->ncid, par_dim_ids[0], dim_name, &par_dim[0]);  // just one dimension, so index == 0
            if (ndims == 1 && !strcmp(time_var_name, dim_name)) // The dimension is the 'time' dimension
            {
                for (int i = 0; i < nr_par_loc; i++)
                {
                    char * crd = strdup(this->par_loc[i]->location_var_name);
                    if (strstr("model_wide", (const char *)crd))
                    {
                        i_par_loc = i;
                        i_param = this->par_loc[i_par_loc]->nr_parameters;
                        this->par_loc[i_par_loc]->nr_parameters += 1;
                        this->nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
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
                length = -1;
                status = nc_inq_attlen(this->ncid, i_var, "long_name", &length);
                if (status == NC_NOERR)
                {
                    char * parameter_name_c = (char *)malloc(sizeof(char) * (length + 1));
                    status = nc_get_att(this->ncid, i_var, "long_name", parameter_name_c);
                    parameter_name_c[length] = '\0';
                    parameter_name = strdup(StripWhiteSpaces(parameter_name_c));
                    free(parameter_name_c);
                    parameter_name_c = nullptr;
                }
                else
                {
                    parameter_name = strdup(var_name);
                }
                this->yaxis_label = QString(var_name);

                this->par_loc[i_par_loc]->parameter[i_param]->name = strdup(parameter_name);
                status = nc_inq_attlen(this->ncid, i_var, "units", &length);
                if (status == NC_NOERR)
                {
                    unit = (char *)malloc(sizeof(char) * (length + 1));
                    status = nc_get_att(this->ncid, i_var, "units", unit);
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
                status = nc_inq_var(this->ncid, i_var, var_name, NULL, &ndims, NULL, &natts);
                for (int i = 0; i < ndims; i++)
                {
                    status = nc_inq_dim(this->ncid, par_dim_ids[i], dim_name, &par_dim[i]);

                    if (!strcmp(this->time_var_name, dim_name))
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
                    for (int i = 0; i < nr_par_loc; i++)
                    {
                        char * crd = strdup(this->par_loc[i]->location_var_name);
                        if (strstr("model_wide", (const char *)crd))
                        {
                            i_par_loc = i;
                            i_param = this->par_loc[i_par_loc]->nr_parameters;
                            this->par_loc[i_par_loc]->nr_parameters += 1;
                            this->nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
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
                    length = -1;
                    status = nc_inq_attlen(this->ncid, i_var, "long_name", &length);
                    if (status == NC_NOERR)
                    {
                        char * parameter_name_c = (char *)malloc(sizeof(char) * (length + 1));
                        status = nc_get_att(this->ncid, i_var, "long_name", parameter_name_c);
                        parameter_name_c[length] = '\0';
                        parameter_name = StripWhiteSpaces(parameter_name_c);
                        free(parameter_name_c);
                        parameter_name_c = nullptr;
                    }
                    else
                    {
                        parameter_name = strdup(var_name);
                    }

                    this->par_loc[i_par_loc]->parameter[i_param]->name = strdup(parameter_name);
                    status = nc_inq_attlen(this->ncid, i_var, "units", &length);
                    if (status == NC_NOERR)
                    {
                        unit = (char *)malloc(sizeof(char) * (length + 1));
                        status = nc_get_att(this->ncid, i_var, "units", unit);
                        unit[length] = '\0';
                        this->par_loc[i_par_loc]->parameter[i_param]->unit = strdup(unit);
                        free(unit); unit = NULL;
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
    this->nr_parameters = this->par_loc[i_par_loc]->nr_parameters;
    param = (struct _parameter *) malloc(sizeof(struct _parameter) * this->nr_parameters);
    for (long i = 0; i < this->nr_parameters; i++)
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
    }
    return param;
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
    long i_par_loc;
    long * sn_dims; // station names dimensions
    bool model_wide_found = false;
    bool model_wide_exist = false;

    if (this->time_var_name == NULL)
    {
        return;
    }

    nr_par_loc = 0;

    dim_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    var_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));

    status = nc_inq(this->ncid, &ndims, &nvars, &natts, &nunlimited);
    // cf_role = timeseries_id
    for (long i = 0; i < nvars; i++)
    {
        length = -1;
        status = nc_inq_attlen(this->ncid, i, "cf_role", &length);
        if (status == NC_NOERR)
        {
            cf_role = (char *)malloc(sizeof(char) * (length + 1));
            status = nc_get_att(this->ncid, i, "cf_role", cf_role);
            cf_role[length] = '\0';

            if (!strcmp(cf_role, "timeseries_id"))
            {
                nc_type nc_type;
                location_name_varid = i;
                status = nc_inq_var(this->ncid, location_name_varid, var_name, &nc_type, &ndims, NULL, &natts);
                for (int i = 0; i < nr_par_loc; i++)
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
                    nr_par_loc += 1;
                    ensure_capacity_par_loc(nr_par_loc);
                    if (model_wide_exist)
                    {
                        struct _par_loc * tmp = this->par_loc[i_par_loc];
                        this->par_loc[i_par_loc] = this->par_loc[nr_par_loc - 1];
                        this->par_loc[nr_par_loc - 1] = tmp;
                    }
                    else
                    {
                        i_par_loc = nr_par_loc - 1;
                    }
                }
                this->par_loc[i_par_loc]->location_var_name = strdup(var_name);
                length = -1;
                status = nc_inq_attlen(this->ncid, location_name_varid, "long_name", &length);
                if (status == NC_NOERR)
                {
                    char * long_name = (char *)malloc(sizeof(char) * (length + 1));
                    status = nc_get_att(this->ncid, location_name_varid, "long_name", long_name);
                    long_name[length] = '\0';
                    this->par_loc[i_par_loc]->location_long_name = strdup(long_name);
                    //free(long_name);
                }
                else
                {
                    this->par_loc[i_par_loc]->location_long_name = strdup(var_name);
                }

                sn_dims = (long *)malloc(sizeof(long *)*ndims);
                status = nc_inq_vardimid(this->ncid, location_name_varid, (int*)sn_dims);

                mem_length = 1;
                name_len = -1;
                for (long j = 0; j < ndims; j++)
                {
                    length = -1;
                    status = nc_inq_dim(this->ncid, sn_dims[j], dim_name, &length);

                    if (strstr(dim_name, "len") || name_len == -1 && j == 1)  // second dimension is the string length if not already set
                    {
                        name_len = length;
                    }
                    else
                    {
                        this->par_loc[i_par_loc]->location_dim_name = strdup(dim_name);
                        this->nr_locations = (long)length;
                    }
                    mem_length = mem_length * length;
                }
                this->par_loc[i_par_loc]->location = new QString *[nr_locations];
                // reading 64 strings for each location, length of string??
                if (nc_type == NC_STRING)
                {
                    char ** location_strings = (char **)malloc(sizeof(char *) * (mem_length)+1);
                    status = nc_get_var_string(this->ncid, location_name_varid, location_strings);
                    QString janm = QString("JanM");
                    for (int k = 0; k < nr_locations; k++)
                    {
                        janm = QString("");
                        for (int k2 = 0; k2 < name_len; k2++)
                        {
                            QTextCodec *codec2 = QTextCodec::codecForName("UTF-8");
                            QString str = codec2->toUnicode(*(location_strings + k * name_len + k2));
                            janm = janm + str;
                        }
                        this->par_loc[i_par_loc]->location[k] = new QString(janm);
                    }
                    free(location_strings);
                }
                else if (nc_type == NC_CHAR)
                {
                    char * location_chars = (char *)malloc(sizeof(char *) * (mem_length)+1);
                    status = nc_get_var_text(this->ncid, location_name_varid, location_chars);
                    char * janm = (char *)malloc(sizeof(char)*(name_len + 1));
                    janm[name_len] = '\0';
                    for (int k = 0; k < nr_locations; k++)
                    {
                        strncpy(janm, location_chars + k * name_len, name_len);
                        this->par_loc[i_par_loc]->location[k] = new QString(janm);
                    }
                    free(janm);
                    free(location_chars);
                }
                else
                {
                    // trying to read unsupported variable
                }

                this->par_loc[i_par_loc]->nr_locations = this->nr_locations;
                free(sn_dims);
            }
            free(cf_role);
        }
        else
        {
            // no cf_role == timeseries_id found, so search for variables with only the timedimension and add the location with name "model wide"
            bool model_wide_found = false;
            location_name_varid = i;
            status = nc_inq_var(this->ncid, location_name_varid, var_name, NULL, &ndims, NULL, &natts);
            if (ndims == 1)
            {
                sn_dims = (long *)malloc(sizeof(long *)*ndims);
                status = nc_inq_vardimid(this->ncid, location_name_varid, (int*)sn_dims);
                status = nc_inq_dim(this->ncid, sn_dims[0], dim_name, &length);
                if (!strcmp(this->time_var_name, dim_name))
                {
                    for (int i = 0; i < nr_par_loc; i++)
                    {
                        if (!strcmp(this->par_loc[i]->location_var_name, "model_wide"))
                        {
                            model_wide_found = true;
                        }
                    }
                    if (!model_wide_found)
                    {
                        nr_par_loc += 1;
                        i_par_loc = nr_par_loc - 1;
                        ensure_capacity_par_loc(nr_par_loc);
                        this->par_loc[i_par_loc]->location_var_name = strdup("model_wide");
                        this->par_loc[i_par_loc]->location_long_name = strdup("Model wide");
                        this->par_loc[i_par_loc]->location_dim_name = strdup("");
                        this->par_loc[i_par_loc]->nr_locations = 1;
                        this->par_loc[i_par_loc]->location = new QString *[1];
                        this->par_loc[i_par_loc]->location[0] = new QString ("Model wide");
                    }
                }
                free(sn_dims);
            }
            else if (ndims == 2)  //two dimensions, one is the time and the other has dimesion 1.
            {
                int accepted = 0;
                sn_dims = (long *)malloc(sizeof(long *)*ndims);
                status = nc_inq_vardimid(this->ncid, location_name_varid, (int*)sn_dims);
                for (int i = 0; i < ndims; i++)
                {
                    status = nc_inq_dim(this->ncid, sn_dims[i], dim_name, &length);

                    if (!strcmp(this->time_var_name, dim_name))
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
                    for (int i = 0; i < nr_par_loc; i++)
                    {
                        if (!strcmp(this->par_loc[i]->location_var_name, "model_wide"))
                        {
                            model_wide_found = true;
                        }
                    }
                    if (!model_wide_found)
                    {
                        nr_par_loc += 1;
                        i_par_loc = nr_par_loc - 1;
                        ensure_capacity_par_loc(nr_par_loc);
                        this->par_loc[i_par_loc]->location_var_name = strdup("model_wide");
                        this->par_loc[i_par_loc]->location_long_name = strdup("Model wide");
                        this->par_loc[i_par_loc]->nr_locations = 1;
                        this->par_loc[i_par_loc]->location[0] = new QString("Model wide");
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
    return this->nr_par_loc;
}
char * TSFILE::get_par_loc_long_name(long i_par_loc)
{
    return this->par_loc[i_par_loc]->location_long_name;
}

long TSFILE::get_count_locations(long i_par_loc)
{
    return this->par_loc[i_par_loc]->nr_locations;
}

QString ** TSFILE::get_location_names(long i_par_loc)
{
    long nr_loc = this->par_loc[i_par_loc]->nr_locations;
    QString ** locations;
    locations = new QString *[nr_loc];
    for (long i = 0; i < nr_loc; i++)
    {
        locations[i] = this->par_loc[i_par_loc]->location[i];
    }

    return locations;
}


void TSFILE::read_global_attributes()
{
    long status;
    int ndims, nvars, natts, nunlimited;
    nc_type att_type;
    size_t att_length;

    char * att_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    att_name_c[0] = '\0';

    status = nc_inq(this->ncid, &ndims, &nvars, &natts, &nunlimited);

    this->global_attributes = (_global_attributes *) malloc(sizeof(_global_attribute));
    this->global_attributes->count = natts;
    this->global_attributes->attribute = (_global_attribute **) malloc(sizeof(_global_attribute *) * natts);
    for (long i = 0; i < natts; i++)
    {
        this->global_attributes->attribute[i] = (_global_attribute *)malloc(sizeof(_global_attribute));
    }

    for (long i = 0; i < natts; i++)
    {
        status = nc_inq_attname(this->ncid, NC_GLOBAL, i, att_name_c);
        status = nc_inq_att(this->ncid, NC_GLOBAL, att_name_c, &att_type, &att_length);
        this->global_attributes->attribute[i]->name = strdup(att_name_c);
        this->global_attributes->attribute[i]->type = NC_CHAR;
        this->global_attributes->attribute[i]->length = att_length;
        this->global_attributes->attribute[i]->cvalue = NULL;
        //global_attributes->attribute[i]->svalue = NULL;
        this->global_attributes->attribute[i]->ivalue = NULL;
        this->global_attributes->attribute[i]->rvalue = NULL;
        this->global_attributes->attribute[i]->dvalue = NULL;
        if (att_type == NC_CHAR)
        {
            char * att_value_c = (char *)malloc(sizeof(char) * (att_length + 1));
            att_value_c[0] = '\0';
            status = nc_get_att_text(this->ncid, NC_GLOBAL, att_name_c, att_value_c);
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

double * TSFILE::get_time_series(long cb_index, char * parameter, long loc_id, long layer_id)
{
    int ndims, nvars, natts, nunlimited;
    long status;
    long par_id = -1;
    long j;
    long nr_loc;
    int nr_layers;
    size_t length;
    double * y_array;
    size_t mem_length;
    char * dim_name;
    char * var_name;
    char * tmp_name;

    long * var_dims; // dimensions of variable
    dim_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    var_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));

    status = nc_open(this->tsfilefilename, NC_NOWRITE, &this->ncid);
    status = nc_inq(this->ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i = 0; i < nvars; i++)
    {
        // look for the var_id of the nc_variable with long_name "parameter"
        length = -1;
        status = nc_inq_attlen(this->ncid, i, "long_name", &length);
        if (status == NC_NOERR)
        {
            tmp_name = (char *)malloc(sizeof(char) * (length + 1));
            tmp_name[length] = '\0';
            status = nc_get_att(this->ncid, i, "long_name", tmp_name);
            tmp_name = StripWhiteSpaces(tmp_name);
            if (!strcmp(parameter, tmp_name))
            {
                par_id = i;
                break;
            }
            free(tmp_name); tmp_name = NULL;
        }
        else
        {
            // try var_name
            status = nc_inq_var(this->ncid, i, var_name, NULL, NULL, NULL, NULL);
            if (!strcmp(parameter, var_name))
            {
                par_id = i;
                break;
            }
        }
    }

    // retrieve the data belonging to the parameter variable
    status = nc_inq_var(this->ncid, par_id, var_name, NULL, &ndims, NULL, &natts);

    var_dims = (long *)malloc(sizeof(long) * ndims);
    status = nc_inq_vardimid(this->ncid, par_id, (int *)var_dims);

    nr_loc = this->get_count_locations(cb_index);
    nr_layers = 1;
    
    mem_length = 1;
    for (int i = 0; i < ndims; i++)
    {
        status = nc_inq_dim(this->ncid, var_dims[i], dim_name, &length);
        if (i == 2) nr_layers = length;  // third dimension is the layer
        mem_length = mem_length * length;
    }

    y_array = (double *)malloc(sizeof(double) * mem_length);
    status = nc_get_var_double(this->ncid, par_id, y_array);
    //select colum loc_id from variable "parameter"
    j = -1;
    long ii_layer = fmax(0, layer_id);
    this->y_values = (double *)malloc(sizeof(double) * this->datetime_ntimes);
    int ii;
    for (int i_times = 0; i_times < this->datetime_ntimes; i_times++)
    {
        j = j + 1;
        ii = ii_layer + nr_layers * loc_id + nr_layers * nr_loc * i_times;
        this->y_values[j] = y_array[ii];
    }

    free(y_array); y_array = NULL;
    free(var_dims); var_dims= NULL;
    free(dim_name); dim_name = NULL;
    free(var_name); var_name = NULL;

    status = nc_close(this->ncid);
    if (status != NC_NOERR) {
        // handle_error(status);
    }
    return this->y_values;
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
}
void TSFILE::ensure_capacity_parameters(long i_par_loc, long ndims)
{
    long i_param;
    if (this->nr_parameters == 1)
    {
        this->par_loc[i_par_loc]->parameter = (struct _parameter **) malloc(sizeof(struct _parameter *) * this->nr_parameters);
    }
    else if (this->nr_parameters > 1)
    {
        this->par_loc[i_par_loc]->parameter = (struct _parameter **) realloc(this->par_loc[i_par_loc]->parameter, sizeof(struct _parameter *) * this->nr_parameters);
    }
    i_param = this->par_loc[i_par_loc]->nr_parameters-1;
    this->par_loc[i_par_loc]->parameter[i_param] = (struct _parameter *) malloc(sizeof(struct _parameter) * 1);
    this->par_loc[i_par_loc]->parameter[i_param]->dim_id = (long *)malloc(sizeof(long *) * ndims);
    this->par_loc[i_par_loc]->parameter[i_param]->dim_val = (long *)malloc(sizeof(long *) * ndims);
    this->par_loc[i_par_loc]->parameter[i_param]->dim_name = (char **)malloc(sizeof(char *) * ndims);
}
/* @@
 *
 * Remove leading and trailing blanks from a string ended with \0
 */
char * TSFILE::StripWhiteSpaces(char * string)
{
    char * s;
    /*
     * Remove first trailing whitespaces and than leading
     */
    for (s = string + strlen(string) - 1;
        (*s == ' ' || *s == '\t' || *s == '\n');
        s--);
    *(s + 1) = '\0';
    for (s = string; *s == ' ' || *s == '\t'; s++);
    strcpy(string, s);
    return string;
}


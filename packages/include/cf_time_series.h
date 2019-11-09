#ifndef TSFILE_H
#define TSFILE_H

typedef enum{
    UNKNOWN,
    HISTORY,
} FILE_TYPE;

struct _global_attributes {
    int count;
    struct _global_attribute ** attribute;
};
struct _global_attribute {
    int type;
    int length;
    char * name;
    char * cvalue;
    char ** svalue;
    char * ivalue;
    char * rvalue;
    char * dvalue;
};

struct _parameter {
    long ndim;
    long * dim_id;
    long * dim_val;
    char ** dim_name;
    char * name;
    char * unit;
}; 

struct _par_loc {
    char * location_var_name;  // to check the coordinate attribute
    char * location_long_name;  // to present in combobox
    char * location_dim_name;
    long ndim;
    long nr_parameters;
    long nr_locations;
    struct _parameter ** parameter;
    QString ** location;
};


class TSFILE
{
public:
    TSFILE(QFileInfo filename, FILE_TYPE ftype);
    ~TSFILE();
    long read(QProgressBar *);
    void read_times(QProgressBar *, long, long);
    void read_global_attributes();
    void read_parameters();
    void read_locations();
    struct _global_attributes * get_global_attributes(void);
    double * get_time_series(long, char *, long, long);
    QString get_xaxis_label(void);
    long get_count_par_loc();
    long get_count_parameters(long);
    char * get_par_loc_long_name(long);
    struct _parameter * get_parameters(long);
    long get_count_locations(long);
    QString ** get_location_names(long);
    long get_count_times();
    double * get_times();
    QList<QDateTime> get_qdt_times();
    quint64 ReferenceDatemSecsSinceEpoch();
    void ensure_capacity_par_loc(long);
    void ensure_capacity_parameters(long, long);
    
    QFileInfo fname;
    FILE_TYPE type;
    struct _meta * meta;
    _global_attributes * global_attributes;
    struct _parameter * param;
    char * time_var_name;
    size_t datetime_ntimes;
    QString datetime_units;
    QDateTime * RefDate;

    double * times;  // only the seconds
    QList<QDateTime> qdt_times;  // date time presentation yyyy-MM-dd hh:mm:ss.zzz
    char ** location_name;
    char ** parameter;
#if defined (DEBUG)
    char  janm;
#endif

private:
    char * tsfilefilename;
    struct _par_loc ** par_loc;  // array of parameters, locations and time-series
    QStringList date_time;
    QStringList datetime_ref_date;
    QStringList datetime_ref_time;
    QString xaxis_label;
    QString xaxis_unit;
    QString yaxis_label;
    QString yaxis_unit;
    long nr_locations;
    size_t name_len;
    long nr_parameters;
    long nr_par_loc;
    int ncid;
    double * y_values;
};

#endif

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
    int pre_selected;  // tri-state: 0: not pre-selected (show all); only when saved to json file (also in memory will this action be performed to generate a new set of preselected parameters)
                       //            1: pre-selected (show all); increase by 1 show then show only (2) (Should know what was previously selected)
};

struct _location {
    QString * name;
    int pre_selected;  // tri-state: 0: not pre-selected (show all); only when saved to json file (also in memory will this action be performed to generate a new set of preselected parameters)
                       //            1: pre-selected (show all); increase by 1 show then show only (2)
                       //            2: show pre-selected; decrease by 1, show all (0 and 1) (Should know what was previously selected)
};

struct _par_loc {
    char * location_var_name;  // to check the coordinate attribute
    char * location_long_name;  // to present in combobox
    char * location_dim_name;
    long ndim;
    long nr_parameters;
    long nr_locations;
    bool pre_selection;  // one of the parameters and locaions is pre-selected
    struct _parameter ** parameter;
    struct _location ** location;
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
    std::vector<double> get_time_series(long, char *, long, long);
    QString get_xaxis_label(void);
    long get_count_par_loc();
    long get_count_parameters(long);
    char * get_par_loc_long_name(long);
    struct _parameter * get_parameters(long);
    long get_count_locations(long);
    struct _location * get_locations(long);
    long put_location(long, long, struct _location *);
    long put_parameter(long, long, struct _parameter *);

    long get_count_times();
    std::vector<double> get_times();
    QList<QDateTime> get_qdt_times();
    QDateTime * get_reference_date();
    bool get_pre_selection();
    void put_pre_selection(bool);
    int get_cb_parloc_index();
    void put_cb_parloc_index(int);

    void ensure_capacity_par_loc(long);
    void ensure_capacity_parameters(long, long);
    void ensure_capacity_locations(long, long);

    QFileInfo fname;
    FILE_TYPE type;
    struct _meta * meta;
    _global_attributes * global_attributes;
    struct _parameter * param;
    char * time_var_name;
    size_t datetime_ntimes;
    QString datetime_units;
    QDateTime * RefDate;

    double * times_c;  // only the seconds
    std::vector<double> times;  // vector of seconds
    QList<QDateTime> qdt_times;  // date time presentation yyyy-MM-dd hh:mm:ss.zzz
    char ** location_name;
    char ** parameter;
#if defined (DEBUG)
    char  janm;
#endif

private:
    char * StripWhiteSpaces(char *);

    char * tsfilefilename;
    struct _par_loc ** par_loc;  // array of parameters, locations and time-series
    QStringList date_time;
    QStringList datetime_ref_date;
    QStringList datetime_ref_time;
    QString xaxis_label;
    QString xaxis_unit;
    QString yaxis_label;
    QString yaxis_unit;
    size_t name_len;
    long nr_par_loc;
    long nr_parameters;
    long nr_locations;
    bool m_pre_selection;
    int m_cb_parloc_index;
    int ncid;
    std::vector<double> m_y_values;
};

#endif

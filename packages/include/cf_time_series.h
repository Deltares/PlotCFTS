#ifndef TSFILE_H
#define TSFILE_H

#include <boost/algorithm/string/trim.hpp>

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

struct _time_series {
    QString long_name;
    QString unit;
    std::vector<double> times;  // vector of seconds
    QList<QDateTime> qdt_time;  // date time presentation yyyy-MM-dd hh:mm:ss.zzz
    std::vector<int> pre_selected;  // tri-state: 0: not pre-selected (show all); only when saved to json file (also in memory will this action be performed to generate a new set of preselected parameters)
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
    long read_locations();
    struct _global_attributes * get_global_attributes(void);
    std::vector<double> get_time_series(long, std::string, long, long);
    QString get_xaxis_label(void);
    long get_count_par_loc();
    long get_count_parameters(long);
    char * get_par_loc_long_name(long);
    struct _parameter * get_parameters(long);
    long get_count_locations(long);
    struct _location * get_locations(long);
    long put_location(long, long, struct _location *);
    long put_parameter(long, long, struct _parameter *);

    _time_series m_times;
    long get_count_times();
    _time_series get_times();
    void put_times(_time_series);

    // Needed for reading times; taken from read_ugrid example
    struct _time_series m_time_series;
    QVector<QDateTime> qdt_times;
    std::vector<_time_series> time_series;

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
    struct _parameter * m_param;
    size_t datetime_ntimes;
    QString datetime_units;
    QDateTime * RefDate;

    char ** location_name;
    char ** parameter;

private:
    int get_attribute(int, int, std::string, std::string*);
    int get_attribute(int, int, std::string, double*);
    int get_attribute(int, int, std::string, int*);   
    int get_attribute(int, int, std::string, long*);
    int get_name_for_variable(int, int, std::string*);
    int get_unit_for_variable(int, int, std::string*);
    
    std::string trim(std::string);
    char* trim(char*);

    char * tsfilefilename;
    struct _par_loc ** par_loc;  // array of parameters, locations and time-series
    QStringList m_date_time;
    QStringList datetime_ref_date;
    QStringList datetime_ref_time;
    char* m_time_var_name;
    QString xaxis_label;
    QString xaxis_unit;
    QString m_yaxis_label;
    QString yaxis_unit;
    size_t name_len;
    long m_nr_par_loc;
    long m_nr_parameters;
    long m_nr_locations;
    bool m_pre_selection;
    int m_cb_parloc_index;
    int m_ncid;
    std::vector<double> m_y_values;
};

#endif

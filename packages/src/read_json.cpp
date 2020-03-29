
/* 
A json file parser based on boost libraries. It creates a tree structure and parses the json file.
prop_get templated function does the work of extracting the numbers(integer, float or double) from the strings
using c++11 regular expression functionality.
*/

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <type_traits>
#include <regex>
#include <vector>
#include <iostream>
#include <string>
#include "read_json.h"
#include "QMessageBox"

struct Getter
{
    template<typename T, class Ptree>
    static void prop_get_json(Ptree &pt, const string & data, vector<T> & results)
    {
        try {
            // first strip key from data, ex. data=data.boundary.nodeid, chapter==data.boundary, key==nodeid
            size_t i = data.find_last_of(".");
            string chapter = data.substr(0, i);
            string key = data.substr(i + 1);
            auto child = pt.get_child(chapter);
            for (auto& p : child)
            {
                //cout << p.first.data() << ": " << p.second.data() << endl;
                if (p.first.data() == key)
                {
                    T val = p.second.get_value<T>();
                    results.push_back(val);
                }
            }
            for (auto& pv : child)
            {
                //cout << endl;
                for (auto& p : pv.second)
                {
                    //cout << p.first.data() << ": " << p.second.data() << endl;
                    if (p.first.data() == key)
                    {
                        T val = p.second.get_value<T>();
                        results.push_back(val);
                    }
                }
            }
        }
        catch (const ptree_error &e)
        {
#if defined DEBUG
            //cout << e.what() << endl;
            QString msg = QString::fromStdString(e.what()).trimmed();
            //QMessageBox::warning(0, "DEBUG: READ_JSON::prop_get_json", QString("%1").arg(msg));
#endif
        }
    }
};

READ_JSON::READ_JSON(string file_json)
{
    m_filename = file_json;
    try
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(file_json, pt);
        m_ptrtree = pt;
    }
    catch (const ptree_error &e)
    {
        //cout << e.what() << endl;
        QString msg = QString::fromStdString(e.what()).trimmed();
        QMessageBox::warning(0, "READ_JSON::READ_JSON", QString("%1").arg(msg));
    }
}
READ_JSON::~READ_JSON()
{
    delete & m_ptrtree;
}
long READ_JSON::get(string data, vector<string> & strJsonResults)
{
    Getter::prop_get_json(m_ptrtree, data, strJsonResults);
    return 0;
}
long READ_JSON::get(string data, vector<double> & strJsonResults)
{
    Getter::prop_get_json(m_ptrtree, data, strJsonResults);
    return 0;
}
long READ_JSON::find(string data)
{
    long error = 1;
    boost::optional<std::string> c = m_ptrtree.get_optional<std::string>(data);
    if (c.is_initialized())
    {
        error = 0;
    }
    return error;
}

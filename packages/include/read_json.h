#ifndef __READ_JSON_H
#define __READ_JSON_H

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
using namespace boost;
using namespace property_tree;

class READ_JSON
{
public:
    READ_JSON(string);
    ~READ_JSON();
    long get(string, vector<string> &);
    long get(string, vector<double> &);
    long READ_JSON::find(string);
        
    void prop_get_json(boost::property_tree::ptree &, const string, vector<string> &);
    void prop_get_json(boost::property_tree::ptree &, const string, vector<double> &);
    template<class T> void gett(string, vector<T>);

private:
    string m_filename;
    boost::property_tree::ptree m_ptrtree;
};
#endif  // __READ_JSON_H

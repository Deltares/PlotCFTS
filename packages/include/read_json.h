#ifndef __READ_JSON_H
#define __READ_JSON_H

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace boost;
using namespace property_tree;

class READ_JSON
{
public:
    READ_JSON(std::string);
    ~READ_JSON();
    long get(std::string, std::vector<std::string> &);
    long get(std::string, std::vector<double> &);
    long find(std::string);
        
    void prop_get_json(boost::property_tree::ptree &, const std::string, std::vector<std::string> &);
    void prop_get_json(boost::property_tree::ptree &, const std::string, std::vector<double> &);
    template<class T> void gett(std::string, std::vector<T>);

private:
    std::string m_filename;
    boost::property_tree::ptree m_ptrtree;
};
#endif  // __READ_JSON_H

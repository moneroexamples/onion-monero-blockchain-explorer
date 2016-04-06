//
// Created by marcin on 22/11/15.
//

#ifndef XMR2CSV_DATEPARSER_H
#define XMR2CSV_DATEPARSER_H

#include <iostream>
#include <boost/date_time/local_time/local_time.hpp>


// taken from: http://stackoverflow.com/a/19482908/248823
struct dateparser
{
    boost::posix_time::ptime pt;
    unsigned year, month, day;

    dateparser(std::string fmt);

    bool
    operator()(std::string const& text);

private:
    std::stringstream ss;
};

#endif //XMR2CSV_DATEPARSER_H

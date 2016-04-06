//
// Created by marcin on 22/11/15.
//

#include "dateparser.h"


dateparser::dateparser(std::string fmt)
{
    // set format
    using namespace boost::local_time;
    local_time_input_facet* input_facet = new local_time_input_facet();
    input_facet->format(fmt.c_str());
    ss.imbue(std::locale(ss.getloc(), input_facet));
}


bool
dateparser::operator()(std::string const& text)
{
    ss.clear();
    ss.str(text);

    bool ok = bool(ss >> pt);

    if (ok)
    {
        auto tm = to_tm(pt);
        year    = tm.tm_year;
        month   = tm.tm_mon + 1; // for 1-based (1:jan, .. 12:dec)
        day     = tm.tm_mday;
    }

    return ok;
}
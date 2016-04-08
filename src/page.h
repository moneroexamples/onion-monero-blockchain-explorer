//
// Created by mwo on 8/04/16.
//

#ifndef CROWXMR_PAGE_H
#define CROWXMR_PAGE_H



#include "../ext/minicsv.h"

#include "monero_headers.h"
#include "tools.h"


#define TMPL_DIR   "./templates"
#define TMPL_INDEX TMPL_DIR  "/index.html"

#define READ_TMPL(tmpl_path) xmreg::read(tmpl_path)

namespace xmreg {


    class page {

    public:

        string
        index()
        {
            std::string view = READ_TMPL(TMPL_INDEX);

            return view;
        }


    private:

    };
}


#endif //CROWXMR_PAGE_H

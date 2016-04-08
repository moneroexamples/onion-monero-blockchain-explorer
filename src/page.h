//
// Created by mwo on 8/04/16.
//

#ifndef CROWXMR_PAGE_H
#define CROWXMR_PAGE_H



#include "mstch/mstch.hpp"
#include "../ext/format.h"




#include "monero_headers.h"

#include "MicroCore.h"
#include "tools.h"


#define TMPL_DIR              "./templates"
#define TMPL_INDEX  TMPL_DIR  "/index.html"
#define TMPL_HEADER TMPL_DIR  "/header.html"
#define TMPL_FOOTER TMPL_DIR  "/footer.html"

#define READ_TMPL(tmpl_path) xmreg::read(tmpl_path)

namespace xmreg {

    using namespace cryptonote;
    using namespace crypto;
    using namespace std;

    class page {


        MicroCore* mcore;
        Blockchain* core_storage;

    public:

        page(MicroCore* _mcore, Blockchain* _core_storage)
                :  mcore {_mcore}, core_storage {_core_storage}
        {

        }

        string
        index()
        {
            // get the current blockchain height. Just to check  if it reads ok.
            uint64_t height = core_storage->get_current_blockchain_height() - 1;

            mstch::map context {
                    {"height",  fmt::format("{:d}", height)},
                    {"blocks",  mstch::array()}
            };

            size_t no_of_last_blocks {50};

            mstch::array& blocks = boost::get<mstch::array>(context["blocks"]);

            for (size_t i = height; i > height -  no_of_last_blocks; --i)
            {
                block blk;

                mcore->get_block_by_height(i, blk);

                crypto::hash blk_hash = core_storage->get_block_id_by_height(i);

                blocks.push_back(mstch::map {
                        {"height"     , to_string(i)},
                        {"timestamp"  , xmreg::timestamp_to_str(blk.timestamp)},
                        {"hash"       , fmt::format("{:s}", blk_hash)},
                        {"notx"       , fmt::format("{:d}", blk.tx_hashes.size())}
                });
            }


            std::string view = READ_TMPL(TMPL_INDEX);

            string full_page = get_full_page(view);

            return mstch::render(view, context);
        }


    private:
        string
        get_full_page(string& middle)
        {
            return READ_TMPL(TMPL_HEADER)
                   + middle
                   + READ_TMPL(TMPL_FOOTER);
        }

    };
}


#endif //CROWXMR_PAGE_H

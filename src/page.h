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


#include <algorithm>

#include<ctime>

#define TMPL_DIR             "./templates"
#define TMPL_INDEX  TMPL_DIR "/index.html"
#define TMPL_HEADER TMPL_DIR "/header.html"
#define TMPL_FOOTER TMPL_DIR "/footer.html"



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
        index(bool refresh_page = false)
        {
            //get current server timestamp
            time_t server_timestamp = std::time(nullptr);

            // get the current blockchain height. Just to check  if it reads ok.
            uint64_t height = core_storage->get_current_blockchain_height() - 1;

            // initalise page tempate map with basic info about blockchain
            mstch::map context {
                    {"refresh",          refresh_page},
                    {"height",           fmt::format("{:d}", height)},
                    {"server_timestamp", xmreg::timestamp_to_str(server_timestamp)},
                    {"blocks",           mstch::array()}
            };

            // number of last blocks to show
            size_t no_of_last_blocks {100 + 1};

            // get reference to blocks template map to be field below
            mstch::array& blocks = boost::get<mstch::array>(context["blocks"]);

            time_t prev_blk_timestamp {0};

            // iterate over last no_of_last_blocks of blocks
            //for (size_t i = height; i > height -  no_of_last_blocks; --i)
            for (size_t i = height - no_of_last_blocks; i <= height; ++i)
            {
                // get block at the given height i
                block blk;

                mcore->get_block_by_height(i, blk);

                // get block's hash
                crypto::hash blk_hash = core_storage->get_block_id_by_height(i);

                uint64_t delta_minutes {0};
                uint64_t delta_seconds {0};

                if (prev_blk_timestamp > 0)
                {
                    array<size_t, 5> delta_time = timestamp_difference(
                                     prev_blk_timestamp, blk.timestamp);

                    delta_minutes = delta_time[3];
                    delta_seconds = delta_time[4];
                }

                string timestamp_str = xmreg::timestamp_to_str(blk.timestamp)
                                            + fmt::format(" ({:02d}:{:02d})",
                                               delta_minutes, delta_seconds);

                // get xmr in the block reward
                array<uint64_t, 2> coinbase_tx = sum_money_in_tx(blk.miner_tx);

                // get transactions in the block
                const vector<cryptonote::transaction>& txs_in_blk =
                            core_storage->get_db().get_tx_list(blk.tx_hashes);

                // sum xmr in the inputs and ouputs of all transactions
                array<uint64_t, 2> sum_xmr_in_out = sum_money_in_txs(txs_in_blk);

                // get sum of all transactions in the block
                uint64_t sum_fees = sum_fees_in_txs(txs_in_blk);

                // get mixin number in each transaction
                vector<uint64_t> mixin_numbers = get_mixin_no_in_txs(txs_in_blk);

                // find minimum and maxium mixin numbers
                int mixin_min {-1};
                int mixin_max {-1};

                if (!mixin_numbers.empty())
                {
                    mixin_min = static_cast<int>(
                            *std::min_element(mixin_numbers.begin(), mixin_numbers.end()));
                    mixin_max = static_cast<int>(
                            *max_element(mixin_numbers.begin(), mixin_numbers.end()));
                }

                auto mixin_format = [=]() -> mstch::node
                {
                    if (mixin_min < 0)
                    {
                        return string("N/A");
                    }
                    return fmt::format("{:d} - {:d}", mixin_min, mixin_max);
                };

                // set output page template map
                blocks.push_back(mstch::map {
                        {"height"      , to_string(i)},
                        {"timestamp"   , timestamp_str},
                        {"hash"        , fmt::format("{:s}", blk_hash)},
                        {"block_reward", fmt::format("{:0.4f} ({:0.4f})",
                                                     XMR_AMOUNT(coinbase_tx[1]),
                                                     XMR_AMOUNT(sum_fees))},
                        {"notx"        , fmt::format("{:d}", blk.tx_hashes.size())},
                        {"xmr_inputs"  , fmt::format("{:0.4f}",
                                                     XMR_AMOUNT(sum_xmr_in_out[0]))},
                        {"xmr_outputs" , fmt::format("{:0.4f}",
                                                     XMR_AMOUNT(sum_xmr_in_out[1]))},
                        {"mixin_range" , mstch::lambda {mixin_format}}
                });

                prev_blk_timestamp  = blk.timestamp;
            }

            // reverse blocks and remove last (i.e., oldest)
            // block. This is done so that time delats
            // are easier to calcualte in the above for loop
            std::reverse(blocks.begin(), blocks.end());
            blocks.pop_back();

            // read index.html
            std::string index_html = xmreg::read(TMPL_INDEX);

            // add header and footer
            string full_page = get_full_page(index_html);

            // render the page
            return mstch::render(full_page, context);
        }


    private:

        string
        get_full_page(string& middle)
        {
            return xmreg::read(TMPL_HEADER)
                   + middle
                   + xmreg::read(TMPL_FOOTER);
        }

    };
}


#endif //CROWXMR_PAGE_H

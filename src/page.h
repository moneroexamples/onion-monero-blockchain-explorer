//
// Created by mwo on 8/04/16.
//

#ifndef CROWXMR_PAGE_H
#define CROWXMR_PAGE_H



#include "mstch/mstch.hpp"
#include "rapidjson/document.h"
#include "../ext/format.h"




#include "monero_headers.h"

#include "MicroCore.h"
#include "tools.h"
#include "rpccalls.h"

#include <algorithm>

#include<ctime>

#define TMPL_DIR              "./templates"
#define TMPL_INDEX   TMPL_DIR "/index.html"
#define TMPL_MEMPOOL TMPL_DIR "/mempool.html"
#define TMPL_HEADER  TMPL_DIR "/header.html"
#define TMPL_FOOTER  TMPL_DIR "/footer.html"



namespace xmreg {

    using namespace cryptonote;
    using namespace crypto;
    using namespace std;

    using request = cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request;
    using response = cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response;
    using http_simple_client = epee::net_utils::http::http_simple_client;

    class page {


        MicroCore* mcore;
        Blockchain* core_storage;
        rpccalls rpc;

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
           // uint64_t height = core_storage->get_current_blockchain_height() - 1;

            uint64_t height = rpc.get_current_height() - 1;

            fmt::print("Current height: {:d}\n", height);

            // initalise page tempate map with basic info about blockchain
            mstch::map context {
                    {"refresh",          refresh_page},
                    {"height",           fmt::format("{:d}", height + 1)},
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
                vector<uint64_t> mixin_numbers = xmreg::get_mixin_no_in_txs(txs_in_blk);

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

            string mempool_html = mempool();

            context["mempool_info"] = mempool_html;

            // read index.html
            string index_html = xmreg::read(TMPL_INDEX);

            // add header and footer
            string full_page = get_full_page(index_html);

            // render the page
            return mstch::render(full_page, context);
        }


        string
        mempool()
        {
            string deamon_url {"http:://127.0.0.1:18081"};

            // perform RPC call to deamon to get
            // its transaction pull
            boost::mutex m_daemon_rpc_mutex;

            request req;
            response res;

            http_simple_client m_http_client;

            m_daemon_rpc_mutex.lock();

            bool r = epee::net_utils::invoke_http_json_remote_command2(
                    deamon_url + "/get_transaction_pool",
                    req, res, m_http_client, 200000);

            m_daemon_rpc_mutex.unlock();

            if (!r)
            {
                cerr << "Error connecting to Monero deamon at "
                     << deamon_url << endl;
                return "Error connecting to Monero deamon to get mempool";
            }

            // initalise page tempate map with basic info about blockchain
            mstch::map context {
                    {"mempool_size",  fmt::format("{:d}", res.transactions.size())},
                    {"mempooltxs" ,   mstch::array()}
            };

            // get reference to blocks template map to be field below
            mstch::array& txs = boost::get<mstch::array>(context["mempooltxs"]);

            // for each transaction in the memory pool
            for (size_t i = 0; i < res.transactions.size(); ++i)
            {
                // get transaction info of the tx in the mempool
                cryptonote::tx_info _tx_info = res.transactions.at(i);

                // display basic info
                //fmt::print("Tx hash: {:s}\n", _tx_info.id_hash);

               // fmt::print("Fee: {:0.10f} xmr, size {:d} bytes\n",
                 //     XMR_AMOUNT(_tx_info.fee),
                //      _tx_info.blob_size);

               // fmt::print("Receive time: {:s}\n",
               //       xmreg::timestamp_to_str(_tx_info.receive_time));

                uint64_t sum_inputs = sum_xmr_inputs(_tx_info.tx_json);
                uint64_t sum_outputs = sum_xmr_outputs(_tx_info.tx_json);


                // get mixin number in each transaction
                vector<uint64_t> mixin_numbers = get_mixin_no_in_txs(_tx_info.tx_json);

                // set output page template map
                txs.push_back(mstch::map {
                        {"timestamp"   , xmreg::timestamp_to_str(_tx_info.receive_time)},
                        {"hash"        , fmt::format("<{:s}>", _tx_info.id_hash)},
                        {"fee"         , fmt::format("{:0.4f}", XMR_AMOUNT(_tx_info.fee))},
                        {"xmr_inputs" , fmt::format("{:0.4f}", XMR_AMOUNT(sum_inputs))},
                        {"xmr_outputs" , fmt::format("{:0.4f}", XMR_AMOUNT(sum_outputs))},
                        {"mixin" , fmt::format("{:d}", mixin_numbers.at(0))}
                });
            }

            // read index.html
            string mempool_html = xmreg::read(TMPL_MEMPOOL);

            // render the page
            return mstch::render(mempool_html, context);
        }


    private:

        uint64_t
        sum_xmr_outputs(const string& json_str)
        {
            uint64_t sum_xmr {0};

            rapidjson::Document json;

            if (json.Parse(json_str.c_str()).HasParseError())
            {
                cerr << "Failed to parse JSON" << endl;
                return 0;
            }


            // get information about outputs
            const rapidjson::Value& vout = json["vout"];

            if (vout.IsArray())
            {
               // print("Outputs:\n");

                for (rapidjson::SizeType i = 0; i < vout.Size(); ++i)
                {
                    //print(" - {:s}, {:0.8f} xmr\n",
                      //    vout[i]["target"]["key"].GetString(),
                      //    XMR_AMOUNT(vout[i]["amount"].GetUint64()));

                    sum_xmr += vout[i]["amount"].GetUint64();
                }
            }

            return sum_xmr;
        }

        uint64_t
        sum_xmr_inputs(const string& json_str)
        {
            uint64_t sum_xmr {0};

            rapidjson::Document json;

            if (json.Parse(json_str.c_str()).HasParseError())
            {
                cerr << "Failed to parse JSON" << endl;
                return 0;
            }


            // get information about inputs
            const rapidjson::Value& vin = json["vin"];

            if (vin.IsArray())
            {
                // print("Input key images:\n");

                for (rapidjson::SizeType i = 0; i < vin.Size(); ++i)
                {
                    if (vin[i].HasMember("key"))
                    {
                        const rapidjson::Value& key_img = vin[i]["key"];

                        // print(" - {:s}, {:0.8f} xmr\n",
                        //       key_img["k_image"].GetString(),
                        //       XMR_AMOUNT(key_img["amount"].GetUint64()));

                        sum_xmr += key_img["amount"].GetUint64();
                    }
                }
            }

            return sum_xmr;
        }

        vector<uint64_t>
        get_mixin_no_in_txs(const string& json_str)
        {
            vector<uint64_t> mixin_no;

            rapidjson::Document json;

            if (json.Parse(json_str.c_str()).HasParseError())
            {
                cerr << "Failed to parse JSON" << endl;
                return mixin_no;
            }

            // get information about inputs
            const rapidjson::Value& vin = json["vin"];

            if (vin.IsArray())
            {
               // print("Input key images:\n");

                for (rapidjson::SizeType i = 0; i < vin.Size(); ++i)
                {
                    if (vin[i].HasMember("key"))
                    {
                        const rapidjson::Value& key_img = vin[i]["key"];

                        // print(" - {:s}, {:0.8f} xmr\n",
                        //       key_img["k_image"].GetString(),
                        //       XMR_AMOUNT(key_img["amount"].GetUint64()));


                        mixin_no.push_back(key_img["key_offsets"].Size());
                    }
                }
            }

            return mixin_no;
        }

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

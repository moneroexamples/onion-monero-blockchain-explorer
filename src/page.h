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
#define TMPL_BLOCK   TMPL_DIR "/block.html"



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
        time_t server_timestamp;

    public:

        page(MicroCore* _mcore, Blockchain* _core_storage, string deamon_url)
                : mcore {_mcore},
                  core_storage {_core_storage},
                  rpc {deamon_url},
                  server_timestamp {std::time(nullptr)}
        {
        }

        string
        index(uint64_t page_no = 0, bool refresh_page = false)
        {
            // connect to the deamon if not yet connected
            bool is_connected = rpc.connect_to_monero_deamon();

            if (!is_connected)
            {
                cerr << "Connection to the Monero demon does not exist or was lost!" << endl;
                return "Connection to the Monero demon does not exist or was lost!";
            }

            //get current server timestamp
            server_timestamp = std::time(nullptr);

            // number of last blocks to show
            uint64_t no_of_last_blocks {100 + 1};

            uint64_t height = rpc.get_current_height() - 1;

            // initalise page tempate map with basic info about blockchain
            mstch::map context {
                    {"refresh"         , refresh_page},
                    {"height"          , fmt::format("{:d}", height)},
                    {"server_timestamp", xmreg::timestamp_to_str(server_timestamp)},
                    {"blocks"          , mstch::array()},
                    {"age_format"      , string("[h:m:d]")},
                    {"page_no"         , fmt::format("{:d}", page_no)},
                    {"total_page_no"   , fmt::format("{:d}", height / (no_of_last_blocks))},
                    {"is_page_zero"    , !bool(page_no)},
                    {"next_page"       , fmt::format("{:d}", page_no + 1)},
                    {"prev_page"       , fmt::format("{:d}", (page_no > 0 ? page_no - 1 : 0))}
            };


            // get reference to blocks template map to be field below
            mstch::array& blocks = boost::get<mstch::array>(context["blocks"]);

            // calculate starting and ending block numbers to show
            uint64_t start_height = height - no_of_last_blocks * (page_no + 1);
            uint64_t end_height   = height - no_of_last_blocks * (page_no);

            // check few conditions to make sure we are whithin the avaliable range
            //@TODO its too messed up. needs to find cleaner way.
            start_height = start_height > 0      ? start_height : 0;
            end_height   = end_height   < height ? end_height   : height;
            start_height = start_height > end_height ? 0 : start_height;
            end_height   = end_height - start_height > no_of_last_blocks
                           ? no_of_last_blocks : end_height;

            // iterate over last no_of_last_blocks of blocks
            for (uint64_t i = start_height; i <= end_height; ++i)
            {
                // get block at the given height i
                block blk;

                if (!mcore->get_block_by_height(i, blk))
                {
                    cerr << "Cant get block: " << i << endl;
                    continue;
                }

                // get block's hash
                crypto::hash blk_hash = core_storage->get_block_id_by_height(i);

                // remove "<" and ">" from the hash string
                string blk_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", blk_hash));

                // get block timestamp in user friendly format
                string timestamp_str = xmreg::timestamp_to_str(blk.timestamp);

                pair<string, string> age = get_age(server_timestamp, blk.timestamp);

                context["age_format"] = age.second;

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

                // mixing format for the templates
                auto mixin_format = [=]() -> mstch::node
                {
                    if (mixin_min < 0)
                    {
                        return string("N/A");
                    }
                    return fmt::format("{:d}-{:d}", mixin_min - 1, mixin_max - 1);
                };

                // get block size in bytes
                uint64_t blk_size = get_object_blobsize(blk);

                // set output page template map
                blocks.push_back(mstch::map {
                        {"height"      , to_string(i)},
                        {"timestamp"   , timestamp_str},
                        {"age"         , age.first},
                        {"hash"        , blk_hash_str},
                        {"block_reward", fmt::format("{:0.4f}/{:0.4f}",
                                                     XMR_AMOUNT(coinbase_tx[1] - sum_fees),
                                                     XMR_AMOUNT(sum_fees))},
                        {"notx"        , fmt::format("{:d}", blk.tx_hashes.size())},
                        {"xmr_inputs"  , fmt::format("{:0.2f}",
                                                     XMR_AMOUNT(sum_xmr_in_out[0]))},
                        {"xmr_outputs" , fmt::format("{:0.2f}",
                                                     XMR_AMOUNT(sum_xmr_in_out[1]))},
                        {"mixin_range" , mstch::lambda {mixin_format}},
                        {"blksize"     , fmt::format("{:0.2f}",
                                                     static_cast<double>(blk_size) / 1024.0)}
                });
            } //  for (uint64_t i = start_height; i <= end_height; ++i)

            // reverse blocks and remove last (i.e., oldest)
            // block. This is done so that time delats
            // are easier to calcualte in the above for loop
            std::reverse(blocks.begin(), blocks.end());
            //blocks.pop_back();

            // get memory pool rendered template
            string mempool_html = mempool();

            // append mempool_html to the index context map
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
            std::vector<tx_info> mempool_txs;

            if (!rpc.get_mempool(mempool_txs))
            {
              return "Getting mempool failed";
            }

            // initalise page tempate map with basic info about mempool
            mstch::map context {
                    {"mempool_size",  fmt::format("{:d}", mempool_txs.size())},
                    {"mempooltxs" ,   mstch::array()}
            };

            // get reference to blocks template map to be field below
            mstch::array& txs = boost::get<mstch::array>(context["mempooltxs"]);

            // for each transaction in the memory pool
            for (size_t i = 0; i < mempool_txs.size(); ++i)
            {
                // get transaction info of the tx in the mempool
                tx_info _tx_info = mempool_txs.at(i);

                // calculate difference between tx in mempool and server timestamps
                array<size_t, 5> delta_time = timestamp_difference(
                                              server_timestamp,
                                              _tx_info.receive_time);

                // use only hours, so if we have days, add
                // it to hours
                uint64_t delta_hours {delta_time[1]*24 + delta_time[2]};

                string age_str = fmt::format("{:02d}:{:02d}:{:02d}",
                                             delta_hours,
                                             delta_time[3], delta_time[4]);

                // if more than 99 hourse, change formating
                // for the template
                if (delta_hours > 99)
                {
                    age_str = fmt::format("{:03d}:{:02d}:{:02d}",
                                             delta_hours,
                                             delta_time[3], delta_time[4]);
                }

                // sum xmr in inputs and ouputs in the given tx
                uint64_t sum_inputs  = sum_xmr_inputs(_tx_info.tx_json);
                uint64_t sum_outputs = sum_xmr_outputs(_tx_info.tx_json);

                // get mixin number in each transaction
                vector<uint64_t> mixin_numbers = get_mixin_no_in_txs(_tx_info.tx_json);

                // set output page template map
                txs.push_back(mstch::map {
                        {"timestamp"     , xmreg::timestamp_to_str(_tx_info.receive_time)},
                        {"age"           , age_str},
                        {"hash"          , fmt::format("{:s}", _tx_info.id_hash)},
                        {"fee"           , fmt::format("{:0.4f}", XMR_AMOUNT(_tx_info.fee))},
                        {"xmr_inputs"    , fmt::format("{:0.2f}", XMR_AMOUNT(sum_inputs))},
                        {"xmr_outputs"   , fmt::format("{:0.2f}", XMR_AMOUNT(sum_outputs))},
                        {"mixin"         , fmt::format("{:d}", mixin_numbers.at(0) - 1)},
                        {"txsize"        , fmt::format("{:0.2f}", static_cast<double>(_tx_info.blob_size)/1024.0)}
                });
            }

            // read index.html
            string mempool_html = xmreg::read(TMPL_MEMPOOL);

            // render the page
            return mstch::render(mempool_html, context);
        }


        string
        show_block(uint64_t _blk_height)
        {
            // get block at the given height i
            block blk;

            if (!mcore->get_block_by_height(_blk_height, blk))
            {
                cerr << "Cant get block: " << _blk_height << endl;
                return fmt::format("Block of height {:d} not found!", _blk_height);
            }

            // get block's hash
            crypto::hash blk_hash = core_storage->get_block_id_by_height(_blk_height);

            crypto::hash prev_hash = blk.prev_id;
            crypto::hash next_hash = core_storage->get_block_id_by_height(_blk_height + 1);

            bool have_next_hash = (next_hash == null_hash ? false : true);
            bool have_prev_hash = (prev_hash == null_hash ? false : true);

            // remove "<" and ">" from the hash string
            string prev_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", prev_hash));
            string next_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", next_hash));

            // remove "<" and ">" from the hash string
            string blk_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", blk_hash));

            // get block timestamp in user friendly format
            string blk_timestamp = xmreg::timestamp_to_str(blk.timestamp);

            // get age of the block relative to the server time
            pair<string, string> age = get_age(server_timestamp, blk.timestamp);

            // get time from the last block
            string delta_time {"N/A"};

            if (have_prev_hash)
            {
                block prev_blk = core_storage->get_db().get_block(prev_hash);

                pair<string, string> delta_diff = get_age(blk.timestamp, prev_blk.timestamp);

                delta_time = delta_diff.first;
            }

            // get block size in bytes
            uint64_t blk_size = get_object_blobsize(blk);

            // miner reward tx
            transaction coinbase_tx = blk.miner_tx;

            // transcation in the block
            vector<crypto::hash> tx_hashes = blk.tx_hashes;

            bool have_txs = !blk.tx_hashes.empty();

            // initalise page tempate map with basic info about blockchain
            mstch::map context {
                    {"blk_hash"       , blk_hash_str},
                    {"blk_height"     , fmt::format("{:d}", _blk_height)},
                    {"blk_timestamp"  , blk_timestamp},
                    {"prev_hash"      , prev_hash_str},
                    {"next_hash"      , next_hash_str},
                    {"have_next_hash" , have_next_hash},
                    {"have_prev_hash" , have_prev_hash},
                    {"blk_age"        , age.first},
                    {"delta_time"     , delta_time},
                    {"blk_nonce"      , std::to_string(blk.nonce)},
                    {"age_format"     , age.second},
                    {"major_ver"      , std::to_string(blk.major_version)},
                    {"minor_ver"      , std::to_string(blk.minor_version)},
                    {"blk_size"       , fmt::format("{:0.4f}",
                                                    static_cast<double>(blk_size) / 1024.0)}
            };

            // read block.html
            string block_html = xmreg::read(TMPL_BLOCK);

            // add header and footer
            string full_page = get_full_page(block_html);

            // render the page
            return mstch::render(full_page, context);
        }

        string
        show_block(string _blk_hash)
        {
            crypto::hash blk_hash;

            if (!xmreg::parse_str_secret_key(_blk_hash, blk_hash))
            {
                cerr << "Cant parse blk hash: " << blk_hash << endl;
                return fmt::format("Cant parse block hash {:s}!", blk_hash);
            }

            uint64_t blk_height;

            try
            {
                blk_height = core_storage->get_db().get_block_height(blk_hash);
            }
            catch (const BLOCK_DNE& e)
            {
                cerr << "Block does not exist: " << blk_hash << endl;
                return fmt::format("Block of hash {:s} does not exist!", blk_hash);
            }
            catch (const std::exception& e)
            {
                cerr << "Cant get block: " << blk_hash << endl;
                return fmt::format("Block of hash {:s} not found!", blk_hash);
            }

            return show_block(blk_height);
        }


    private:

        pair<string, string>
        get_age(uint64_t timestamp1, uint64_t timestamp2)
        {


            pair<string, string> age_pair;

            // calculate difference between server and block timestamps
            array<size_t, 5> delta_time = timestamp_difference(
                    timestamp1, timestamp2);

            // default format for age
            string age_str = fmt::format("{:02d}:{:02d}:{:02d}",
                                         delta_time[2], delta_time[3],
                                         delta_time[4]);

            string age_format {"[h:m:s]"};

            // if have days or years, change age format
            if (delta_time[0] > 0)
            {
                age_str = fmt::format("{:02d}:{:02d}:{:02d}:{:02d}:{:02d}",
                                      delta_time[0], delta_time[1], delta_time[2],
                                      delta_time[3], delta_time[4]);

                age_format = string("[y:d:h:m:s]");
            }
            else if (delta_time[1] > 0)
            {
                age_str = fmt::format("{:02d}:{:02d}:{:02d}:{:02d}",
                                      delta_time[1], delta_time[2],
                                      delta_time[3], delta_time[4]);

                age_format = string("[d:h:m:s]");
            }

            age_pair.first  = age_str;
            age_pair.second = age_format;

            return age_pair;
        }

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

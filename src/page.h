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
#include <limits>
#include <ctime>

#define TMPL_DIR              "./templates"
#define TMPL_INDEX   TMPL_DIR "/index.html"
#define TMPL_MEMPOOL TMPL_DIR "/mempool.html"
#define TMPL_HEADER  TMPL_DIR "/header.html"
#define TMPL_FOOTER  TMPL_DIR "/footer.html"
#define TMPL_BLOCK   TMPL_DIR "/block.html"
#define TMPL_TX      TMPL_DIR "/tx.html"


namespace xmreg {

    using namespace cryptonote;
    using namespace crypto;
    using namespace std;

    /**
     * @brief The tx_details struct
     *
     * Basic information about tx
     *
     */
    struct tx_details
    {
        crypto::hash hash;
        crypto::public_key pk;
        uint64_t xmr_inputs;
        uint64_t xmr_outputs;
        uint64_t fee;
        uint64_t mixin_no;
        uint64_t size;
        size_t version;
        uint64_t unlock_time;

        // key images of inputs
        vector<txin_to_key> input_key_imgs;

        // public keys and xmr amount of outputs
        vector<pair<txout_to_key, uint64_t>> output_pub_keys;


        mstch::map
        get_mstch_map()
        {
            // remove "<" and ">" from the hash string
            string tx_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", hash));

            string tx_pk_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", pk));

            return mstch::map {
                    {"hash"          , tx_hash_str},
                    {"pub_key"       , tx_pk_str},
                    {"tx_fee"        , fmt::format("{:0.6f}", XMR_AMOUNT(fee))},
                    {"sum_inputs"    , fmt::format("{:0.6f}", XMR_AMOUNT(xmr_inputs))},
                    {"sum_outputs"   , fmt::format("{:0.6f}", XMR_AMOUNT(xmr_outputs))},
                    {"no_inputs"     , input_key_imgs.size()},
                    {"no_outputs"    , output_pub_keys.size()},
                    {"mixin"         , std::to_string(mixin_no - 1)},
                    {"version"       , std::to_string(version)},
                    {"unlock_time"   , std::to_string(unlock_time)},
                    {"tx_size"       , fmt::format("{:0.4f}", static_cast<double>(size)/1024.0)}
            };
        }
    };

    class page {

        static const bool FULL_AGE_FORMAT {true};

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
                    {"height"          , std::to_string(height)},
                    {"server_timestamp", xmreg::timestamp_to_str(server_timestamp)},
                    {"blocks"          , mstch::array()},
                    {"age_format"      , string("[h:m:d]")},
                    {"page_no"         , std::to_string(page_no)},
                    {"total_page_no"   , std::to_string(height / (no_of_last_blocks))},
                    {"is_page_zero"    , !bool(page_no)},
                    {"next_page"       , std::to_string(page_no + 1)},
                    {"prev_page"       , std::to_string((page_no > 0 ? page_no - 1 : 0))}
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

            // previous blk timestamp, initalised to lowest possible value
            double prev_blk_timestamp {std::numeric_limits<double>::lowest()};

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

                // get time difference [m] between previous and current blocks
                string time_delta_str {};

                if (prev_blk_timestamp > std::numeric_limits<double>::lowest())
                {
                  time_delta_str = fmt::format("{:0.2f}",
                      (double(blk.timestamp) - double(prev_blk_timestamp))/60.0);
                }

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
                        {"time_delta"  , time_delta_str},
                        {"age"         , age.first},
                        {"hash"        , blk_hash_str},
                        {"block_reward", fmt::format("{:0.4f}/{:0.4f}",
                                                     XMR_AMOUNT(coinbase_tx[1] - sum_fees),
                                                     XMR_AMOUNT(sum_fees))},
                        {"fees"        , fmt::format("{:0.3f}", XMR_AMOUNT(sum_fees))},
                        {"notx"        , fmt::format("{:d}", blk.tx_hashes.size())},
                        {"xmr_inputs"  , fmt::format("{:0.2f}",
                                                     XMR_AMOUNT(sum_xmr_in_out[0]))},
                        {"xmr_outputs" , fmt::format("{:0.2f}",
                                                     XMR_AMOUNT(sum_xmr_in_out[1]))},
                        {"mixin_range" , mstch::lambda {mixin_format}},
                        {"blksize"     , fmt::format("{:0.2f}",
                                                     static_cast<double>(blk_size) / 1024.0)}
                });

                // save current's block timestamp as reference for the next one
                prev_blk_timestamp  = static_cast<double>(blk.timestamp);

            } //  for (uint64_t i = start_height; i <= end_height; ++i)

            // reverse blocks and remove last (i.e., oldest)
            // block. This is done so that time delats
            // are easier to calcualte in the above for loop
            std::reverse(blocks.begin(), blocks.end());

            // if we look at the genesis time, we should not remove
            // the last block, i.e. genesis one.
            if (!(start_height < 2))
            {
              blocks.pop_back();
            }

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
                    {"mempool_size",  std::to_string(mempool_txs.size())},
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
                        {"fee"           , fmt::format("{:0.3f}", XMR_AMOUNT(_tx_info.fee))},
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

            // sum of all transactions in the block
            uint64_t sum_fees = 0;

            // get tx details for the coinbase tx, i.e., miners reward
            tx_details txd_coinbase = get_tx_details(blk.miner_tx, true);

            // initalise page tempate map with basic info about blockchain
            mstch::map context {
                    {"blk_hash"       , blk_hash_str},
                    {"blk_height"     , _blk_height},
                    {"blk_timestamp"  , blk_timestamp},
                    {"prev_hash"      , prev_hash_str},
                    {"next_hash"      , next_hash_str},
                    {"have_next_hash" , have_next_hash},
                    {"have_prev_hash" , have_prev_hash},
                    {"have_txs"       , have_txs},
                    {"no_txs"         , std::to_string(blk.tx_hashes.size())},
                    {"blk_age"        , age.first},
                    {"delta_time"     , delta_time},
                    {"blk_nonce"      , blk.nonce},
                    {"age_format"     , age.second},
                    {"major_ver"      , std::to_string(blk.major_version)},
                    {"minor_ver"      , std::to_string(blk.minor_version)},
                    {"blk_size"       , fmt::format("{:0.4f}",
                                                    static_cast<double>(blk_size) / 1024.0)},
                    {"coinbase_txs"   , mstch::array{{txd_coinbase.get_mstch_map()}}},
                    {"blk_txs"        , mstch::array()}
            };

            // .push_back(txd_coinbase.get_mstch_map()

           // boost::get<mstch::array>(context["blk_txs"]).push_back(txd_coinbase.get_mstch_map());

            // now process nomral transactions
            // get reference to blocks template map to be field below
            mstch::array& txs = boost::get<mstch::array>(context["blk_txs"]);

            // timescale representation for each tx in the block
            vector<string> mixin_timescales_str;

            // for each transaction in the memory pool
            for (size_t i = 0; i < blk.tx_hashes.size(); ++i)
            {
                // get transaction info of the tx in the mempool
                const crypto::hash& tx_hash = blk.tx_hashes.at(i);

                // remove "<" and ">" from the hash string
                string tx_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", tx_hash));


                // get transaction
                transaction tx;

                if (!mcore->get_tx(tx_hash, tx))
                {
                    cerr << "Cant get tx: " << tx_hash << endl;
                    continue;
                }

                tx_details txd = get_tx_details(tx);

                // add fee to the rest
                sum_fees += txd.fee;


                // get mixins in time scale for visual representation
                //string mixin_times_scale = xmreg::timestamps_time_scale(mixin_timestamps,
                //                                                        server_timestamp);


                // add tx details mstch map to context
                txs.push_back(txd.get_mstch_map());
            }


            // add total fees in the block to the context
            context["sum_fees"]   = fmt::format("{:0.6f}",
                                         XMR_AMOUNT(sum_fees));

            // get xmr in the block reward
            context["blk_reward"] = fmt::format("{:0.6f}",
                                         XMR_AMOUNT(txd_coinbase.xmr_outputs - sum_fees));

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

        string
        show_tx(string tx_hash_str)
        {

            // parse tx hash string to hash object
            crypto::hash tx_hash;

            if (!xmreg::parse_str_secret_key(tx_hash_str, tx_hash))
            {
                cerr << "Cant parse tx hash: " << tx_hash_str << endl;
                return string("Cant parse tx hash: " + tx_hash_str);
            }

            // get transaction
            transaction tx;

            if (!mcore->get_tx(tx_hash, tx))
            {
                cerr << "Cant get tx: " << tx_hash << endl;
                return string("Cant get tx: " + tx_hash_str);
            }

            tx_details txd = get_tx_details(tx);

            uint64_t _blk_height {0};

            // initalise page tempate map with basic info about blockchain
            mstch::map context {
                    {"tx_hash"        , tx_hash_str},
                    {"blk_height"     , _blk_height},
                    {"inputs_no"      , txd.input_key_imgs.size()},
                    {"outputs_no"     , txd.output_pub_keys.size()}
            };

            string server_time_str = xmreg::timestamp_to_str(server_timestamp, "%F");


            mstch::array inputs = mstch::array{};

            mstch::array mixins_timescales;
            double timescale_scale {0.0}; // size of one '_' in days


            uint64_t input_idx {0};

            // make timescale maps for mixins in input
            for (const txin_to_key& in_key: txd.input_key_imgs)
            {
                // get absolute offsets of mixins
                std::vector<uint64_t> absolute_offsets
                        = cryptonote::relative_output_offsets_to_absolute(
                                in_key.key_offsets);

                // get public keys of outputs used in the mixins that match to the offests
                std::vector<cryptonote::output_data_t> outputs;
                core_storage->get_db().get_output_key(in_key.amount,
                                                      absolute_offsets,
                                                      outputs);

                vector<uint64_t> mixin_timestamps;



                inputs.push_back(mstch::map {
                    {"in_key_img", REMOVE_HASH_BRAKETS(fmt::format("{:s}", in_key.k_image))},
                    {"amount"    , fmt::format("{:0.12f}", XMR_AMOUNT(in_key.amount))},
                    {"input_idx" , fmt::format("{:02d}", input_idx++)},
                    {"mixins"    , mstch::array{}},
                });

                // get reference to mixins array created above
                mstch::array& mixins = boost::get<mstch::array>(
                            boost::get<mstch::map>(inputs.back())["mixins"]);

                size_t count = 0;

                // for each found output public key find its block to get timestamp
                for (const uint64_t &i: absolute_offsets)
                {
                    // get basic information about mixn's output
                    cryptonote::output_data_t output_data = outputs.at(count);

                    // get pair pair<crypto::hash, uint64_t> where first is tx hash
                    // and second is local index of the output i in that tx
                    tx_out_index tx_out_idx =
                            core_storage->get_db().get_output_tx_and_index(in_key.amount, i);

                    // get block of given height, as we want to get its timestamp
                    cryptonote::block blk;

                    if (!mcore->get_block_by_height(output_data.height, blk))
                    {
                        cerr << "- cant get block of height: " << output_data.height << endl;
                        return fmt::format("- cant get block of height: {}\n", output_data.height);
                    }

                    pair<string, string> mixin_age = get_age(server_timestamp,
                                                             blk.timestamp,
                                                             FULL_AGE_FORMAT);

                    mixins.push_back(mstch::map {
                            {"mix_blk"        , fmt::format("{:08d}", output_data.height)},
                            {"mix_pub_key"    , REMOVE_HASH_BRAKETS(fmt::format("{:s}", output_data.pubkey))},
                            {"mix_tx_hash"    , REMOVE_HASH_BRAKETS(fmt::format("{:s}", tx_out_idx.first))},
                            {"mix_out_indx"   , fmt::format("{:d}", tx_out_idx.second)},
                            {"mix_timestamp"  , xmreg::timestamp_to_str(blk.timestamp)},
                            {"mix_age"        , mixin_age.first},
                            {"mix_age_format" , mixin_age.second},
                            {"mix_idx"        , fmt::format("{:02d}", count)},
                    });

                    // get mixin timestamp from its orginal block
                    mixin_timestamps.push_back(blk.timestamp);

                    ++count;
                }

                // get mixins in time scale for visual representation
                pair<string, double> mixin_times_scale = xmreg::timestamps_time_scale(
                                                            mixin_timestamps,
                                                            server_timestamp, 165);

                // save resolution of mixin timescales
                timescale_scale = mixin_times_scale.second;

                // save the string timescales for later to show
                mixins_timescales.push_back(mstch::map {
                    {"timescale", mixin_times_scale.first}});
            }

            context["server_time"]      = server_time_str;
            context["inputs"]           = inputs;
            context["timescales"]       = mixins_timescales;
            context["timescales_scale"] = fmt::format("{:0.2f}",
                                                timescale_scale / 3600.0 / 24.0); // in days


            uint64_t output_idx {0};

            mstch::array outputs;

            for (pair<txout_to_key, uint64_t>& outp: txd.output_pub_keys)
            {
                outputs.push_back(mstch::map {
                      {"out_pub_key"   , REMOVE_HASH_BRAKETS(fmt::format("{:s}", outp.first.key))},
                      {"amount"        , fmt::format("{:0.12f}", XMR_AMOUNT(outp.second))},
                      {"output_idx"    , fmt::format("{:02d}", output_idx++)}
                });
            }

            context["outputs"] = outputs;


            // read tx.html
            string tx_html = xmreg::read(TMPL_TX);

            // add header and footer
            string full_page = get_full_page(tx_html);

            // render the page
            return mstch::render(full_page, context);
        }


    private:


        tx_details
        get_tx_details(const transaction& tx, bool coinbase = false)
        {
            tx_details txd;

            // get tx hash
            txd.hash = get_transaction_hash(tx);

            // get tx public key
            txd.pk = get_tx_pub_key_from_extra(tx);

            // sum xmr in inputs and ouputs in the given tx
            txd.xmr_inputs  = sum_money_in_inputs(tx);
            txd.xmr_outputs = sum_money_in_outputs(tx);

            // get mixin number
            txd.mixin_no    = get_mixin_no(tx);

            if (!coinbase)
            {
                // get tx fee
                txd.fee = get_tx_fee(tx);
            }

            // get tx size in bytes
            txd.size = get_object_blobsize(tx);

            txd.input_key_imgs  = get_key_images(tx);
            txd.output_pub_keys = get_ouputs(tx);


            // get tx version
            txd.version = tx.version;

            // get unlock time
            txd.unlock_time = tx.unlock_time;

            return txd;
        }

        pair<string, string>
        get_age(uint64_t timestamp1, uint64_t timestamp2, bool full_format = 0)
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
            if (delta_time[0] > 0 || full_format == true)
            {
                age_str = fmt::format("{:02d}:{:03d}:{:02d}:{:02d}:{:02d}",
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

//
// Created by mwo on 8/04/16.
//

#ifndef CROWXMR_PAGE_H
#define CROWXMR_PAGE_H



#include "mstch/mstch.hpp"

#include "version.h"

#include "monero_headers.h"

#include "MicroCore.h"
#include "tools.h"
#include "rpccalls.h"
#include "mylmdb.h"
#include "../ext/crow/http_request.h"

#include "../ext/vpetrigocaches/cache.hpp"
#include "../ext/vpetrigocaches/lru_cache_policy.hpp"
#include "../ext/vpetrigocaches/fifo_cache_policy.hpp"

#include <algorithm>
#include <limits>
#include <ctime>

#define TMPL_DIR                    "./templates"
#define TMPL_PARIALS_DIR            TMPL_DIR "/partials"
#define TMPL_CSS_STYLES             TMPL_DIR "/css/style.css"
#define TMPL_INDEX                  TMPL_DIR "/index.html"
#define TMPL_INDEX2                 TMPL_DIR "/index2.html"
#define TMPL_MEMPOOL                TMPL_DIR "/mempool.html"
#define TMPL_HEADER                 TMPL_DIR "/header.html"
#define TMPL_FOOTER                 TMPL_DIR "/footer.html"
#define TMPL_BLOCK                  TMPL_DIR "/block.html"
#define TMPL_TX                     TMPL_DIR "/tx.html"
#define TMPL_ADDRESS                TMPL_DIR "/address.html"
#define TMPL_MY_OUTPUTS             TMPL_DIR "/my_outputs.html"
#define TMPL_SEARCH_RESULTS         TMPL_DIR "/search_results.html"
#define TMPL_MY_RAWTX               TMPL_DIR "/rawtx.html"
#define TMPL_MY_CHECKRAWTX          TMPL_DIR "/checkrawtx.html"
#define TMPL_MY_PUSHRAWTX           TMPL_DIR "/pushrawtx.html"
#define TMPL_MY_RAWKEYIMGS          TMPL_DIR "/rawkeyimgs.html"
#define TMPL_MY_CHECKRAWKEYIMGS     TMPL_DIR "/checkrawkeyimgs.html"
#define TMPL_MY_RAWOUTPUTKEYS       TMPL_DIR "/rawoutputkeys.html"
#define TMPL_MY_CHECKRAWOUTPUTKEYS  TMPL_DIR "/checkrawoutputkeys.html"

namespace xmreg
{


using namespace cryptonote;
using namespace crypto;
using namespace std;

// define a checker to test if a structure has "tx_blob"
// member variable. I use modified daemon with few extra
// bits and pieces here and there. One of them is
// tx_blob in cryptonote::tx_info structure
// thus I check if I run my version, or just
// generic one
DEFINE_MEMBER_CHECKER(tx_blob)

// define getter to get tx_blob, i.e., get_tx_blob function
// as string if exists. the getter return empty string if
// tx_blob does not exist
DEFINE_MEMBER_GETTER(tx_blob, string)



/**
 * Check if a given header filed contains value string
 *
 * @param req
 * @param field
 * @param value
 * @return string
 */
string
does_header_has(const crow::request& req,
                const string& field = "Accept",
                const string& value = "q=.2, */*; q=.2")
{
    string accept = req.get_header_value(field);

    if (!accept.empty())
    {
        if (accept.find(value) != std::string::npos)
        {
            return accept;
        }
    }

    return string {};
}



/**
 * @brief The tx_details struct
 *
 * Basic information about tx
 *
 */
struct tx_details
{
    crypto::hash hash;
    crypto::hash prefix_hash;
    crypto::public_key pk;
    uint64_t xmr_inputs;
    uint64_t xmr_outputs;
    uint64_t num_nonrct_inputs;
    uint64_t fee;
    uint64_t mixin_no;
    uint64_t size;
    uint64_t blk_height;
    size_t   version;
    uint64_t unlock_time;
    uint64_t no_confirmations;
    vector<uint8_t> extra;

    crypto::hash  payment_id  = null_hash; // normal
    crypto::hash8 payment_id8 = null_hash8; // encrypted

    string json_representation;

    std::vector<std::vector<crypto::signature> > signatures;

    // key images of inputs
    vector<txin_to_key> input_key_imgs;

    // public keys and xmr amount of outputs
    vector<pair<txout_to_key, uint64_t>> output_pub_keys;

    mstch::map
    get_mstch_map()
    {
        // remove "<" and ">" from the hash string
        string tx_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", hash));

        string tx_prefix_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", prefix_hash));

        string tx_pk_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", pk));

        //cout << "payment_id: " << payment_id << endl;

        string pid_str   = REMOVE_HASH_BRAKETS(fmt::format("{:s}", payment_id));
        string pid8_str  = REMOVE_HASH_BRAKETS(fmt::format("{:s}", payment_id8));

        string mixin_str {"N/A"};
        string fee_str {"N/A"};
        string fee_short_str {"N/A"};

        if (!input_key_imgs.empty())
        {
            mixin_str     = std::to_string(mixin_no - 1);
            fee_str       = fmt::format("{:0.6f}", XMR_AMOUNT(fee));
            fee_short_str = fmt::format("{:0.3f}", XMR_AMOUNT(fee));
        }


        //cout << "extra: " << extra_str << endl;

        mstch::map txd_map {
            {"hash"              , tx_hash_str},
            {"prefix_hash"       , tx_prefix_hash_str},
            {"pub_key"           , tx_pk_str},
            {"tx_fee"            , fee_str},
            {"tx_fee_short"      , fee_short_str},
            {"sum_inputs"        , xmr_amount_to_str(xmr_inputs , "{:0.6f}")},
            {"sum_outputs"       , xmr_amount_to_str(xmr_outputs, "{:0.6f}")},
            {"sum_inputs_short"  , xmr_amount_to_str(xmr_inputs , "{:0.3f}")},
            {"sum_outputs_short" , xmr_amount_to_str(xmr_outputs, "{:0.3f}")},
            {"no_inputs"         , static_cast<uint64_t>(input_key_imgs.size())},
            {"no_outputs"        , static_cast<uint64_t>(output_pub_keys.size())},
            {"no_nonrct_inputs"  , num_nonrct_inputs},
            {"mixin"             , mixin_str},
            {"blk_height"        , blk_height},
            {"version"           , std::to_string(version)},
            {"has_payment_id"    , payment_id  != null_hash},
            {"has_payment_id8"   , payment_id8 != null_hash8},
            {"payment_id"        , pid_str},
            {"confirmations"     , no_confirmations},
            {"extra"             , get_extra_str()},
            {"payment_id8"       , pid8_str},
            {"unlock_time"       , std::to_string(unlock_time)},
            {"tx_size"           , fmt::format("{:0.4f}",
                                               static_cast<double>(size)/1024.0)},
            {"tx_size_short"     , fmt::format("{:0.2f}",
                                               static_cast<double>(size)/1024.0)}
        };


        return txd_map;
    }


    string
    get_extra_str()
    {

        string extra_str = epee::string_tools::buff_to_hex_nodelimer(
                    string{reinterpret_cast<const char*>(extra.data()), extra.size()});

        return extra_str;
    }


    mstch::array
    get_ring_sig_for_input(uint64_t in_i)
    {
         mstch::array ring_sigs {};

         if (in_i >= signatures.size())
         {
             return ring_sigs;
         }

         for (const crypto::signature &sig: signatures.at(in_i))
         {
             ring_sigs.push_back(mstch::map{
                 {"ring_sig", print_signature(sig)}
             });
         }

         return ring_sigs;
    }

    string
    print_signature(const signature& sig)
    {
         stringstream ss;

         ss << epee::string_tools::pod_to_hex(sig.c)
            << epee::string_tools::pod_to_hex(sig.r);

         return ss.str();
    }
};

class page
{

    // check if we have tx_blob member in tx_info structure
    static const bool HAVE_TX_BLOB {
        HAS_MEMBER(cryptonote::tx_info, tx_blob)
    };

    static const bool FULL_AGE_FORMAT {true};

    MicroCore* mcore;
    Blockchain* core_storage;
    rpccalls rpc;

    atomic<time_t> server_timestamp;

    string lmdb2_path;

    bool testnet;

    bool enable_pusher;

    bool enable_key_image_checker;
    bool enable_output_key_checker;
    bool enable_mixins_details;




    bool enable_autorefresh_option;

    bool have_custom_lmdb;


    uint64_t no_of_mempool_tx_of_frontpage;
    uint64_t no_blocks_on_index;

    // instead of constatnly reading template files
    // from hard drive for each request, we can read
    // them only once, when the explorer starts into this map
    // this will improve performance of the explorer and reduce
    // read operation in OS
    map<string, string> template_file;


    // alias for easy class typing
    template <typename Key, typename Value>
    using lru_cache_t = caches::fixed_sized_cache<Key, Value, caches::LRUCachePolicy<Key>>;



    // alias for easy class typing
    template <typename Key, typename Value>
    using fifo_cache_t = caches::fixed_sized_cache<Key, Value, caches::FIFOCachePolicy<Key>>;


    // this struct is used to keep info about mempool
    // txs in FIFO cache. Should speed up processing
    // mempool txs for each request
    struct mempool_tx_info
    {
        json     j_tx;

        pair<uint64_t, uint64_t> sum_inputs;
        pair<uint64_t, uint64_t> sum_outputs;

        uint64_t num_nonrct_inputs;

        uint64_t mixin_no;

        string   is_ringct_str;
        string   rct_type_str;

        string hash;
        string fee;
        string xmr_inputs_str;
        string xmr_outputs_str;
        string timestamp;

        string mixin_str;
        string txsize;
    };

    // cache of txs in mempool, so that we dont
    // parse their json for each request
    fifo_cache_t<string, mempool_tx_info> mempool_tx_json_cache;

    // cache of txs_map of txs in blocks. this is useful for
    // index2 page, so that we dont parse txs in each block
    // for each request.
    fifo_cache_t<uint64_t, vector<pair<crypto::hash, mstch::node>>> block_tx_cache;

    // basic info about tx to be stored in cashe.
    // we need to store block_no and timestamp,
    // as this time and number of confirmation needs
    // to be updated between requests. Just cant
    // get it from cash, as it will be old very soon
    struct tx_info_cache
    {
        uint64_t   block_no;
        uint64_t   timestamp;
        mstch::map tx_map;
    };

    lru_cache_t<crypto::hash, tx_info_cache> tx_context_cache;


public:

    page(MicroCore* _mcore, Blockchain* _core_storage,
         string _deamon_url, string _lmdb2_path,
         bool _testnet, bool _enable_pusher,
         bool _enable_key_image_checker,
         bool _enable_output_key_checker,
         bool _enable_autorefresh_option,
         bool _enable_mixins_details,
         uint64_t _no_blocks_on_index)
            : mcore {_mcore},
              core_storage {_core_storage},
              rpc {_deamon_url},
              server_timestamp {std::time(nullptr)},
              lmdb2_path {_lmdb2_path},
              testnet {_testnet},
              enable_pusher {_enable_pusher},
              have_custom_lmdb {false},
              enable_key_image_checker {_enable_key_image_checker},
              enable_output_key_checker {_enable_output_key_checker},
              enable_autorefresh_option {_enable_autorefresh_option},
              enable_mixins_details {_enable_mixins_details},
              no_blocks_on_index {_no_blocks_on_index},
              mempool_tx_json_cache(500),
              block_tx_cache(100),
              tx_context_cache(1000)
    {

        no_of_mempool_tx_of_frontpage = 25;

        // just moneky patching now, to check
        // if custom lmdb database exist, so that
        // we can search for, e.g., key images,
        // payments ids. try to open this database.
        // if it fails, we assume it does not exist.
        // this is ugly check, but will do for now.
        // it does not even check if this custom lmdb
        // is up to date.
        try
        {
            unique_ptr<xmreg::MyLMDB> mylmdb;

            if (bf::is_directory(lmdb2_path))
            {
                mylmdb = make_unique<xmreg::MyLMDB>(lmdb2_path);

                // if we got to here, it seems that database exist
                have_custom_lmdb = true;
            }

        }
        catch (const std::exception& e)
        {
            cerr << "Custom lmdb databse seem not to exist. Its not big deal. "
                    "Just some searches wont be possible"
                 << endl;
        }


        // read template files for all the pages
        // into template_file map

        template_file["css_styles"]      = xmreg::read(TMPL_CSS_STYLES);
        template_file["header"]          = xmreg::read(TMPL_HEADER);
        template_file["footer"]          = get_footer();
        template_file["index2"]          = get_full_page(xmreg::read(TMPL_INDEX2));
        template_file["mempool"]         = xmreg::read(TMPL_MEMPOOL);
        template_file["mempool_full"]    = get_full_page(template_file["mempool"]);
        template_file["block"]           = get_full_page(xmreg::read(TMPL_BLOCK));
        template_file["tx"]              = get_full_page(xmreg::read(TMPL_TX));
        template_file["my_outputs"]      = get_full_page(xmreg::read(TMPL_MY_OUTPUTS));
        template_file["rawtx"]           = get_full_page(xmreg::read(TMPL_MY_RAWTX));
        template_file["checkrawtx"]      = get_full_page(xmreg::read(TMPL_MY_CHECKRAWTX));
        template_file["pushrawtx"]       = get_full_page(xmreg::read(TMPL_MY_PUSHRAWTX));
        template_file["rawkeyimgs"]      = get_full_page(xmreg::read(TMPL_MY_RAWKEYIMGS));
        template_file["rawoutputkeys"]   = get_full_page(xmreg::read(TMPL_MY_RAWOUTPUTKEYS));
        template_file["checkrawkeyimgs"] = get_full_page(xmreg::read(TMPL_MY_CHECKRAWKEYIMGS));
        template_file["checkoutputkeys"] = get_full_page(xmreg::read(TMPL_MY_CHECKRAWOUTPUTKEYS));
        template_file["address"]         = get_full_page(xmreg::read(TMPL_ADDRESS));
        template_file["search_results"]  = get_full_page(xmreg::read(TMPL_SEARCH_RESULTS));
        template_file["tx_details"]      = xmreg::read(string(TMPL_PARIALS_DIR) + "/tx_details.html");
        template_file["tx_table_head"]   = xmreg::read(string(TMPL_PARIALS_DIR) + "/tx_table_head.html");
        template_file["tx_table_row"]    = xmreg::read(string(TMPL_PARIALS_DIR) + "/tx_table_row.html");
    

    }

    /**
     * @brief show recent transactions and mempool
     * @param page_no block page to show
     * @param refresh_page enable autorefresh
     * @return rendered index page
     */
    string
    index2(uint64_t page_no = 0, bool refresh_page = false)
    {
        //get current server timestamp
        server_timestamp = std::time(nullptr);

        // number of last blocks to show
        uint64_t no_of_last_blocks {no_blocks_on_index + 1};

        // get the current blockchain height. Just to check
        uint64_t height = core_storage->get_current_blockchain_height();

        // initalise page tempate map with basic info about blockchain
        mstch::map context {
                {"testnet"                  , testnet},
                {"have_custom_lmdb"         , have_custom_lmdb},
                {"refresh"                  , refresh_page},
                {"height"                   , height},
                {"server_timestamp"         , xmreg::timestamp_to_str(server_timestamp)},
                {"age_format"               , string("[h:m:d]")},
                {"page_no"                  , page_no},
                {"total_page_no"            , (height / no_of_last_blocks)},
                {"is_page_zero"             , !bool(page_no)},
                {"no_of_last_blocks"        , no_of_last_blocks},
                {"next_page"                , (page_no + 1)},
                {"prev_page"                , (page_no > 0 ? page_no - 1 : 0)},
                {"enable_pusher"            , enable_pusher},
                {"enable_key_image_checker" , enable_key_image_checker},
                {"enable_output_key_checker", enable_output_key_checker},
                {"enable_autorefresh_option", enable_autorefresh_option}
        };

        context.emplace("txs", mstch::array()); // will keep tx to show

        // get reference to txs mstch map to be field below
        mstch::array& txs = boost::get<mstch::array>(context["txs"]);

        // calculate starting and ending block numbers to show
        int64_t start_height = height - no_of_last_blocks * (page_no + 1);

        // check if start height is not below range
        start_height = start_height < 0 ? 0 : start_height;

        int64_t end_height = start_height + no_of_last_blocks;

        // previous blk timestamp, initalised to lowest possible value
        double prev_blk_timestamp {std::numeric_limits<double>::lowest()};

        vector<double> blk_sizes;

        // measure time of cache based execution, and non-cached execution
        double duration_cached     {0.0};
        double duration_non_cached {0.0};
        uint64_t cache_hits   {0};
        uint64_t cache_misses {0};

        // loop index
        uint64_t i = start_height;

        // iterate over last no_of_last_blocks of blocks
        while (i <= end_height)
        {
            // get block at the given height i
            block blk;

            if (!mcore->get_block_by_height(i, blk))
            {
                cerr << "Cant get block: " << i << endl;
                ++i;
                continue;
            }

            // get block's hash
            crypto::hash blk_hash = core_storage->get_block_id_by_height(i);

            // get block size in kB
            double blk_size = static_cast<double>(core_storage->get_db().get_block_size(i))/1024.0;

            string blk_size_str = fmt::format("{:0.2f}", blk_size);

            blk_sizes.push_back(blk_size);

            // remove "<" and ">" from the hash string
            string blk_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", blk_hash));

            // get block age
            pair<string, string> age = get_age(server_timestamp, blk.timestamp);

            context["age_format"] = age.second;

            // get time difference [m] between previous and current blocks
            string time_delta_str {};

            if (prev_blk_timestamp > std::numeric_limits<double>::lowest())
            {
              time_delta_str = fmt::format("({:06.2f})",
                  (double(blk.timestamp) - double(prev_blk_timestamp))/60.0);
            }


            if (block_tx_cache.Contains(i))
            {
                // get txs info in the ith block from
                // our cache

                // start measure time here
                auto start = std::chrono::steady_clock::now();

                const vector<pair<crypto::hash, mstch::node>>& txd_pairs
                        = block_tx_cache.Get(i);

                // copy tx maps from txs_maps_tmp into txs array,
                // that will go to templates
                for (const pair<crypto::hash, mstch::node>& txd_pair: txd_pairs)
                {
                    // we need to check if the given transaction is still
                    // in the same block as when it was cached. it is possible
                    // the block got orphaned, and this tx is in mempool
                    // or different block, and what we have in cache
                    // is thus wrong

                    // but we do this only for first top blocks. no sense
                    // doing it for all blocks

                    bool is_tx_still_in_block_as_expected {true};

                    if (i + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE > height)
                    {
                        const crypto::hash& tx_hash = txd_pair.first;

                        if (core_storage->have_tx(tx_hash))
                        {
                            try
                            {
                                uint64_t tx_height_in_blockchain =
                                        core_storage->get_db().get_tx_block_height(tx_hash);

                                // check if height of the given tx that we have in cache,
                                // denoted by i, is same as what is acctually stored
                                // in blockchain
                                if (tx_height_in_blockchain == i)
                                {
                                    is_tx_still_in_block_as_expected = true;
                                }
                                else
                                {
                                    // if no tx in the given block, just stop
                                    // any futher search. no need. we are going
                                    // to ditch the cache, in a monent
                                    is_tx_still_in_block_as_expected = false;
                                    break;
                                }
                            }
                            catch (const TX_DNE& e)
                            {
                                cerr << "Tx from cache" << pod_to_hex(tx_hash)
                                     << " is no longer in the blockchain "
                                     << endl;

                                is_tx_still_in_block_as_expected = false;
                                break;
                            }
                        }
                        else
                        {
                            is_tx_still_in_block_as_expected = false;
                            break;
                        }

                    } // if (i + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE > height)


                    if (!is_tx_still_in_block_as_expected)
                    {
                        // if some tx in cache is not in blockchain
                        // where it should be, its probably better to
                        // ditch entire cache, as redo it below.

                        block_tx_cache.Clear();
                        txs.clear();
                        i = start_height;
                        continue; // reado the main loop
                    }

                    // if we got to here, it means that everything went fine
                    // and no unexpeced things happended.
                    const mstch::map& txd_map = boost::get<mstch::map>(txd_pair.second);

                    txs.push_back(txd_map);

                }  // for (const pair<crypto::hash, mstch::map>& txd_pair: txd_pairs)

                auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                        (std::chrono::steady_clock::now() - start);

                // cout << "block_tx_json_cache from cache" << endl;

                duration_cached += duration.count();

                ++cache_hits;
            }
            else
            {
                // this is new block. not in cashe.
                // need to process its txs and add to cache

                // start measure time here
                auto start = std::chrono::steady_clock::now();

                // get all transactions in the block found
                // initialize the first list with transaction for solving
                // the block i.e. coinbase.
                list<cryptonote::transaction> blk_txs {blk.miner_tx};
                list<crypto::hash> missed_txs;

                if (!core_storage->get_transactions(blk.tx_hashes, blk_txs, missed_txs))
                {
                    cerr << "Cant get transactions in block: " << i << endl;
                    ++i;
                    continue;
                }

                uint64_t tx_i {0};

                // this vector will go into block_tx cache
                //          tx_hash     , txd_map
                vector<pair<crypto::hash, mstch::node>> txd_pairs;

                for(list<cryptonote::transaction>::reverse_iterator rit = blk_txs.rbegin();
                    rit != blk_txs.rend(); ++rit)
                {
                    const cryptonote::transaction& tx = *rit;

                    tx_details txd = get_tx_details(tx);

                    mstch::map txd_map = txd.get_mstch_map();

                    //add age to the txd mstch map
                    txd_map.insert({"height"    , i});
                    txd_map.insert({"blk_hash"  , blk_hash_str});
                    txd_map.insert({"time_delta", time_delta_str});
                    txd_map.insert({"age"       , age.first});
                    txd_map.insert({"is_ringct" , (tx.version > 1)});
                    txd_map.insert({"rct_type"  , tx.rct_signatures.type});
                    txd_map.insert({"blk_size"  , blk_size_str});


                    // do not show block info for other than
                    // last (i.e., first after reverse below)
                    // tx in the block
                    if (tx_i < blk_txs.size() - 1)
                    {
                        txd_map["height"]     = string("");
                        txd_map["age"]        = string("");
                        txd_map["time_delta"] = string("");
                        txd_map["blk_size"]   = string("");
                    }

                    txd_pairs.emplace_back(txd.hash, txd_map);

                    ++tx_i;

                } // for(list<cryptonote::transaction>::reverse_iterator rit = blk_txs.rbegin();

                // copy tx maps from txs_maps_tmp into txs array,
                // that will go to templates
                for (const pair<crypto::hash, mstch::node>& txd_pair: txd_pairs)
                {
                    txs.push_back(boost::get<mstch::map>(txd_pair.second));
                }

                auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                        (std::chrono::steady_clock::now() - start);

                // cout << "block_tx_json_cache from cache" << endl;

                duration_non_cached += duration.count();

                ++cache_misses;

                // save in block_tx cache
                block_tx_cache.Put(i, txd_pairs);

            } // else if (block_tx_json_cache.Contains(i))


            // save current's block timestamp as reference for the next one
            prev_blk_timestamp  = static_cast<double>(blk.timestamp);

            ++i; // go to next block number

        } // while (i <= end_height)

        // calculate median size of the blocks shown
        double blk_size_median = xmreg::calc_median(blk_sizes.begin(), blk_sizes.end());

        context["blk_size_median"] = fmt::format("{:0.2f}", blk_size_median);


        // save computational times for disply in the frontend

        context["construction_time_cached"] = fmt::format(
                "{:0.4f}", duration_cached/1.0e6);

        context["construction_time_non_cached"] = fmt::format(
                "{:0.4f}", duration_non_cached/1.0e6);

        context["construction_time_total"] = fmt::format(
                "{:0.4f}", (duration_non_cached+duration_cached)/1.0e6);

        context["cache_hits"]   = cache_hits;
        context["cache_misses"] = cache_misses;


        // reverse txs and remove last (i.e., oldest)
        // tx. This is done so that time delats
        // are easier to calcualte in the above for loop
        std::reverse(txs.begin(), txs.end());

        // get memory pool rendered template
        string mempool_html = mempool(false, no_of_mempool_tx_of_frontpage);

        // append mempool_html to the index context map
        context["mempool_info"] = mempool_html;

        add_css_style(context);

        // render the page
        return mstch::render(template_file["index2"], context);

    }


    /**
     * Render mempool data
     */
    string
    mempool(bool add_header_and_footer = false, uint64_t no_of_mempool_tx = 25)
    {
        std::vector<tx_info> mempool_txs;

        if (!rpc.get_mempool(mempool_txs))
        {
          return "Getting mempool failed";
        }

        // initalise page tempate map with basic info about mempool
        mstch::map context {
                {"mempool_size",  std::to_string(mempool_txs.size())},
        };

        context.emplace("mempooltxs" , mstch::array());

        // get reference to blocks template map to be field below
        mstch::array& txs = boost::get<mstch::array>(context["mempooltxs"]);

        uint64_t mempool_size_bytes {0};

        // process only up to no_of_mempool_tx txs of mempool.
        // this is useful from the front page were we show by default
        // only 25 mempool txs. this way, we just parse 25 txs, rather
        // than potentially hundrets just to ditch most of them later.

        if (add_header_and_footer == false)
        {
            // this is to show limited number of txs in mempool
            // for example, in the front page
            no_of_mempool_tx = mempool_txs.size() > no_of_mempool_tx
                               ? no_of_mempool_tx
                               : mempool_txs.size();
        }
        else
        {
            // if we are adding footers and headers, means we
            // disply mempool on its own page, thus show all mempoool txs.
            no_of_mempool_tx = mempool_txs.size();
        }

        // for each transaction in the memory pool
        for (size_t i = 0; i < no_of_mempool_tx; ++i)
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
            pair<uint64_t, uint64_t> sum_inputs;
            pair<uint64_t, uint64_t> sum_outputs;
            uint64_t num_nonrct_inputs;

            // get mixin number in each transaction
            vector<uint64_t> mixin_numbers;

            uint64_t mixin_no {0};

            string is_ringct_str  {"N/A"};
            string rct_type_str   {"N/A"};

            string hash_str;
            string fee_str;
            string xmr_inputs_str;
            string xmr_outputs_str;
            string timestamp_str;

            string mixin_str;
            string txsize;

            try
            {
                // get the above incormation from json of that tx

                json j_tx;

                if (mempool_tx_json_cache.Contains(_tx_info.id_hash))
                {
                    // maybe its already in cashe, so we can save some time
                    // by using this, rather then making parsing json
                    // and calculating it from json

                    mempool_tx_info cached_tx_info = mempool_tx_json_cache.Get(_tx_info.id_hash);

                    //j_tx              = cached_tx_info.j_tx; // dont need it currently
                    sum_inputs        = cached_tx_info.sum_inputs;
                    sum_outputs       = cached_tx_info.sum_outputs;
                    num_nonrct_inputs = cached_tx_info.num_nonrct_inputs;
                    mixin_no          = cached_tx_info.mixin_no;
                    is_ringct_str     = cached_tx_info.is_ringct_str;
                    rct_type_str      = cached_tx_info.rct_type_str;
                    hash_str          = cached_tx_info.hash;
                    fee_str           = cached_tx_info.fee;
                    xmr_inputs_str    = cached_tx_info.xmr_inputs_str;
                    xmr_outputs_str   = cached_tx_info.xmr_outputs_str;
                    timestamp_str     = cached_tx_info.timestamp;
                    mixin_str         = cached_tx_info.mixin_str;
                    txsize            = cached_tx_info.txsize;

                    //cout << "getting json from cash for: " << _tx_info.id_hash << endl;
                }
                else
                {
                    // its not in cash. Its new tx in mempool, so
                    // construct this data and save into cash for later use

                    j_tx = json::parse(_tx_info.tx_json);

                    // sum xmr in inputs and ouputs in the given tx
                    sum_inputs        = xmreg::sum_money_in_inputs(j_tx);
                    sum_outputs       = xmreg::sum_money_in_outputs(j_tx);
                    num_nonrct_inputs = xmreg::count_nonrct_inputs(j_tx);
                    mixin_numbers     = xmreg::get_mixin_no(j_tx);

                    if (!mixin_numbers.empty())
                        mixin_no = mixin_numbers.at(0) - 1;


                    if (j_tx["version"].get<size_t>() > 1)
                    {
                        is_ringct_str = "yes";
                        rct_type_str  = string("/") + to_string(j_tx["rct_signatures"]["type"].get<uint8_t>());
                    }
                    else
                    {
                        is_ringct_str = "no";
                        rct_type_str  = "";
                    }

                    hash_str        = fmt::format("{:s}", _tx_info.id_hash);
                    fee_str         = xmreg::xmr_amount_to_str(_tx_info.fee, "{:0.3f}");
                    xmr_inputs_str  = xmreg::xmr_amount_to_str(sum_inputs.first , "{:0.3f}");
                    xmr_outputs_str = xmreg::xmr_amount_to_str(sum_outputs.first, "{:0.3f}");
                    timestamp_str   = xmreg::timestamp_to_str(_tx_info.receive_time);

                    mixin_str         = fmt::format("{:d}", mixin_no);
                    txsize            = fmt::format("{:0.2f}",
                                                    static_cast<double>(_tx_info.blob_size)/1024.0);

                    // save in mempool cache
                    mempool_tx_json_cache.Put(
                            _tx_info.id_hash,
                            mempool_tx_info {
                                j_tx,  sum_inputs, sum_outputs,
                                num_nonrct_inputs, mixin_no,
                                is_ringct_str, rct_type_str,
                                hash_str, fee_str,
                                xmr_inputs_str, xmr_outputs_str,
                                timestamp_str, mixin_str, txsize
                            });

                } // else if (mempool_tx_json_cache.Contains(_tx_info.id_hash))


            }
            catch (std::invalid_argument& e)
            {
                cerr << " j_tx = json::parse(_tx_info.tx_json): " << e.what() << endl;
            }

            // set output page template map
            txs.push_back(mstch::map {
                    {"timestamp_no"    , _tx_info.receive_time},
                    {"timestamp"       , timestamp_str},
                    {"age"             , age_str},
                    {"hash"            , hash_str},
                    {"fee"             , fee_str},
                    {"xmr_inputs"      , xmr_inputs_str},
                    {"xmr_outputs"     , xmr_outputs_str},
                    {"no_inputs"       , sum_inputs.second},
                    {"no_outputs"      , sum_outputs.second},
                    {"no_nonrct_inputs", num_nonrct_inputs},
                    {"is_ringct"       , is_ringct_str},
                    {"rct_type"        , rct_type_str},
                    {"mixin"           , mixin_str},
                    {"txsize"          , txsize}
            });

            mempool_size_bytes += _tx_info.blob_size;
        }

        context.insert({"mempool_size_kB",
                        fmt::format("{:0.2f}", static_cast<double>(mempool_size_bytes)/1024.0)});

        if (add_header_and_footer)
        {
            // this is when mempool is on its own page, /mempool
            add_css_style(context);

            context["partial_mempool_shown"] = false;

            // render the page
            return mstch::render(template_file["mempool_full"], context);
        }

        // this is for partial disply on front page.

        context["mempool_fits_on_front_page"]    = (mempool_txs.size() <= no_of_mempool_tx);
        context["no_of_mempool_tx_of_frontpage"] = no_of_mempool_tx;

        context["partial_mempool_shown"] = true;

        // render the page
        return mstch::render(template_file["mempool"], context);
    }


    string
    show_block(uint64_t _blk_height)
    {

        // get block at the given height i
        block blk;

        //cout << "_blk_height: " << _blk_height << endl;

        uint64_t current_blockchain_height
                =  core_storage->get_current_blockchain_height();

        if (_blk_height > current_blockchain_height)
        {
            cerr << "Cant get block: " << _blk_height
                 << " since its higher than current blockchain height"
                 << " i.e., " <<  current_blockchain_height
                 << endl;
            return fmt::format("Cant get block {:d} since its higher than current blockchain height!",
                               _blk_height);
        }


        if (!mcore->get_block_by_height(_blk_height, blk))
        {
            cerr << "Cant get block: " << _blk_height << endl;
            return fmt::format("Cant get block {:d}!", _blk_height);
        }

        // get block's hash
        crypto::hash blk_hash = core_storage->get_block_id_by_height(_blk_height);

        crypto::hash prev_hash = blk.prev_id;
        crypto::hash next_hash = null_hash;

        if (_blk_height + 1 <= current_blockchain_height)
        {
            next_hash = core_storage->get_block_id_by_height(_blk_height + 1);
        }

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
        uint64_t blk_size = core_storage->get_db().get_block_size(_blk_height);

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
                {"testnet"              , testnet},
                {"have_custom_lmdb"     , have_custom_lmdb},
                {"blk_hash"             , blk_hash_str},
                {"blk_height"           , _blk_height},
                {"blk_timestamp"        , blk_timestamp},
                {"blk_timestamp_epoch"  , blk.timestamp},
                {"prev_hash"            , prev_hash_str},
                {"next_hash"            , next_hash_str},
                {"have_next_hash"       , have_next_hash},
                {"have_prev_hash"       , have_prev_hash},
                {"have_txs"             , have_txs},
                {"no_txs"               , std::to_string(
                                                blk.tx_hashes.size())},
                {"blk_age"              , age.first},
                {"delta_time"           , delta_time},
                {"blk_nonce"            , blk.nonce},
                {"age_format"           , age.second},
                {"major_ver"            , std::to_string(blk.major_version)},
                {"minor_ver"            , std::to_string(blk.minor_version)},
                {"blk_size"             , fmt::format("{:0.4f}",
                                                static_cast<double>(blk_size) / 1024.0)},
        };
        context.emplace("coinbase_txs", mstch::array{{txd_coinbase.get_mstch_map()}});
        context.emplace("blk_txs"     , mstch::array());

        // .push_back(txd_coinbase.get_mstch_map()

       // boost::get<mstch::array>(context["blk_txs"]).push_back(txd_coinbase.get_mstch_map());

        // now process nomral transactions
        // get reference to blocks template map to be field below
        mstch::array& txs = boost::get<mstch::array>(context["blk_txs"]);

        // timescale representation for each tx in the block
        vector<string> mixin_timescales_str;

        // for each transaction in the block
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
        context["sum_fees"]
                = xmreg::xmr_amount_to_str(sum_fees, "{:0.6f}");

        // get xmr in the block reward
        context["blk_reward"]
                = xmreg::xmr_amount_to_str(txd_coinbase.xmr_outputs - sum_fees, "{:0.6f}");

        add_css_style(context);

        // render the page
        return mstch::render(template_file["block"], context);
    }


    string
    show_block(string _blk_hash)
    {
        crypto::hash blk_hash;

        if (!xmreg::parse_str_secret_key(_blk_hash, blk_hash))
        {
            cerr << "Cant parse blk hash: " << blk_hash << endl;
            return fmt::format("Cant get block {:s} due to block hash parse error!", blk_hash);
        }

        uint64_t blk_height;

        if (core_storage->have_block(blk_hash))
        {
            blk_height = core_storage->get_db().get_block_height(blk_hash);
        }
        else
        {
            cerr << "Cant get block: " << blk_hash << endl;
            return fmt::format("Cant get block {:s} for some uknown reason", blk_hash);
        }

        return show_block(blk_height);
    }

    string
    show_tx(string tx_hash_str, uint16_t with_ring_signatures = 0)
    {

        // parse tx hash string to hash object
        crypto::hash tx_hash;

        if (!xmreg::parse_str_secret_key(tx_hash_str, tx_hash))
        {
            cerr << "Cant parse tx hash: " << tx_hash_str << endl;
            return string("Cant get tx hash due to parse error: " + tx_hash_str);
        }

        // tx age
        pair<string, string> age;

        string blk_timestamp {"N/A"};

        // get transaction
        transaction tx;

        bool show_more_details_link {true};

        if (!mcore->get_tx(tx_hash, tx))
        {
            cerr << "Cant get tx in blockchain: " << tx_hash
                 << ". \n Check mempool now" << endl;

            vector<pair<tx_info, transaction>> found_txs
                    = search_mempool(tx_hash);

            if (!found_txs.empty())
            {
                // there should be only one tx found
                tx = found_txs.at(0).second;

                // since its tx in mempool, it has no blk yet
                // so use its recive_time as timestamp to show

                uint64_t tx_recieve_timestamp
                        = found_txs.at(0).first.receive_time;

                blk_timestamp = xmreg::timestamp_to_str(tx_recieve_timestamp);

                age = get_age(server_timestamp, tx_recieve_timestamp,
                              FULL_AGE_FORMAT);

                // for mempool tx, we dont show more details, e.g., json tx representation
                // so no need for the link
                show_more_details_link = false;
            }
            else
            {
                // tx is nowhere to be found :-(
                return string("Cant get tx: " + tx_hash_str);
            }
        }

        mstch::map tx_context;

        if (tx_context_cache.Contains(tx_hash))
        {

            // we are going to measure time for the construction of the
            // tx context from cashe. just for fun, to see if cache is any faster.
            auto start = std::chrono::steady_clock::now();

            const tx_info_cache& tx_info_cashed = tx_context_cache.Get(tx_hash);

            tx_context = tx_info_cashed.tx_map;

            //cout << "get tx from cash: " << tx_hash_str <<endl;
            //cout << " - tx_blk_height: " << boost::get<uint64_t>(tx_context["tx_blk_height"]) <<endl;
            //cout << " - blk_timestamp_uint: " << boost::get<uint64_t>(tx_context["blk_timestamp_uint"]) <<endl;

            // now have to update age and confirmation numbers of the tx.

            uint64_t tx_blk_height      = boost::get<uint64_t>(tx_context["tx_blk_height"]);
            uint64_t blk_timestamp_uint = boost::get<uint64_t>(tx_context["blk_timestamp_uint"]);

            if (tx_blk_height > 0)
            {
                // seems to be in blockchain. off course it could have been orphaned
                // so double check if its for sure in blockchain

                if (core_storage->have_tx(tx_hash))
                {
                    // ok, it is still in blockchain
                    // update its age and number of confirmations

                    pair<string, string> age = get_age(std::time(nullptr), blk_timestamp_uint, FULL_AGE_FORMAT);

                    tx_context["delta_time"] = age.first;

                    uint64_t bc_height = core_storage->get_current_blockchain_height();

                    tx_context["confirmations"] = bc_height - (tx_blk_height - 1);

                    // marke it as from cashe. useful if we want to show
                    // info about cashed/not cashed in frontend.
                    tx_context["from_cache"] = true;

                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                            (std::chrono::steady_clock::now() - start);

                    tx_context["construction_time"] = fmt::format(
                            "{:0.4f}", static_cast<double>(duration.count())/1.0e6);

                    // normally we should update this into in the cache.
                    // but since we make this check all the time,
                    // we can skip updating cashed version

                } // if (core_storage->have_tx(tx_hash))
                else
                {
                    // its not in blockchain, but it was there when we cashed it.
                    // so we update it in cash, as it should be back in mempool

                    tx_context = construct_tx_context(tx, with_ring_signatures);

                    tx_context_cache.Put(tx_hash, tx_info_cache {
                            boost::get<uint64_t>(tx_context["tx_blk_height"]),
                            boost::get<uint64_t>(tx_context["blk_timestamp_uint"]),
                            tx_context
                    });

                }
            } //  if (tx_blk_height > 0)
            else
            {
                // the tx was cashed when in mempool.
                // since then, it might have been included in some block.
                // so we check it.

                if (core_storage->have_tx(tx_hash))
                {
                    // checking if in blockchain already
                    // it was before in mempool, but now maybe already in blockchain

                    tx_context = construct_tx_context(tx, with_ring_signatures);

                    tx_context_cache.Put(tx_hash, tx_info_cache {
                            boost::get<uint64_t>(tx_context["tx_blk_height"]),
                            boost::get<uint64_t>(tx_context["blk_timestamp_uint"]),
                            tx_context
                    });

                } // if (core_storage->have_tx(tx_hash))
                else
                {
                    // still seems to be in mempool only.
                    // so just get its time duration, as its read only
                    // from cache

                    tx_context["from_cache"] = true;

                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                            (std::chrono::steady_clock::now() - start);

                    tx_context["construction_time"] = fmt::format(
                            "{:0.4f}", static_cast<double>(duration.count())/1.0e6);

                }

            }  // else if (tx_blk_height > 0)

        } // if (tx_context_cache.Contains(tx_hash))
        else
        {

            // we are going to measure time for the construction of the
            // tx context. just for fun, to see if cache is any faster.
            auto start = std::chrono::steady_clock::now();

            tx_context = construct_tx_context(tx, with_ring_signatures);

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                    (std::chrono::steady_clock::now() - start);

            tx_context_cache.Put(tx_hash, tx_info_cache {
                    boost::get<uint64_t>(tx_context["tx_blk_height"]),
                    boost::get<uint64_t>(tx_context["blk_timestamp_uint"]),
                    tx_context
            });

            tx_context["construction_time"] = fmt::format(
                    "{:0.4f}", static_cast<double>(duration.count())/1.0e6);

        } // else if (tx_context_cache.Contains(tx_hash))
        

        tx_context["show_more_details_link"] = show_more_details_link;

        if (boost::get<bool>(tx_context["has_error"]))
        {
            return boost::get<string>(tx_context["error_msg"]);
        }

        mstch::map context {
                {"testnet"          , this->testnet},
                {"have_custom_lmdb" , have_custom_lmdb},
                {"txs"              , mstch::array{}}
        };

        boost::get<mstch::array>(context["txs"]).push_back(tx_context);

        map<string, string> partials {
                {"tx_details", template_file["tx_details"]},
        };

        add_css_style(context);

        // render the page
        return mstch::render(template_file["tx"], context, partials);
    }

    string
    show_my_outputs(string tx_hash_str,
                    string xmr_address_str,
                    string viewkey_str, /* or tx_prv_key_str when tx_prove == true */
                    string raw_tx_data,
                    bool tx_prove = false)
    {

        // remove white characters
        boost::trim(tx_hash_str);
        boost::trim(xmr_address_str);
        boost::trim(viewkey_str);
        boost::trim(raw_tx_data);

        if (tx_hash_str.empty())
        {
            return string("tx hash not provided!");
        }

        if (xmr_address_str.empty())
        {
            return string("Monero address not provided!");
        }

        if (viewkey_str.empty())
        {
            if (!tx_prove)
                return string("Viewkey not provided!");
            else
                return string("Tx private key not provided!");
        }

        // parse tx hash string to hash object
        crypto::hash tx_hash;

        if (!xmreg::parse_str_secret_key(tx_hash_str, tx_hash))
        {
            cerr << "Cant parse tx hash: " << tx_hash_str << endl;
            return string("Cant get tx hash due to parse error: " + tx_hash_str);
        }

        // parse string representing given monero address
        cryptonote::account_public_address address;

        if (!xmreg::parse_str_address(xmr_address_str,  address, testnet))
        {
            cerr << "Cant parse string address: " << xmr_address_str << endl;
            return string("Cant parse xmr address: " + xmr_address_str);
        }

        // parse string representing given private key
        crypto::secret_key prv_view_key;

        if (!xmreg::parse_str_secret_key(viewkey_str, prv_view_key))
        {
            cerr << "Cant parse the private key: " << viewkey_str << endl;
            return string("Cant parse private key: " + viewkey_str);
        }


        // just to see how would having spend keys could worked
        // this is from testnet wallet: A2VTvE8bC9APsWFn3mQzgW8Xfcy2SP2CRUArD6ZtthNaWDuuvyhtBcZ8WDuYMRt1HhcnNQvpXVUavEiZ9waTbyBhP6RM8TV
        // view key: 041a241325326f9d86519b714a9b7f78b29111551757eeb6334d39c21f8b7400
        // example tx: 430b070e213659a864ec82d674fddb5ccf7073cae231b019ba1ebb4bfdc07a15
//        string spend_key_str("643fedcb8dca1f3b406b84575ecfa94ba01257d56f20d55e8535385503dacc08");
//
//        crypto::secret_key prv_spend_key;
//        if (!xmreg::parse_str_secret_key(spend_key_str, prv_spend_key))
//        {
//            cerr << "Cant parse the prv_spend_key : " << spend_key_str << endl;
//            return string("Cant parse prv_spend_key : " + spend_key_str);
//        }

        // tx age
        pair<string, string> age;

        string blk_timestamp {"N/A"};

        // get transaction
        transaction tx;

        if (!raw_tx_data.empty())
        {
            // we want to check outputs of tx submited through tx pusher.
            // it is raw tx data, it is not in blockchain nor in mempool.
            // so we need to reconstruct tx object from this string

            cryptonote::blobdata tx_data_blob;

            if (!epee::string_tools::parse_hexstr_to_binbuff(raw_tx_data, tx_data_blob))
            {
                string msg = fmt::format("Cant obtain tx_data_blob from raw_tx_data");

                cerr << msg << endl;

                return msg;
            }

            crypto::hash tx_hash_from_blob;
            crypto::hash tx_prefix_hash_from_blob;

            if (!cryptonote::parse_and_validate_tx_from_blob(tx_data_blob,
                                                             tx,
                                                             tx_hash_from_blob,
                                                             tx_prefix_hash_from_blob))
            {
                string msg = fmt::format("cant parse_and_validate_tx_from_blob");

                cerr << msg << endl;

                return msg;
            }

        }
        else if (!mcore->get_tx(tx_hash, tx))
        {
            cerr << "Cant get tx in blockchain: " << tx_hash
                 << ". \n Check mempool now" << endl;

            vector<pair<tx_info, transaction>> found_txs
                    = search_mempool(tx_hash);

            if (!found_txs.empty())
            {
                // there should be only one tx found
                tx = found_txs.at(0).second;

                // since its tx in mempool, it has no blk yet
                // so use its recive_time as timestamp to show

                uint64_t tx_recieve_timestamp
                        = found_txs.at(0).first.receive_time;

                blk_timestamp = xmreg::timestamp_to_str(tx_recieve_timestamp);

                age = get_age(server_timestamp,
                              tx_recieve_timestamp,
                              FULL_AGE_FORMAT);
            }
            else
            {
                // tx is nowhere to be found :-(
                return string("Cant get tx: " + tx_hash_str);
            }
        }

        tx_details txd = get_tx_details(tx);

        uint64_t tx_blk_height {0};

        bool tx_blk_found {false};

        try
        {
            tx_blk_height = core_storage->get_db().get_tx_block_height(tx_hash);
            tx_blk_found = true;
        }
        catch (exception& e)
        {
            cerr << "Cant get block height: " << tx_hash
                 << e.what() << endl;
        }


        // get block cointaining this tx
        block blk;

        if (tx_blk_found && !mcore->get_block_by_height(tx_blk_height, blk))
        {
            cerr << "Cant get block: " << tx_blk_height << endl;
        }

        string tx_blk_height_str {"N/A"};

        if (tx_blk_found)
        {
            // calculate difference between tx and server timestamps
            age = get_age(server_timestamp, blk.timestamp, FULL_AGE_FORMAT);

            blk_timestamp = xmreg::timestamp_to_str(blk.timestamp);

            tx_blk_height_str = std::to_string(tx_blk_height);
        }

        // payments id. both normal and encrypted (payment_id8)
        string pid_str   = REMOVE_HASH_BRAKETS(fmt::format("{:s}", txd.payment_id));
        string pid8_str  = REMOVE_HASH_BRAKETS(fmt::format("{:s}", txd.payment_id8));


        // initalise page tempate map with basic info about blockchain
        mstch::map context {
                {"testnet"              , testnet},
                {"have_custom_lmdb"     , have_custom_lmdb},
                {"tx_hash"              , tx_hash_str},
                {"tx_prefix_hash"       , pod_to_hex(txd.prefix_hash)},
                {"xmr_address"          , xmr_address_str},
                {"viewkey"              , viewkey_str},
                {"tx_pub_key"           , REMOVE_HASH_BRAKETS(fmt::format("{:s}", txd.pk))},
                {"blk_height"           , tx_blk_height_str},
                {"tx_size"              , fmt::format("{:0.4f}",
                                                      static_cast<double>(txd.size) / 1024.0)},
                {"tx_fee"               , xmreg::xmr_amount_to_str(txd.fee)},
                {"blk_timestamp"        , blk_timestamp},
                {"delta_time"           , age.first},
                {"outputs_no"           , static_cast<uint64_t>(txd.output_pub_keys.size())},
                {"has_payment_id"       , txd.payment_id  != null_hash},
                {"has_payment_id8"      , txd.payment_id8 != null_hash8},
                {"payment_id"           , pid_str},
                {"payment_id8"          , pid8_str},
                {"tx_prove"             , tx_prove}
        };

        string server_time_str = xmreg::timestamp_to_str(server_timestamp, "%F");



        // public transaction key is combined with our viewkey
        // to create, so called, derived key.
        key_derivation derivation;

        public_key pub_key = tx_prove ? address.m_view_public_key : txd.pk;

        //cout << "txd.pk: " << pod_to_hex(txd.pk) << endl;

        if (!generate_key_derivation(pub_key, prv_view_key, derivation))
        {
            cerr << "Cant get derived key for: "  << "\n"
                 << "pub_tx_key: " << pub_key << " and "
                 << "prv_view_key" << prv_view_key << endl;

            return string("Cant get key_derivation");
        }

        mstch::array outputs;

        uint64_t sum_xmr {0};

        std::vector<uint64_t> money_transfered(tx.vout.size(), 0);

        //std::deque<rct::key> mask(tx.vout.size());

        uint64_t output_idx {0};

        for (pair<txout_to_key, uint64_t>& outp: txd.output_pub_keys)
        {

            // get the tx output public key
            // that normally would be generated for us,
            // if someone had sent us some xmr.
            public_key tx_pubkey;

            derive_public_key(derivation,
                              output_idx,
                              address.m_spend_public_key,
                              tx_pubkey);

            //cout << pod_to_hex(outp.first.key) << endl;
            //cout << pod_to_hex(tx_pubkey) << endl;

            // check if generated public key matches the current output's key
            bool mine_output = (outp.first.key == tx_pubkey);

            // if mine output has RingCT, i.e., tx version is 2
            if (mine_output && tx.version == 2)
            {
                // cointbase txs have amounts in plain sight.
                // so use amount from ringct, only for non-coinbase txs
                if (!is_coinbase(tx))
                {

                    // initialize with regular amount
                    uint64_t rct_amount = money_transfered[output_idx];

                    bool r;

                    r = decode_ringct(tx.rct_signatures,
                                      pub_key,
                                      prv_view_key,
                                      output_idx,
                                      tx.rct_signatures.ecdhInfo[output_idx].mask,
                                      rct_amount);

                    if (!r)
                    {
                        cerr << "\nshow_my_outputs: Cant decode ringCT! " << endl;
                    }

                    outp.second         = rct_amount;
                    money_transfered[output_idx] = rct_amount;
                }

            }

            if (mine_output)
            {
                sum_xmr += outp.second;
            }

            outputs.push_back(mstch::map {
                {"out_pub_key"   , REMOVE_HASH_BRAKETS(
                                           fmt::format("{:s}",
                                                       outp.first.key))},
                {"amount"        , xmreg::xmr_amount_to_str(outp.second)},
                {"mine_output"   , mine_output},
                {"output_idx"    , fmt::format("{:02d}", output_idx)}
            });

            ++output_idx;
        }

        // we can also test ouputs used in mixins for key images
        // this can show possible spending. Only possible, because
        // without a spend key, we cant know for sure. It might be
        // that our output was used by someone else for their mixins.

        bool show_key_images {false};


        mstch::array inputs;

        vector<txin_to_key> input_key_imgs = xmreg::get_key_images(tx);

        // to hold sum of xmr in matched mixins, those that
        // perfectly match mixin public key with outputs in mixn_tx.
        uint64_t sum_mixin_xmr {0};

        // this is used for the final check. we assument that number of
        // parefct matches must be equal to number of inputs in a tx.
        uint64_t no_of_matched_mixins {0};

        for (const txin_to_key& in_key: input_key_imgs)
        {

            // get absolute offsets of mixins
            std::vector<uint64_t> absolute_offsets
                    = cryptonote::relative_output_offsets_to_absolute(
                            in_key.key_offsets);

            // get public keys of outputs used in the mixins that match to the offests
            std::vector<cryptonote::output_data_t> mixin_outputs;


            try
            {
                core_storage->get_db().get_output_key(in_key.amount,
                                                      absolute_offsets,
                                                      mixin_outputs);
            }
            catch (const OUTPUT_DNE& e)
            {
                cerr << "get_output_keys: " << e.what() << endl;
                continue;
            }

            inputs.push_back(mstch::map{
                    {"key_image"       , pod_to_hex(in_key.k_image)},
                    {"key_image_amount", xmreg::xmr_amount_to_str(in_key.amount)},
                    make_pair(string("mixins"), mstch::array{})
            });

            mstch::array& mixins = boost::get<mstch::array>(
                 boost::get<mstch::map>(inputs.back())["mixins"]
            );

            // to store our mixins found for the given key image
            vector<map<string, string>> our_mixins_found;

            // mixin counter
            size_t count = 0;

            // there can be more than one our output used for mixin in a single
            // input. For example, if two outputs are matched (marked by *) in html,
            // one of them will be our real spending, and second will be used as a fake
            // one. ideally, to determine which is which, spendkey is required.
            // obvisouly we dont have it here, so we need to pick one in other way.
            // for now I will just pick the first one we find, and threat it as the
            // real spending output. The no_of_output_matches_found variable
            // is used for this purporse.
            // testnet tx 430b070e213659a864ec82d674fddb5ccf7073cae231b019ba1ebb4bfdc07a15
            // and testnet wallet details provided earier for spend key,
            // demonstrate this. this txs has one input that uses two of our ouputs.
            // without spent key, its imposible to know which one is real spendking
            // and which one is fake.
            size_t no_of_output_matches_found {0};

            // for each found output public key check if its ours or not
            for (const uint64_t& abs_offset: absolute_offsets)
            {

                // get basic information about mixn's output
                cryptonote::output_data_t output_data = mixin_outputs.at(count);

                tx_out_index tx_out_idx;

                try
                {
                    // get pair pair<crypto::hash, uint64_t> where first is tx hash
                    // and second is local index of the output i in that tx
                    tx_out_idx = core_storage->get_db()
                            .get_output_tx_and_index(in_key.amount, abs_offset);
                }
                catch (const OUTPUT_DNE& e)
                {

                    string out_msg = fmt::format(
                            "Output with amount {:d} and index {:d} does not exist!",
                            in_key.amount, abs_offset
                    );

                    cerr << out_msg << endl;

                    break;
                }

                string out_pub_key_str = pod_to_hex(output_data.pubkey);

                //cout << "out_pub_key_str: " << out_pub_key_str << endl;


                // get mixin transaction
                transaction mixin_tx;

                if (!mcore->get_tx(tx_out_idx.first, mixin_tx))
                {
                    cerr << "Cant get tx: " << tx_out_idx.first << endl;

                    break;
                }

                string mixin_tx_hash_str = pod_to_hex(tx_out_idx.first);

                mixins.push_back(mstch::map{
                        {"mixin_pub_key"      , out_pub_key_str},
                        make_pair<string, mstch::array>("mixin_outputs"      , mstch::array{}),
                        {"has_mixin_outputs"  , false}
                });

                mstch::array& mixin_outputs = boost::get<mstch::array>(
                        boost::get<mstch::map>(mixins.back())["mixin_outputs"]
                );

                mstch::node& has_mixin_outputs
                        = boost::get<mstch::map>(mixins.back())["has_mixin_outputs"];


                bool found_something {false};


                public_key mixin_tx_pub_key
                        = xmreg::get_tx_pub_key_from_received_outs(mixin_tx);

                string mixin_tx_pub_key_str = pod_to_hex(mixin_tx_pub_key);

                // public transaction key is combined with our viewkey
                // to create, so called, derived key.
                key_derivation derivation;

                if (!generate_key_derivation(mixin_tx_pub_key, prv_view_key, derivation))
                {
                    cerr << "Cant get derived key for: "  << "\n"
                         << "pub_tx_key: " << mixin_tx_pub_key << " and "
                         << "prv_view_key" << prv_view_key << endl;

                    continue;
                }

                //          <public_key  , amount  , out idx>
                vector<tuple<txout_to_key, uint64_t, uint64_t>> output_pub_keys;

                output_pub_keys = xmreg::get_ouputs_tuple(mixin_tx);

                mixin_outputs.push_back(mstch::map{
                        {"mix_tx_hash"      , mixin_tx_hash_str},
                        {"mix_tx_pub_key"   , mixin_tx_pub_key_str},
                        make_pair<string, mstch::array>("found_outputs"    , mstch::array{}),
                        {"has_found_outputs", false}
                });

                mstch::array& found_outputs = boost::get<mstch::array>(
                        boost::get<mstch::map>(mixin_outputs.back())["found_outputs"]
                );

                mstch::node& has_found_outputs
                        = boost::get<mstch::map>(mixin_outputs.back())["has_found_outputs"];

                // for each output in mixin tx, find the one from key_image
                // and check if its ours.
                for (const auto& mix_out: output_pub_keys)
                {

                    txout_to_key txout_k      = std::get<0>(mix_out);
                    uint64_t amount           = std::get<1>(mix_out);
                    uint64_t output_idx_in_tx = std::get<2>(mix_out);

                    //cout << " - " << pod_to_hex(txout_k.key) << endl;

//                        // analyze only those output keys
//                        // that were used in mixins
//                        if (txout_k.key != output_data.pubkey)
//                        {
//                            continue;
//                        }

                    // get the tx output public key
                    // that normally would be generated for us,
                    // if someone had sent us some xmr.
                    public_key tx_pubkey_generated;

                    derive_public_key(derivation,
                                      output_idx_in_tx,
                                      address.m_spend_public_key,
                                      tx_pubkey_generated);

                    // check if generated public key matches the current output's key
                    bool mine_output = (txout_k.key == tx_pubkey_generated);


                    if (mine_output && mixin_tx.version == 2)
                    {
                        // cointbase txs have amounts in plain sight.
                        // so use amount from ringct, only for non-coinbase txs
                        if (!is_coinbase(mixin_tx))
                        {
                            // initialize with regular amount
                            uint64_t rct_amount = amount;

                            bool r;

                            r = decode_ringct(mixin_tx.rct_signatures,
                                              mixin_tx_pub_key,
                                              prv_view_key,
                                              output_idx_in_tx,
                                              mixin_tx.rct_signatures.ecdhInfo[output_idx_in_tx].mask,
                                              rct_amount);

                            if (!r)
                            {
                                cerr << "show_my_outputs: key images: Cant decode ringCT!" << endl;
                            }

                            amount = rct_amount;

                        } // if (mine_output && mixin_tx.version == 2)
                    }

                    // makre only
                    bool output_match = (txout_k.key == output_data.pubkey);

                    // mark only first output_match as the "real" one
                    // due to luck of better method of gussing which output
                    // is real if two are found in a single input.
                    output_match = output_match && no_of_output_matches_found == 0;

                    // save our mixnin's public keys
                    found_outputs.push_back(mstch::map {
                            {"my_public_key"   , pod_to_hex(txout_k.key)},
                            {"tx_hash"         , tx_hash_str},
                            {"mine_output"     , mine_output},
                            {"out_idx"         , to_string(output_idx_in_tx)},
                            {"formed_output_pk", out_pub_key_str},
                            {"out_in_match"    , output_match},
                            {"amount"          , xmreg::xmr_amount_to_str(amount)}
                    });

                    //cout << "txout_k.key == output_data.pubkey" << endl;
                    //cout << pod_to_hex(txout_k.key) << " == " << pod_to_hex(output_data.pubkey) << endl;

                    if (mine_output)
                    {

                        found_something = true;
                        show_key_images = true;

                        // increase sum_mixin_xmr only when
                        // public key of an outputs used in ring signature,
                        // matches a public key in a mixin_tx
                        if (txout_k.key != output_data.pubkey)
                        {
                            continue;
                        }

                        // sum up only first output matched found in each input
                        if (no_of_output_matches_found == 0)
                        {
                            // for regular txs, just concentrated on outputs
                            // which have same amount as the key image.
                            // for ringct its not possible to know for sure amount
                            // in key image without spend key, so we just use all
                            // for regular/old txs there must be also a match
                            // in amounts, not only in output public keys
                            if (mixin_tx.version < 2 && amount == in_key.amount)
                            {
                                sum_mixin_xmr += amount;
                            }
                            else if (mixin_tx.version == 2) // ringct
                            {
                                sum_mixin_xmr += amount;
                            }

                            no_of_matched_mixins++;
                        }


                          // generate key_image using this output
                          // just to see how would having spend keys worked
//                        crypto::key_image key_img;
//
//                        if (!xmreg::generate_key_image(derivation,
//                                                       output_idx_in_tx, /* position in the tx */
//                                                       prv_spend_key,
//                                                       address.m_spend_public_key,
//                                                       key_img)) {
//                            cerr << "Cant generate key image for output: "
//                                 << pod_to_hex(output_data.pubkey) << endl;
//                            break;
//                        }
//
//                        cout    << "output_data.pubkey: " << pod_to_hex(output_data.pubkey)
//                                << ", key_img: " << pod_to_hex(key_img)
//                                << ", key_img == input_key: " << (key_img == in_key.k_image)
//                                << endl;



                        no_of_output_matches_found++;

                    }

                } // for (const pair<txout_to_key, uint64_t>& mix_out: txd.output_pub_keys)

                has_found_outputs = !found_outputs.empty();

                has_mixin_outputs = found_something;

                ++count;

            } // for (const cryptonote::output_data_t& output_data: mixin_outputs)



        } //  for (const txin_to_key& in_key: input_key_imgs)


        context.emplace("outputs", outputs);

        context["found_our_outputs"] = (sum_xmr > 0);
        context["sum_xmr"]           = xmreg::xmr_amount_to_str(sum_xmr);

        context.emplace("inputs", inputs);

        context["show_inputs"]   = show_key_images;
        context["inputs_no"]     = static_cast<uint64_t>(inputs.size());
        context["sum_mixin_xmr"] = xmreg::xmr_amount_to_str(
                sum_mixin_xmr, "{:0.12f}", false);


        uint64_t possible_spending  {0};

        // show spending only if sum of mixins is more than
        // what we get + fee, and number of perferctly matched
        // mixis is equal to number of inputs
        if (sum_mixin_xmr > (sum_xmr + txd.fee)
                && no_of_matched_mixins == inputs.size())
        {
            //                  (outcoming    - incoming) - fee
            possible_spending = (sum_mixin_xmr - sum_xmr) - txd.fee;
        }

        context["possible_spending"] = xmreg::xmr_amount_to_str(
                possible_spending, "{:0.12f}", false);

        add_css_style(context);

        // render the page
        return mstch::render(template_file["my_outputs"], context);
    }

    string
    show_prove(string tx_hash_str,
                    string xmr_address_str,
                    string tx_prv_key_str)
    {
        string raw_tx_data {""}; // not using it in prove tx. only for outputs

        return show_my_outputs(tx_hash_str, xmr_address_str,
                               tx_prv_key_str, raw_tx_data, true);
    }

    string
    show_rawtx()
    {

        // initalise page tempate map with basic info about blockchain
        mstch::map context {
                {"testnet"              , testnet},
                {"have_custom_lmdb"     , have_custom_lmdb}
        };

        add_css_style(context);

        // render the page
        return mstch::render(template_file["rawtx"], context);
    }

    string
    show_checkrawtx(string raw_tx_data, string action)
    {
        clean_post_data(raw_tx_data);

        string decoded_raw_tx_data = epee::string_encoding::base64_decode(raw_tx_data);

        //cout << decoded_raw_tx_data << endl;

        const size_t magiclen = strlen(UNSIGNED_TX_PREFIX);

        string data_prefix = xmreg::make_printable(decoded_raw_tx_data.substr(0, magiclen));

        bool unsigned_tx_given {false};

        if (strncmp(decoded_raw_tx_data.c_str(), UNSIGNED_TX_PREFIX, magiclen) == 0)
        {
            unsigned_tx_given = true;
        }

        // initalize page template context map
        mstch::map context {
                {"testnet"              , testnet},
                {"have_custom_lmdb"     , have_custom_lmdb},
                {"unsigned_tx_given"    , unsigned_tx_given},
                {"have_raw_tx"          , true},
                {"data_prefix"          , data_prefix},
        };
        context.emplace("txs", mstch::array{});

        if (unsigned_tx_given)
        {

            bool r {false};

            string s = decoded_raw_tx_data.substr(magiclen);

            ::tools::wallet2::unsigned_tx_set exported_txs;

            try
            {
                std::istringstream iss(s);
                boost::archive::portable_binary_iarchive ar(iss);
                ar >> exported_txs;

                r = true;
            }
            catch (...)
            {
                cerr << "Failed to parse unsigned tx data " << endl;
            }

            if (r)
            {
                mstch::array& txs = boost::get<mstch::array>(context["txs"]);

                for (const ::tools::wallet2::tx_construction_data& tx_cd: exported_txs.txes)
                {
                    size_t no_of_sources = tx_cd.sources.size();

                    const tx_destination_entry& tx_change   = tx_cd.change_dts;

                    crypto::hash payment_id   = null_hash;
                    crypto::hash8 payment_id8 = null_hash8;

                    get_payment_id(tx_cd.extra, payment_id, payment_id8);

                    // payments id. both normal and encrypted (payment_id8)
                    string pid_str   = REMOVE_HASH_BRAKETS(fmt::format("{:s}", payment_id));
                    string pid8_str  = REMOVE_HASH_BRAKETS(fmt::format("{:s}", payment_id8));


                    mstch::map tx_cd_data {
                            {"no_of_sources"      , static_cast<uint64_t>(no_of_sources)},
                            {"use_rct"            , tx_cd.use_rct},
                            {"change_amount"      , xmreg::xmr_amount_to_str(tx_change.amount)},
                            {"has_payment_id"     , (payment_id  != null_hash)},
                            {"has_payment_id8"    , (payment_id8 != null_hash8)},
                            {"payment_id"         , pid_str},
                            {"payment_id8"        , pid8_str},
                    };
                    tx_cd_data.emplace("dest_sources" , mstch::array{});
                    tx_cd_data.emplace("dest_infos"   , mstch::array{});

                    mstch::array& dest_sources = boost::get<mstch::array>(tx_cd_data["dest_sources"]);
                    mstch::array& dest_infos = boost::get<mstch::array>(tx_cd_data["dest_infos"]);

                    for (const tx_destination_entry& a_dest: tx_cd.splitted_dsts)
                    {
                        mstch::map dest_info {
                                {"dest_address"  , get_account_address_as_str(testnet, a_dest.addr)},
                                {"dest_amount"   , xmreg::xmr_amount_to_str(a_dest.amount)}
                        };

                        dest_infos.push_back(dest_info);
                    }

                    vector<vector<uint64_t>> mixin_timestamp_groups;
                    vector<uint64_t> real_output_indices;

                    uint64_t sum_outputs_amounts {0};

                    for (size_t i = 0; i < no_of_sources; ++i)
                    {

                        const tx_source_entry&  tx_source = tx_cd.sources.at(i);

                        mstch::map single_dest_source {
                            {"output_amount"              , xmreg::xmr_amount_to_str(tx_source.amount)},
                            {"real_output"                , static_cast<uint64_t>(tx_source.real_output)},
                            {"real_out_tx_key"            , pod_to_hex(tx_source.real_out_tx_key)},
                            {"real_output_in_tx_index"    , static_cast<uint64_t>(tx_source.real_output_in_tx_index)},
                        };
                        single_dest_source.emplace("outputs", mstch::array{});

                        sum_outputs_amounts += tx_source.amount;

                        //cout << "tx_source.real_output: "             << tx_source.real_output << endl;
                        //cout << "tx_source.real_out_tx_key: "         << tx_source.real_out_tx_key << endl;
                        //cout << "tx_source.real_output_in_tx_index: " << tx_source.real_output_in_tx_index << endl;

                        uint64_t index_of_real_output = tx_source.outputs[tx_source.real_output].first;

                        tx_out_index real_toi;

                        uint64_t tx_source_amount = (tx_source.rct ? 0 : tx_source.amount);

                        try
                        {
                            // get tx of the real output
                            real_toi = core_storage->get_db()
                                    .get_output_tx_and_index(tx_source_amount,
                                                             index_of_real_output);
                        }
                        catch (const OUTPUT_DNE& e)
                        {

                            string out_msg = fmt::format(
                                    "Output with amount {:d} and index {:d} does not exist!",
                                    tx_source_amount, index_of_real_output
                            );

                            cerr << out_msg << endl;

                            return string(out_msg);
                        }



                        transaction real_source_tx;

                        if (!mcore->get_tx(real_toi.first, real_source_tx))
                        {
                            cerr << "Cant get tx in blockchain: " << real_toi.first << endl;
                            return string("Cant get tx: " + pod_to_hex(real_toi.first));
                        }

                        tx_details real_txd = get_tx_details(real_source_tx);

                        real_output_indices.push_back(tx_source.real_output);

                        public_key real_out_pub_key = real_txd.output_pub_keys[tx_source.real_output_in_tx_index].first.key;

                        //cout << "real_txd.hash: "    << pod_to_hex(real_txd.hash) << endl;
                        //cout << "real_txd.pk: "      << pod_to_hex(real_txd.pk) << endl;
                        //cout << "real_out_pub_key: " << pod_to_hex(real_out_pub_key) << endl;

                        mstch::array& outputs = boost::get<mstch::array>(single_dest_source["outputs"]);

                        vector<uint64_t> mixin_timestamps;

                        size_t output_i {0};

                        for(const tx_source_entry::output_entry& oe: tx_source.outputs)
                        {

                            tx_out_index toi;

                            try
                            {

                                // get tx of the real output
                                toi = core_storage->get_db()
                                        .get_output_tx_and_index(tx_source_amount, oe.first);
                            }
                            catch (OUTPUT_DNE& e)
                            {

                                string out_msg = fmt::format(
                                        "Output with amount {:d} and index {:d} does not exist!",
                                        tx_source_amount, oe.first
                                );

                                cerr << out_msg << endl;

                                return string(out_msg);
                            }

                            transaction tx;

                            if (!mcore->get_tx(toi.first, tx))
                            {
                                cerr << "Cant get tx in blockchain: " << toi.first
                                     << ". \n Check mempool now" << endl;
                                    // tx is nowhere to be found :-(
                                    return string("Cant get tx: " + pod_to_hex(toi.first));
                            }

                            tx_details txd = get_tx_details(tx);

                            public_key out_pub_key = txd.output_pub_keys[toi.second].first.key;


                            // get block cointaining this tx
                            block blk;

                            if (!mcore->get_block_by_height(txd.blk_height, blk))
                            {
                                cerr << "Cant get block: " << txd.blk_height << endl;
                                return string("Cant get block: "  + to_string(txd.blk_height));
                            }

                            pair<string, string> age = get_age(server_timestamp, blk.timestamp);

                            mstch::map single_output {
                                    {"out_index"          , oe.first},
                                    {"tx_hash"            , pod_to_hex(txd.hash)},
                                    {"out_pub_key"        , pod_to_hex(out_pub_key)},
                                    {"ctkey"              , pod_to_hex(oe.second)},
                                    {"output_age"         , age.first},
                                    {"is_real"            , (out_pub_key == real_out_pub_key)}
                            };

                            single_dest_source.insert({"age_format"          , age.second});

                            outputs.push_back(single_output);

                            mixin_timestamps.push_back(blk.timestamp);

                            ++output_i;

                        } // for(const tx_source_entry::output_entry& oe: tx_source.outputs)

                        dest_sources.push_back(single_dest_source);

                        mixin_timestamp_groups.push_back(mixin_timestamps);

                    } //  for (size_t i = 0; i < no_of_sources; ++i)

                    tx_cd_data.insert({"sum_outputs_amounts" ,
                                       xmreg::xmr_amount_to_str(sum_outputs_amounts)});


                    uint64_t min_mix_timestamp;
                    uint64_t max_mix_timestamp;

                    pair<mstch::array, double> mixins_timescales
                            = construct_mstch_mixin_timescales(
                              mixin_timestamp_groups,
                              min_mix_timestamp,
                              max_mix_timestamp
                            );

                    tx_cd_data.emplace("timescales", mixins_timescales.first);
                    tx_cd_data["min_mix_time"]     = xmreg::timestamp_to_str(min_mix_timestamp);
                    tx_cd_data["max_mix_time"]     = xmreg::timestamp_to_str(max_mix_timestamp);
                    tx_cd_data["timescales_scale"] = fmt::format("{:0.2f}",
                                                              mixins_timescales.second
                                                              / 3600.0 / 24.0); // in days

                    // mark real mixing in the mixins timescale graph
                    mark_real_mixins_on_timescales(real_output_indices, tx_cd_data);

                    txs.push_back(tx_cd_data);

                } // for (const ::tools::wallet2::tx_construction_data& tx_cd: exported_txs.txes)
            }
            else
            {
                cerr << "deserialization of unsigned tx data NOT successful" << endl;
                return string("deserialization of unsigned tx data NOT successful. "
                                      "Maybe its not base64 encoded?");
            }
        } // if (unsigned_tx_given)
        else
        {
            // if raw data is not unsigined tx, then assume it is signed tx

            const size_t magiclen = strlen(SIGNED_TX_PREFIX);

            string data_prefix = xmreg::make_printable(decoded_raw_tx_data.substr(0, magiclen));

            if (strncmp(decoded_raw_tx_data.c_str(), SIGNED_TX_PREFIX, magiclen) != 0)
            {

                // ok, so its not signed tx data. but maybe it is raw tx data
                // used in rpc call "/sendrawtransaction". This is for example
                // used in mymonero and openmonero projects.

                // to check this, first we need to encode data back to base64.
                // the reason is that txs submited to "/sendrawtransaction"
                // are not base64, and we earlier always asume it is base64.

                // string reencoded_raw_tx_data = epee::string_encoding::base64_decode(raw_tx_data);

                //cout << "raw_tx_data: " << raw_tx_data << endl;

                cryptonote::blobdata tx_data_blob;

                if (!epee::string_tools::parse_hexstr_to_binbuff(raw_tx_data, tx_data_blob))
                {
                    string msg = fmt::format("The data is neither unsigned, signed tx or raw tx! "
                                              "Its prefix is: {:s}",
                                             data_prefix);

                    cout << msg << endl;

                    return string(msg);
                }

                crypto::hash tx_hash_from_blob;
                crypto::hash tx_prefix_hash_from_blob;
                cryptonote::transaction tx_from_blob;

                if (!cryptonote::parse_and_validate_tx_from_blob(tx_data_blob,
                                                                 tx_from_blob,
                                                                 tx_hash_from_blob,
                                                                 tx_prefix_hash_from_blob))
                {
                    string msg = fmt::format("failed to validate transaction");

                    cout << msg << endl;

                    return string(msg);
                }

                //cout << "tx_from_blob.vout.size(): " << tx_from_blob.vout.size() << endl;

                // tx has been correctly deserialized. So
                // we just dispaly it. We dont have any information about real mixins, etc,
                // so there is not much more we can do with tx data.

                mstch::map tx_context = construct_tx_context(tx_from_blob);

                if (boost::get<bool>(tx_context["has_error"]))
                {
                    return boost::get<string>(tx_context["error_msg"]);
                }

                // this will be stored in html for for checking outputs
                // we need this data if we want to use "Decode outputs"
                // to see which outputs are ours, and decode amounts in ringct txs
                tx_context["raw_tx_data"]            = raw_tx_data;
                tx_context["show_more_details_link"] = false;

                context["data_prefix"] = string("none as this is pure raw tx data");
                context["tx_json"]     = obj_to_json_str(tx_from_blob);

                context.emplace("txs"     , mstch::array{});

                boost::get<mstch::array>(context["txs"]).push_back(tx_context);

                map<string, string> partials {
                        {"tx_details", template_file["tx_details"]},
                };

                add_css_style(context);

                // render the page
                return mstch::render(template_file["checkrawtx"], context, partials);

            } // if (strncmp(decoded_raw_tx_data.c_str(), SIGNED_TX_PREFIX, magiclen) != 0)

            context["data_prefix"] = data_prefix;

            bool r {false};

            string s = decoded_raw_tx_data.substr(magiclen);

            ::tools::wallet2::signed_tx_set signed_txs;

            try
            {
                std::istringstream iss(s);
                boost::archive::portable_binary_iarchive ar(iss);
                ar >> signed_txs;

                r = true;
            }
            catch (...)
            {
                cerr << "Failed to parse signed tx data " << endl;
            }

            if (!r)
            {
                cerr << "deserialization of signed tx data NOT successful" << endl;
                return string("deserialization of signed tx data NOT successful. "
                                      "Maybe its not base64 encoded?");
            }

            std::vector<tools::wallet2::pending_tx> ptxs = signed_txs.ptx;

            context.insert({"txs", mstch::array{}});

            for (tools::wallet2::pending_tx& ptx: ptxs)
            {
                mstch::map tx_context = construct_tx_context(ptx.tx);

                if (boost::get<bool>(tx_context["has_error"]))
                {
                    return boost::get<string>(tx_context["error_msg"]);
                }

                tx_context["tx_prv_key"] =  fmt::format("{:s}", ptx.tx_key);

                mstch::array destination_addresses;
                vector<uint64_t> real_ammounts;
                uint64_t outputs_xmr_sum {0};

                // destiantion address for this tx
                for (tx_destination_entry& a_dest: ptx.construction_data.splitted_dsts)
                {
                    //stealth_address_amount.insert({dest.addr, dest.amount});
                    //cout << get_account_address_as_str(testnet, a_dest.addr) << endl;
                    //address_amounts.push_back(a_dest.amount);

                    destination_addresses.push_back(
                            mstch::map {
                                    {"dest_address"   , get_account_address_as_str(testnet, a_dest.addr)},
                                    {"dest_amount"    , xmreg::xmr_amount_to_str(a_dest.amount)},
                                    {"is_this_change" , false}
                            }
                    );

                    outputs_xmr_sum += a_dest.amount;

                    real_ammounts.push_back(a_dest.amount);
                }

                // get change address and amount info
                if (ptx.construction_data.change_dts.amount > 0)
                {
                    destination_addresses.push_back(
                            mstch::map {
                                    {"dest_address"   , get_account_address_as_str(testnet, ptx.construction_data.change_dts.addr)},
                                    {"dest_amount"    , xmreg::xmr_amount_to_str(ptx.construction_data.change_dts.amount)},
                                    {"is_this_change" , true}
                            }
                    );

                    real_ammounts.push_back(ptx.construction_data.change_dts.amount);
                };

                tx_context["outputs_xmr_sum"] = xmreg::xmr_amount_to_str(outputs_xmr_sum);

                tx_context.insert({"dest_infos", destination_addresses});

                // get reference to inputs array created of the tx
                mstch::array& outputs = boost::get<mstch::array>(tx_context["outputs"]);

                // show real output amount for ringct outputs.
                // otherwise its only 0.000000000
                for (size_t i = 0; i < outputs.size(); ++i)
                {
                    mstch::map& output_map = boost::get<mstch::map>(outputs.at(i));

                    string& out_amount_str = boost::get<string>(output_map["amount"]);

                    //cout << boost::get<string>(output_map["out_pub_key"])
                    //    <<", " <<  out_amount_str << endl;

                    uint64_t output_amount;

                    if (parse_amount(output_amount, out_amount_str))
                    {
                        if (output_amount == 0)
                        {
                            out_amount_str = xmreg::xmr_amount_to_str(real_ammounts.at(i));
                        }
                    }
                }

                // get public keys of real outputs
                vector<string>   real_output_pub_keys;
                vector<uint64_t> real_output_indices;
                vector<uint64_t> real_amounts;

                uint64_t inputs_xmr_sum {0};

                for (const tx_source_entry&  tx_source: ptx.construction_data.sources)
                {
                    transaction real_source_tx;

                    uint64_t index_of_real_output = tx_source.outputs[tx_source.real_output].first;

                    uint64_t tx_source_amount = (tx_source.rct ? 0 : tx_source.amount);

                    tx_out_index real_toi;

                    try
                    {
                        // get tx of the real output
                        real_toi =  core_storage->get_db()
                                .get_output_tx_and_index(tx_source_amount, index_of_real_output);
                    }
                    catch (const OUTPUT_DNE& e)
                    {

                        string out_msg = fmt::format(
                                "Output with amount {:d} and index {:d} does not exist!",
                                tx_source_amount, index_of_real_output
                        );

                        cerr << out_msg << endl;

                        return string(out_msg);
                    }

                    if (!mcore->get_tx(real_toi.first, real_source_tx))
                    {
                        cerr << "Cant get tx in blockchain: " << real_toi.first << endl;
                        return string("Cant get tx: " + pod_to_hex(real_toi.first));
                    }

                    tx_details real_txd = get_tx_details(real_source_tx);

                    public_key real_out_pub_key
                            = real_txd.output_pub_keys[tx_source.real_output_in_tx_index].first.key;

                    real_output_pub_keys.push_back(
                            REMOVE_HASH_BRAKETS(fmt::format("{:s}",real_out_pub_key))
                    );

                    real_output_indices.push_back(tx_source.real_output);
                    real_amounts.push_back(tx_source.amount);

                    inputs_xmr_sum += tx_source.amount;
                }

                // mark that we have signed tx data for use in mstch
                tx_context["have_raw_tx"] = true;

                // provide total mount of inputs xmr
                tx_context["inputs_xmr_sum"] = xmreg::xmr_amount_to_str(inputs_xmr_sum);

                // get reference to inputs array created of the tx
                mstch::array& inputs = boost::get<mstch::array>(tx_context["inputs"]);

                uint64_t input_idx {0};

                // mark which mixin is real in each input's mstch context
                for (mstch::node& input_node: inputs)
                {

                    mstch::map& input_map = boost::get<mstch::map>(input_node);

                    // show input amount
                    string& amount = boost::get<string>(
                            boost::get<mstch::map>(input_node)["amount"]
                    );

                    amount = xmreg::xmr_amount_to_str(real_amounts.at(input_idx));

                    // check if key images are spend or not

                    string& in_key_img_str = boost::get<string>(
                            boost::get<mstch::map>(input_node)["in_key_img"]
                    );

                    key_image key_imgage;

                    if (epee::string_tools::hex_to_pod(in_key_img_str, key_imgage))
                    {
                        input_map["already_spent"] = core_storage->get_db().has_key_image(key_imgage);
                    }

                    // mark real mixings

                    mstch::array& mixins = boost::get<mstch::array>(
                            boost::get<mstch::map>(input_node)["mixins"]
                    );

                    for (mstch::node& mixin_node: mixins)
                    {
                        mstch::map& mixin = boost::get<mstch::map>(mixin_node);

                        string mix_pub_key_str = boost::get<string>(mixin["mix_pub_key"]);

                        //cout << mix_pub_key_str << endl;

                        if (std::find(
                                real_output_pub_keys.begin(),
                                real_output_pub_keys.end(),
                                mix_pub_key_str) != real_output_pub_keys.end())
                        {
                            mixin["mix_is_it_real"] = true;
                        }
                    }

                    ++input_idx;
                }

                // mark real mixing in the mixins timescale graph
                mark_real_mixins_on_timescales(real_output_indices, tx_context);

                boost::get<mstch::array>(context["txs"]).push_back(tx_context);
            }

        }

        map<string, string> partials {
                {"tx_details", template_file["tx_details"]},
        };

        add_css_style(context);

        // render the page
        return mstch::render(template_file["checkrawtx"], context, partials);
    }

    string
    show_pushrawtx(string raw_tx_data, string action)
    {
        clean_post_data(raw_tx_data);

        string decoded_raw_tx_data = epee::string_encoding::base64_decode(raw_tx_data);

        const size_t magiclen = strlen(SIGNED_TX_PREFIX);

        string data_prefix = xmreg::make_printable(decoded_raw_tx_data.substr(0, magiclen));

        // initalize page template context map
        mstch::map context {
                {"testnet"              , testnet},
                {"have_custom_lmdb"     , have_custom_lmdb},
                {"have_raw_tx"          , true},
                {"has_error"            , false},
                {"error_msg"            , string {}},
                {"data_prefix"          , data_prefix},
        };
        context.emplace("txs", mstch::array{});

        // add header and footer
        string full_page = template_file["pushrawtx"];

        add_css_style(context);

        if (strncmp(decoded_raw_tx_data.c_str(), SIGNED_TX_PREFIX, magiclen) != 0)
        {
            string error_msg = fmt::format("The data does not appear to be signed raw tx! Data prefix: {:s}",
                                           data_prefix);

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        if (this->enable_pusher == false)
        {
            string error_msg = fmt::format(
                    "Pushing disabled!\n "
                    "Run explorer with --enable-pusher flag to enable it.");

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        bool r {false};

        string s = decoded_raw_tx_data.substr(magiclen);

        ::tools::wallet2::signed_tx_set signed_txs;

        try
        {
            std::istringstream iss(s);
            boost::archive::portable_binary_iarchive ar(iss);
            ar >> signed_txs;

            r = true;
        }
        catch (...)
        {
            cerr << "Failed to parse signed tx data " << endl;
        }


        if (!r)
        {
            string error_msg = fmt::format("Deserialization of signed tx data NOT successful! "
                                           "Maybe its not base64 encoded?");

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        mstch::array& txs = boost::get<mstch::array>(context["txs"]);

        std::vector<tools::wallet2::pending_tx> ptx_vector = signed_txs.ptx;

        // actually commit the transactions
        while (!ptx_vector.empty())
        {
            tools::wallet2::pending_tx& ptx = ptx_vector.back();

            tx_details txd = get_tx_details(ptx.tx);

            string tx_hash_str  = REMOVE_HASH_BRAKETS(fmt::format("{:s}", txd.hash));

            mstch::map tx_cd_data {
                    {"tx_hash"          , tx_hash_str}
            };

            // check in mempool already contains tx to be submited
            vector<pair<tx_info, transaction>> found_mempool_txs
                    = search_mempool(txd.hash);

            if (!found_mempool_txs.empty())
            {
                string error_msg = fmt::format("Tx already exist in the mempool: {:s}\n",
                                               tx_hash_str);

                context["has_error"] = true;
                context["error_msg"] = error_msg;

                break;
            }

            // check if tx to be submited already exists in the blockchain
            if (core_storage->have_tx(txd.hash))
            {
                string error_msg = fmt::format("Tx already exist in the blockchain: {:s}\n",
                               tx_hash_str);

                context["has_error"] = true;
                context["error_msg"] = error_msg;

                break;
            }

            // check if any key images of the tx to be submited are already spend
            vector<key_image> key_images_spent;

            for (const txin_to_key& tx_in: txd.input_key_imgs)
            {
                if (core_storage->have_tx_keyimg_as_spent(tx_in.k_image))
                    key_images_spent.push_back(tx_in.k_image);
            }

            if (!key_images_spent.empty())
            {
                string error_msg = fmt::format("Tx with hash {:s} has already spent inputs\n",
                                               tx_hash_str);

                for (key_image& k_img: key_images_spent)
                {
                    error_msg += REMOVE_HASH_BRAKETS(fmt::format("{:s}", k_img));
                    error_msg += "</br>";
                }

                context["has_error"] = true;
                context["error_msg"] = error_msg;

                break;
            }

            string rpc_error_msg;

            if (this->enable_pusher == false)
            {
                string error_msg = fmt::format(
                        "Pushing signed transactions is disabled. "
                        "Run explorer with --enable-pusher flag to enable it.\n");

                context["has_error"] = true;
                context["error_msg"] = error_msg;

                break;
            }

            if (!rpc.commit_tx(ptx, rpc_error_msg))
            {
                string error_msg = fmt::format(
                        "Submitting signed tx {:s} to daemon failed: {:s}\n",
                        tx_hash_str, rpc_error_msg);

                context["has_error"] = true;
                context["error_msg"] = error_msg;

                break;
            }

            txs.push_back(tx_cd_data);

            // if no exception, remove element from vector
            ptx_vector.pop_back();
        }

        // render the page
        return mstch::render(full_page, context);
    }


    string
    show_rawkeyimgs()
    {
        // initalize page template context map
        mstch::map context {
                {"testnet"            , testnet},
                {"have_custom_lmdb"   , have_custom_lmdb}
        };

        add_css_style(context);

        // render the page
        return mstch::render(template_file["rawkeyimgs"], context);
    }

    string
    show_rawoutputkeys()
    {
        // initalize page template context map
        mstch::map context {
                {"testnet"            , testnet},
                {"have_custom_lmdb"   , have_custom_lmdb}
        };

        add_css_style(context);

        // render the page
        return mstch::render(template_file["rawoutputkeys"], context);
    }

    string
    show_checkrawkeyimgs(string raw_data, string viewkey_str)
    {

        clean_post_data(raw_data);

        // remove white characters
        boost::trim(viewkey_str);

        string decoded_raw_data = epee::string_encoding::base64_decode(raw_data);
        secret_key prv_view_key;

        // initalize page template context map
        mstch::map context{
                {"testnet"         , testnet},
                {"have_custom_lmdb", have_custom_lmdb},
                {"has_error"       , false},
                {"error_msg"       , string{}},
        };

        // add header and footer
        string full_page = template_file["checkrawkeyimgs"];

        add_css_style(context);

        if (viewkey_str.empty())
        {
            string error_msg = fmt::format("View key not given. Cant decode "
                                           "the key image data without it!");

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        if (!xmreg::parse_str_secret_key(viewkey_str, prv_view_key))
        {
            string error_msg = fmt::format("Cant parse the private key: " + viewkey_str);

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        const size_t magiclen = strlen(KEY_IMAGE_EXPORT_FILE_MAGIC);

        string data_prefix = xmreg::make_printable(decoded_raw_data.substr(0, magiclen));

        context["data_prefix"] = data_prefix;

        if (!strncmp(decoded_raw_data.c_str(), KEY_IMAGE_EXPORT_FILE_MAGIC, magiclen) == 0)
        {
            string error_msg = fmt::format("This does not seem to be key image export data.");

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        // decrypt key images data using private view key
        decoded_raw_data = xmreg::decrypt(
                std::string(decoded_raw_data, magiclen),
                prv_view_key, true);

        if (decoded_raw_data.empty())
        {
            string error_msg = fmt::format("Failed to authenticate key images data. "
                                           "Maybe wrong viewkey was porvided?");

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        // header is public spend and keys
        const size_t header_lenght = 2 * sizeof(crypto::public_key);
        const size_t key_img_size  = sizeof(crypto::key_image);
        const size_t record_lenght = key_img_size + sizeof(crypto::signature);
        const size_t chacha_length = sizeof(crypto::chacha8_key);

        if (decoded_raw_data.size() < header_lenght)
        {
            string error_msg = fmt::format("Bad data size from submitted key images raw data.");

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);

        }

        // get xmr address stored in this key image file
         const account_public_address* xmr_address =
                reinterpret_cast<const account_public_address*>(
                        decoded_raw_data.data());

        context.insert({"address"        , REMOVE_HASH_BRAKETS(xmreg::print_address(*xmr_address, testnet))});
        context.insert({"viewkey"        , REMOVE_HASH_BRAKETS(fmt::format("{:s}", prv_view_key))});
        context.insert({"has_total_xmr"  , false});
        context.insert({"total_xmr"      , string{}});
        context.insert({"key_imgs"       , mstch::array{}});

        unique_ptr<xmreg::MyLMDB> mylmdb;

        if (bf::is_directory(lmdb2_path))
        {
            mylmdb = make_unique<xmreg::MyLMDB>(lmdb2_path);
        }
        else
        {
            cout << "Custom lmdb database seem does not exist at: " << lmdb2_path << endl;
        }


        size_t no_key_images = (decoded_raw_data.size() - header_lenght) / record_lenght;

        //vector<pair<crypto::key_image, crypto::signature>> signed_key_images;

        mstch::array& key_imgs_ctx = boost::get<mstch::array>(context["key_imgs"]);

        uint64_t total_xmr {0};


        for (size_t n = 0; n < no_key_images; ++n)
        {
            const char* record_ptr = decoded_raw_data.data() + header_lenght + n * record_lenght;

            crypto::key_image key_image
                    = *reinterpret_cast<const crypto::key_image*>(record_ptr);

            crypto::signature signature
                    = *reinterpret_cast<const crypto::signature*>(record_ptr + key_img_size);


            vector<string> found_tx_hashes;

            if (mylmdb)
            {
                mylmdb->search(epee::string_tools::pod_to_hex(key_image),
                               found_tx_hashes, "key_images");
            }

            mstch::map key_img_info {
                    {"key_no"              , fmt::format("{:03d}", n)},
                    {"key_image"           , REMOVE_HASH_BRAKETS(fmt::format("{:s}", key_image))},
                    {"signature"           , fmt::format("{:s}", signature)},
                    {"address"             , xmreg::print_address(*xmr_address, testnet)},
                    {"amount"              , string{}},
                    {"is_spent"            , core_storage->have_tx_keyimg_as_spent(key_image)},
                    {"tx_hash_found"       , !found_tx_hashes.empty()},
                    {"tx_hash"             , string{}}
            };


            if (!found_tx_hashes.empty())
            {
                string tx_hash_str = found_tx_hashes.at(0);

                key_img_info["tx_hash"]   = tx_hash_str;
                key_img_info["timestamp"] = "";

                transaction tx;

                if (mcore->get_tx(tx_hash_str, tx))
                {
                    crypto::hash tx_hash;

                    epee::string_tools::hex_to_pod(tx_hash_str, tx_hash);

                    // get timestamp of the tx's block
                    uint64_t blk_height    = core_storage
                            ->get_db().get_tx_block_height(tx_hash);

                    uint64_t blk_timestamp = core_storage
                            ->get_db().get_block_timestamp(blk_height);

                    vector<txin_to_key> tx_key_imgs = get_key_images(tx);

                    for (auto it = tx_key_imgs.begin(); it != tx_key_imgs.end(); ++it)
                    {
                        if ((*it).k_image != key_image)
                        {
                            continue;
                        }

                        uint64_t xmr_amount = (*it).amount;

                        // RingCT, i.e., tx version is 2
                        // thus need to decode the amounts
                        // otherwise they all appear to be zero.
                        // so to find the amount, first we need to find
                        // our output in the key images's mixins, and then
                        // decode it using ringct.
                        // this requires to loop over mixins of each key image,
                        // get its source txs, get mixin index in that tx,
                        // and check if its our mixin or not using private_view_key
                        // and public_spend_key_provided
                        if (tx.version == 2)
                        {

                            vector<uint64_t> absolute_offsets
                                    = relative_output_offsets_to_absolute((*it).key_offsets);

                            // need to go through each mixin, fetch its tx
                            // find its index in that txs, obtain tx_public_key
                            // derive a deriviec_key, compare derived output key
                            // with the output key in txs, and decode rct amount
                            // if equal
                            for (uint64_t out_idx: absolute_offsets)
                            {

                                //tx_out_index is pair<transaction hash, output index>
                                tx_out_index toi;

                                try
                                {
                                    // get pair<transaction hash, output index>
                                    toi = core_storage->get_db()
                                            .get_output_tx_and_index((*it).amount, out_idx);
                                }
                                catch (const OUTPUT_DNE& e)
                                {
                                    string error_msg = fmt::format(
                                            "Output with amount {:d} and index {:d} does not exist!",
                                            (*it).amount, out_idx);

                                    context["has_error"] = true;
                                    context["error_msg"] = error_msg;

                                    return mstch::render(full_page, context);
                                }

                                // get actual transaction structure from tx_hash
                                transaction output_source_tx;

                                if (!mcore->get_tx(toi.first, output_source_tx))
                                {
                                    string error_msg = fmt::format(
                                            "Cant get tx in blockchain: {:s}", toi.first);

                                    context["has_error"] = true;
                                    context["error_msg"] = error_msg;

                                    return mstch::render(full_page, context);
                                }

                                uint64_t output_idx_in_tx = toi.second;

                                // get tx outpout key
                                const txout_to_key& output_pub_key
                                        = boost::get<txout_to_key>(
                                                output_source_tx.vout[output_idx_in_tx].target);

                                uint64_t rct_amount = output_source_tx.vout[output_idx_in_tx].amount;

                                public_key tx_pub_key = xmreg::get_tx_pub_key_from_received_outs(
                                        output_source_tx);

                                // public transaction key is combined with our viewkey
                                // to create, so called, derived key.
                                key_derivation derivation;

                                if (!generate_key_derivation(tx_pub_key, prv_view_key, derivation))
                                {
                                    string error_msg = fmt::format(
                                            "Cant get derived key for "
                                            "pub_tx_key: {:s}, prv_view_key: {:s}"
                                            , tx_pub_key, prv_view_key);

                                    context["has_error"] = true;
                                    context["error_msg"] = error_msg;

                                    return mstch::render(full_page, context);
                                }

                                // get the tx output public key
                                // that normally would be generated for us,
                                // if someone had sent us some xmr.
                                public_key derived_output_pubkey;

                                derive_public_key(derivation,
                                                  output_idx_in_tx,
                                                  xmr_address->m_spend_public_key,
                                                  derived_output_pubkey);

                                // check if generated public key matches the current output's key
                                bool mine_output = (output_pub_key.key == derived_output_pubkey);

                                if (mine_output)
                                {
                                    // seems we found our output. so now lets decode its amount
                                    // using ringct

                                    if (output_source_tx.version == 2
                                        && !is_coinbase(output_source_tx))
                                    {
                                        bool r;

                                        r = decode_ringct(output_source_tx.rct_signatures,
                                                          tx_pub_key,
                                                          prv_view_key,
                                                          output_idx_in_tx,
                                                          output_source_tx.rct_signatures.ecdhInfo[output_idx_in_tx].mask,
                                                          rct_amount);

                                        if (!r)
                                        {
                                            string error_msg = fmt::format(
                                                    "Cant decode ringCT for "
                                                            "pub_tx_key: {:s} "
                                                            "using prv_view_key: {:s}",
                                                    tx_pub_key, prv_view_key);

                                            context["has_error"] = true;
                                            context["error_msg"] = error_msg;

                                            return mstch::render(full_page, context);
                                        }

                                        xmr_amount = rct_amount;

                                    } // if (output_source_tx.version == 2 && !is_coinbase(output_source_tx))

                                    break;

                                } // if (mine_output)

                            } // for (uint64_t out_idx: (*it).key_offsets)

                        } // if (tx.version == 2)

                        key_img_info["amount"]    = xmreg::xmr_amount_to_str(xmr_amount);
                        key_img_info["is_ringct"] = tx.version == 2 ? true: false;
                        total_xmr += xmr_amount;

                    } // for (it = tx_key_imgs.begin(); it != tx_key_imgs.end(); ++it)

                    key_img_info["timestamp"] = xmreg::timestamp_to_str(blk_timestamp);

                } // if (mcore->get_tx(tx_hash_str, tx))

            } // if (!found_tx_hashes.empty())

            key_imgs_ctx.push_back(key_img_info);

        } // for (size_t n = 0; n < no_key_images; ++n)

        if (total_xmr > 0)
        {
            context["has_total_xmr"] = true;
            context["total_xmr"] = xmreg::xmr_amount_to_str(total_xmr);
        }

        // render the page
        return mstch::render(full_page, context);
    }

    string
    show_checkcheckrawoutput(string raw_data, string viewkey_str)
    {
        clean_post_data(raw_data);

        // remove white characters
        boost::trim(viewkey_str);

        string decoded_raw_data = epee::string_encoding::base64_decode(raw_data);
        secret_key prv_view_key;

        // initalize page template context map
        mstch::map context{
                {"testnet"         , testnet},
                {"have_custom_lmdb", have_custom_lmdb},
                {"has_error"       , false},
                {"error_msg"       , string{}}
        };

        // add header and footer
        string full_page = template_file["checkoutputkeys"];

        add_css_style(context);

        if (viewkey_str.empty())
        {
            string error_msg = fmt::format("View key not given. Cant decode "
                                                   "the outputs data without it!");

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        if (!xmreg::parse_str_secret_key(viewkey_str, prv_view_key))
        {
            string error_msg = fmt::format("Cant parse the private key: " + viewkey_str);

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        const size_t magiclen = strlen(OUTPUT_EXPORT_FILE_MAGIC);

        string data_prefix = xmreg::make_printable(decoded_raw_data.substr(0, magiclen));

        context["data_prefix"] = data_prefix;

        if (!strncmp(decoded_raw_data.c_str(), OUTPUT_EXPORT_FILE_MAGIC, magiclen) == 0)
        {
            string error_msg = fmt::format("This does not seem to be output keys export data.");

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }


        // decrypt key images data using private view key
        decoded_raw_data = xmreg::decrypt(
                std::string(decoded_raw_data, magiclen),
                prv_view_key, true);


        if (decoded_raw_data.empty())
        {
            string error_msg = fmt::format("Failed to authenticate outputs data. "
                                           "Maybe wrong viewkey was porvided?");

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }


        // header is public spend and keys
        const size_t header_lenght    = 2 * sizeof(crypto::public_key);

        // get xmr address stored in this key image file
        const account_public_address* xmr_address =
                reinterpret_cast<const account_public_address*>(
                        decoded_raw_data.data());

        context.insert({"address"        , REMOVE_HASH_BRAKETS(xmreg::print_address(*xmr_address, testnet))});
        context.insert({"viewkey"        , REMOVE_HASH_BRAKETS(fmt::format("{:s}", prv_view_key))});
        context.insert({"has_total_xmr"  , false});
        context.insert({"total_xmr"      , string{}});
        context.insert({"output_keys"    , mstch::array{}});

        mstch::array& output_keys_ctx = boost::get<mstch::array>(context["output_keys"]);

        unique_ptr<xmreg::MyLMDB> mylmdb;

        if (bf::is_directory(lmdb2_path))
        {
            mylmdb = make_unique<xmreg::MyLMDB>(lmdb2_path);
        }
        else
        {
            cout << "Custom lmdb database seem does not exist at: " << lmdb2_path << endl;
        }

        std::vector<tools::wallet2::transfer_details> outputs;

        try
        {
            std::string body(decoded_raw_data, header_lenght);
            std::stringstream iss;
            iss << body;
            boost::archive::portable_binary_iarchive ar(iss);

            ar >> outputs;

            //size_t n_outputs = m_wallet->import_outputs(outputs);
        }
        catch (const std::exception &e)
        {
            string error_msg = fmt::format("Failed to import outputs: {:s}", e.what());

            context["has_error"] = true;
            context["error_msg"] = error_msg;

            return mstch::render(full_page, context);
        }

        uint64_t total_xmr {0};
        uint64_t output_no {0};

        context["are_key_images_known"] = false;

        for (const tools::wallet2::transfer_details& td: outputs)
        {

            const transaction_prefix& txp = td.m_tx;

            txout_to_key txout_key = boost::get<txout_to_key>(
                    txp.vout[td.m_internal_output_index].target);

            uint64_t xmr_amount = td.amount();

            // if the output is RingCT, i.e., tx version is 2
            // need to decode its amount
            if (td.m_tx.version == 2)
            {
                // get tx associated with the given output
                transaction tx;

                if (!mcore->get_tx(td.m_txid, tx))
                {
                    string error_msg = fmt::format("Cant get tx of hash: {:s}", td.m_txid);

                    context["has_error"] = true;
                    context["error_msg"] = error_msg;

                    return mstch::render(full_page, context);
                }

                public_key tx_pub_key = xmreg::get_tx_pub_key_from_received_outs(tx);

                bool r = decode_ringct(tx.rct_signatures,
                                       tx_pub_key,
                                       prv_view_key,
                                       td.m_internal_output_index,
                                       tx.rct_signatures.ecdhInfo[td.m_internal_output_index].mask,
                                       xmr_amount);

                if (!r)
                {
                    string error_msg = fmt::format(
                            "Cant decode RingCT for output: {:s}",
                            txout_key.key);

                    context["has_error"] = true;
                    context["error_msg"] = error_msg;

                    return mstch::render(full_page, context);
                }
            }

            uint64_t blk_timestamp = core_storage
                    ->get_db().get_block_timestamp(td.m_block_height);

            const key_image* output_key_img;

            bool is_output_spent {false};

            if (td.m_key_image_known)
            {
                //are_key_images_known

                output_key_img = &td.m_key_image;

                is_output_spent = core_storage->have_tx_keyimg_as_spent(*output_key_img);

                context["are_key_images_known"] = true;
            }

            mstch::map output_info {
                    {"output_no"           , fmt::format("{:03d}", output_no)},
                    {"output_pub_key"      , REMOVE_HASH_BRAKETS(fmt::format("{:s}", txout_key.key))},
                    {"amount"              , xmreg::xmr_amount_to_str(xmr_amount)},
                    {"tx_hash"             , REMOVE_HASH_BRAKETS(fmt::format("{:s}", td.m_txid))},
                    {"timestamp"           , xmreg::timestamp_to_str(blk_timestamp)},
                    {"is_spent"            , is_output_spent},
                    {"is_ringct"           , td.m_rct}
            };

            ++output_no;

            if (!is_output_spent)
            {
                total_xmr += xmr_amount;
            }

            output_keys_ctx.push_back(output_info);
        }

        if (total_xmr > 0)
        {
            context["has_total_xmr"] = true;
            context["total_xmr"] = xmreg::xmr_amount_to_str(total_xmr);
        }

        return mstch::render(full_page, context);;
    }


    string
    search(string search_text)
    {
        // remove white characters
        boost::trim(search_text);

        string default_txt {"No such thing found: " + search_text};

        string result_html {default_txt};

        // check first if we look for output with given global index
        // such search start with "goi_", e.g., "goi_543"
        bool search_for_global_output_idx = (search_text.substr(0, 4) == "goi_");

        // check if we look for output with amout index and amount
        // such search start with "aoi_", e.g., "aoi_444-23.00"
        bool search_for_amount_output_idx = (search_text.substr(0, 4) == "aoi_");

        // first check if searching for block of given height
        if (search_text.size() < 12 &&
                            (search_for_global_output_idx == false
                             ||search_for_amount_output_idx == false))
        {
            uint64_t blk_height;

            try
            {
                blk_height = boost::lexical_cast<uint64_t>(search_text);

                result_html = show_block(blk_height);

                // nasty check if output is "Cant get" as a sign of
                // a not found tx. Later need to think of something better.
                if (result_html.find("Cant get") == string::npos)
                {
                     return result_html;
                }
            }
            catch(boost::bad_lexical_cast &e)
            {
                cerr << fmt::format("Parsing {:s} into uint64_t failed", search_text)
                     << endl;
            }
        }


        // check if monero address is given based on its length
        // if yes, then we can only show its public components
        if (search_text.length() == 95)
        {
            // parse string representing given monero address
            cryptonote::account_public_address address;

            bool testnet_addr {false};

            if (search_text[0] == '9' || search_text[0] == 'A')
                testnet_addr = true;

            if (!xmreg::parse_str_address(search_text, address, testnet_addr))
            {
                cerr << "Cant parse string address: " << search_text << endl;
                return string("Cant parse address (probably incorrect format): ")
                       + search_text;
            }

            return show_address_details(address, testnet_addr);
        }

        // check if integrated monero address is given based on its length
        // if yes, then show its public components search tx based on encrypted id
        if (search_text.length() == 106)
        {

            cryptonote::account_public_address address;

            bool has_payment_id;

            crypto::hash8 encrypted_payment_id;

            bool testnet;

            if (!get_account_integrated_address_from_str(address,
                                                    has_payment_id,
                                                    encrypted_payment_id,
                                                    testnet,
                                                    search_text))
            {
                cerr << "Cant parse string integerated address: " << search_text << endl;
                return string("Cant parse address (probably incorrect format): ")
                       + search_text;
            }

            return show_integrated_address_details(address, encrypted_payment_id, testnet);
        }

        // second let try searching for tx
        result_html = show_tx(search_text);

        // nasty check if output is "Cant get" as a sign of
        // a not found tx. Later need to think of something better.
        if (result_html.find("Cant get") == string::npos)
        {
             return result_html;
        }

        // if tx search not successful, check if we are looking
        // for a block with given hash
        result_html = show_block(search_text);

        if (result_html.find("Cant get") == string::npos)
        {
             return result_html;
        }

        result_html = default_txt;

        // get mempool transaction so that what we search,
        // might be there. Note: show_tx above already searches it
        // but only looks for tx hash. Now want to check
        // for key_images, public_keys, payments_id, etc.
        vector<transaction> mempool_txs = get_mempool_txs();

        // key is string indicating where search_text was found.
        map<string, vector<string>> tx_search_results
                            = search_txs(mempool_txs, search_text);

        // now search my own custom lmdb database
        // with key_images, public_keys, payments_id etc.

        vector<pair<string, vector<string>>> all_possible_tx_hashes;

        try
        {
            unique_ptr<xmreg::MyLMDB> mylmdb;

            if (!bf::is_directory(lmdb2_path))
            {
                cout << "Custom lmdb database seem does not exist at: " << lmdb2_path << endl;

                result_html = show_search_results(search_text, all_possible_tx_hashes);

                return result_html;
            }

            cout << "Custom lmdb database seem to exist at: " << lmdb2_path << endl;
            cout << "So lets try to search there for what we are after." << endl;

            mylmdb = make_unique<xmreg::MyLMDB>(lmdb2_path);

            // check if date given in format: 2015-04-15 12:02:33
            // this is 19 characters
            if (search_text.length() == 19)
            {
                uint64_t estimated_blk_height {0};

                // first parse the string to date::sys_seconds and then to timestamp
                // since epoch
                uint64_t blk_timestamp_utc = parse(search_text).time_since_epoch().count();

                if (blk_timestamp_utc)
                {
                    // seems we have a correct date!
                    // so try to estimate block height from it.
                    //
                    // to find block we can use our lmdb outputs_info table
                    // its indexes are timestamps.

                    vector<xmreg::output_info> out_infos;

                    if (mylmdb->get_output_info(blk_timestamp_utc, out_infos))
                    {
                        // since many outputs can be in a single block
                        // just get the first one to obtained its block

                        uint64_t found_blk_height = core_storage->get_db()
                                .get_tx_block_height(out_infos.at(0).tx_hash);

                        return show_block(found_blk_height);
                    }
                }
            }
            else if (search_text.length() == 16)
            {
                // check if date given in format: 2015-04-15 12:02
                // this is 16 characters, i.e., only minut given
                // so search all blocks made within that minute

                // first parse the string to date::sys_seconds and then to timestamp
                // since epoch
                uint64_t blk_timestamp_utc_start
                        = parse(search_text, "%Y-%m-%d %H:%M")
                                .time_since_epoch().count();

                if (blk_timestamp_utc_start)
                {
                    // seems we have a correct date!

                    // add 60 seconds, i.e. 1 min
                    uint64_t blk_timestamp_utc_end
                            = blk_timestamp_utc_start + 59;

                    all_possible_tx_hashes.push_back(
                            make_pair("tx_in_the_minute", vector<string>{}));

                    vector<string>& txs_found_ref
                            = all_possible_tx_hashes.back().second;

                    get_txs_from_timestamp_range(
                            blk_timestamp_utc_start,
                            blk_timestamp_utc_end,
                            mylmdb,
                            txs_found_ref);
                }
            }
            else if (search_text.length() == 13)
            {
                // check if date given in format: 2015-04-15 12
                // this is 13 characters, i.e., only hour given
                // so search all blocks made within that hour

                // first parse the string to date::sys_seconds and then to timestamp
                // since epoch
                uint64_t blk_timestamp_utc_start
                        = parse(search_text, "%Y-%m-%d %H")
                                .time_since_epoch().count();

                if (blk_timestamp_utc_start)
                {
                    // seems we have a correct date!

                    // add 60 seconds, i.e. 1 hour
                    uint64_t blk_timestamp_utc_end
                            = blk_timestamp_utc_start + 3599;

                    all_possible_tx_hashes.push_back(
                            make_pair("tx_in_the_hour", vector<string>{}));

                    vector<string>& txs_found_ref
                            = all_possible_tx_hashes.back().second;

                    get_txs_from_timestamp_range(
                            blk_timestamp_utc_start,
                            blk_timestamp_utc_end,
                            mylmdb,
                            txs_found_ref);
                }
            }
            else if (search_text.length() == 10)
            {
                // check if date given in format: 2015-04-15
                // this is 10 characters, i.e., only day given
                // so search all blocks made within that day

                // first parse the string to date::sys_seconds and then to timestamp
                // since epoch
                uint64_t blk_timestamp_utc_start
                        = parse(search_text, "%Y-%m-%d")
                                .time_since_epoch().count();

                if (blk_timestamp_utc_start)
                {
                    // seems we have a correct date!

                    // add 60 seconds, i.e. 1 day
                    uint64_t blk_timestamp_utc_end
                            = blk_timestamp_utc_start + 86399;

                    all_possible_tx_hashes.push_back(
                            make_pair("tx_in_the_day", vector<string>{}));

                    vector<string>& txs_found_ref
                            = all_possible_tx_hashes.back().second;

                    get_txs_from_timestamp_range(
                            blk_timestamp_utc_start,
                            blk_timestamp_utc_end,
                            mylmdb,
                            txs_found_ref);
                }
            }

            mylmdb->search(search_text,
                           tx_search_results["key_images"],
                           "key_images");

            //cout << "size: " << tx_search_results["key_images"].size() << endl;

            all_possible_tx_hashes.push_back(
                    make_pair("key_images",
                              tx_search_results["key_images"]));


            // search the custum lmdb for tx_public_keys and append the result
            // to those from the mempool search if found

            mylmdb->search(search_text,
                           tx_search_results["tx_public_keys"],
                           "tx_public_keys");

            if (!tx_search_results["tx_public_keys"].empty())
            {
                all_possible_tx_hashes.push_back(
                        make_pair("tx_public_keys",
                                  tx_search_results["tx_public_keys"]));
            }
            else
            {
                // if private tx key is added, use it to obtained tx_public_key
                // and than search for corresponding tx

                public_key tx_pub_key = null_pkey;
                secret_key tx_prv_key;

                if (hex_to_pod(search_text, tx_prv_key))
                {
                    secret_key recovery_key = tx_prv_key;

                    const unsigned char * tx_prv_key_ptr = reinterpret_cast<const unsigned char *>(&tx_prv_key);
                    unsigned char * tx_pub_key_ptr = reinterpret_cast<unsigned char *>(&tx_pub_key);

                    //memcpy(&tx_pub_key.data, reinterpret_cast<char*>(tx_pub_key_ptr), sizeof(tx_pub_key.data));

                    ge_p3 point;
                    ge_scalarmult_base(&point, tx_prv_key_ptr);
                    ge_p3_tobytes(tx_pub_key_ptr, &point);

                    string tx_pub_key_str = pod_to_hex(tx_pub_key);

                    mylmdb->search(tx_pub_key_str,
                                   tx_search_results["tx_public_keys"],
                                   "tx_public_keys");

                    all_possible_tx_hashes.push_back(
                            make_pair("tx_public_keys",
                                      tx_search_results["tx_public_keys"]));
                }
            }


            // search the custum lmdb for payments_id and append the result
            // to those from the mempool search if found

            mylmdb->search(search_text,
                           tx_search_results["payments_id"],
                           "payments_id");

            all_possible_tx_hashes.push_back(
                    make_pair("payments_id",
                              tx_search_results["payments_id"]));

            // search the custum lmdb for encrypted_payments_id and append the result
            // to those from the mempool search if found

            mylmdb->search(search_text,
                           tx_search_results["encrypted_payments_id"],
                           "encrypted_payments_id");

            all_possible_tx_hashes.push_back(
                    make_pair("encrypted_payments_id",
                              tx_search_results["encrypted_payments_id"]));

            // search the custum lmdb for output_public_keys and append the result
            // to those from the mempool search if found

            mylmdb->search(search_text,
                           tx_search_results["output_public_keys"],
                           "output_public_keys");

            all_possible_tx_hashes.push_back(
                    make_pair("output_public_keys",
                              tx_search_results["output_public_keys"]));


            // seach for output using output global index

            if (search_for_global_output_idx)
            {
                try
                {
                    uint64_t global_idx = boost::lexical_cast<uint64_t>(
                            search_text.substr(4));


                    output_data_t output_data;

                    try
                    {
                        // get info about output of a given global index
                        output_data = core_storage->get_db()
                                .get_output_key(global_idx);
                    }
                    catch (const OUTPUT_DNE& e)
                    {
                        string out_msg = fmt::format(
                                "Output with index {:d} does not exist!",
                                global_idx
                        );

                        cerr << out_msg << endl;

                        return out_msg;
                    }

                    //cout << "tx_out.first: " << tx_out.first << endl;
                    //cout << "tx_out.second: " << tx_out.second << endl;

                    string output_pub_key = pod_to_hex(output_data.pubkey);

                    //cout << "output_pub_key: " << output_pub_key << endl;

                    vector<string> found_outputs;

                    mylmdb->search(output_pub_key,
                                   found_outputs,
                                   "output_public_keys");

                    //cout << "found_outputs.size(): " << found_outputs.size() << endl;

                    all_possible_tx_hashes.push_back(
                            make_pair("output_public_keys_based_on_global_idx",
                                      found_outputs));

                }
                catch(boost::bad_lexical_cast &e)
                {
                    cerr << "Cant cast global_idx string: "
                         << search_text.substr(4) << endl;
                }
            } //  if (search_for_global_output_idx)

            // seach for output using output amount index and amount

            if (search_for_amount_output_idx)
            {
                try
                {

                    string str_to_split = search_text.substr(4);

                    vector<string> string_parts;

                    boost::split(string_parts, str_to_split,
                                 boost::is_any_of("-"));

                    if (string_parts.size() != 2)
                    {
                        throw;
                    }

                    uint64_t amount_idx = boost::lexical_cast<uint64_t>(
                            string_parts[0]);

                    uint64_t amount = static_cast<uint64_t>
                    (boost::lexical_cast<double>(
                                    string_parts[1]) * 1e12);


                    //cout << "amount_idx: " << amount_idx << endl;
                    //cout << "amount: "     << amount << endl;

                    output_data_t output_data;

                    try
                    {
                        // get info about output of a given global index
                        output_data = core_storage->get_db()
                                .get_output_key(
                                        amount, amount_idx);
                    }
                    catch (const OUTPUT_DNE& e)
                    {
                        string out_msg = fmt::format(
                                "Output with amount {:d} and index {:d} does not exist!",
                                amount, amount_idx
                        );

                        cerr << out_msg << endl;

                        return out_msg;
                    }

                    string output_pub_key = pod_to_hex(output_data.pubkey);

                    //cout << "output_pub_key: " << output_pub_key << endl;

                    vector<string> found_outputs;

                    mylmdb->search(output_pub_key,
                                   found_outputs,
                                   "output_public_keys");

                    //cout << "found_outputs.size(): " << found_outputs.size() << endl;

                    all_possible_tx_hashes.push_back(
                            make_pair("output_public_keys_based_on_amount_idx",
                                      found_outputs));

                }
                catch(boost::bad_lexical_cast& e)
                {
                    cerr << "Cant parse amout index and amout string: "
                         << search_text.substr(4) << endl;
                }
                catch(const OUTPUT_DNE& e)
                {
                    cerr << "Output not found in the blockchain: "
                         << search_text.substr(4) << endl;

                    return(string("Output not found in the blockchain: ")
                           + search_text.substr(4));
                }
            } // if (search_for_amount_output_idx)
        }
        catch (const lmdb::runtime_error& e)
        {
            cerr << "Error opening/accessing custom lmdb database: "
                 << e.what() << endl;
        }
        catch (std::exception& e)
        {
            cerr << "Error opening/accessing custom lmdb database: "
                 << e.what() << endl;
        }


        result_html = show_search_results(search_text, all_possible_tx_hashes);

        return result_html;
    }

    string
    show_address_details(const account_public_address& address, bool testnet = false)
    {

        string address_str      = xmreg::print_address(address, testnet);
        string pub_viewkey_str  = fmt::format("{:s}", address.m_view_public_key);
        string pub_spendkey_str = fmt::format("{:s}", address.m_spend_public_key);

        mstch::map context {
                {"xmr_address"        , REMOVE_HASH_BRAKETS(address_str)},
                {"public_viewkey"     , REMOVE_HASH_BRAKETS(pub_viewkey_str)},
                {"public_spendkey"    , REMOVE_HASH_BRAKETS(pub_spendkey_str)},
                {"is_integrated_addr" , false},
                {"testnet"            , testnet},
                {"have_custom_lmdb"   , have_custom_lmdb}
        };

        add_css_style(context);

        // render the page
        return mstch::render(template_file["address"], context);
    }

    // ;
    string
    show_integrated_address_details(const account_public_address& address,
                                    const crypto::hash8& encrypted_payment_id,
                                    bool testnet = false)
    {

        string address_str        = xmreg::print_address(address, testnet);
        string pub_viewkey_str    = fmt::format("{:s}", address.m_view_public_key);
        string pub_spendkey_str   = fmt::format("{:s}", address.m_spend_public_key);
        string enc_payment_id_str = fmt::format("{:s}", encrypted_payment_id);

        mstch::map context {
                {"xmr_address"          , REMOVE_HASH_BRAKETS(address_str)},
                {"public_viewkey"       , REMOVE_HASH_BRAKETS(pub_viewkey_str)},
                {"public_spendkey"      , REMOVE_HASH_BRAKETS(pub_spendkey_str)},
                {"encrypted_payment_id" , REMOVE_HASH_BRAKETS(enc_payment_id_str)},
                {"is_integrated_addr"   , true},
                {"testnet"              , testnet},
                {"have_custom_lmdb"     , have_custom_lmdb}
        };

        add_css_style(context);

        // render the page
        return mstch::render(template_file["address"], context);
    }

    map<string, vector<string>>
    search_txs(vector<transaction> txs, const string& search_text)
    {
        map<string, vector<string>> tx_hashes;

        // initlizte the map with empty results
        tx_hashes["key_images"]                             = {};
        tx_hashes["tx_public_keys"]                         = {};
        tx_hashes["payments_id"]                            = {};
        tx_hashes["encrypted_payments_id"]                  = {};
        tx_hashes["output_public_keys"]                     = {};

        for (const transaction& tx: txs)
        {

            tx_details txd = get_tx_details(tx);

            string tx_hash_str = pod_to_hex(txd.hash);

            // check if any key_image matches the search_text

            vector<txin_to_key>::iterator it1 =
                find_if(begin(txd.input_key_imgs), end(txd.input_key_imgs),
                    [&](const txin_to_key& key_img)
                    {
                        return pod_to_hex(key_img.k_image) == search_text;
                    });

            if (it1 != txd.input_key_imgs.end())
            {
                tx_hashes["key_images"].push_back(tx_hash_str);
            }

            // check if  tx_public_key matches the search_text

            if (pod_to_hex(txd.pk) == search_text)
            {
                tx_hashes["tx_public_keys"].push_back(tx_hash_str);
            }

            // check if  payments_id matches the search_text

            if (pod_to_hex(txd.payment_id) == search_text)
            {
                tx_hashes["payment_id"].push_back(tx_hash_str);
            }

            // check if  encrypted_payments_id matches the search_text

            if (pod_to_hex(txd.payment_id8) == search_text)
            {
                tx_hashes["encrypted_payments_id"].push_back(tx_hash_str);
            }

            // check if output_public_keys matche the search_text

            vector<pair<txout_to_key, uint64_t>>::iterator it2 =
                find_if(begin(txd.output_pub_keys), end(txd.output_pub_keys),
                    [&](const pair<txout_to_key, uint64_t>& tx_out_pk)
                    {
                        return pod_to_hex(tx_out_pk.first.key) == search_text;
                    });

            if (it2 != txd.output_pub_keys.end())
            {
                tx_hashes["output_public_keys"].push_back(tx_hash_str);
            }

        }

        return  tx_hashes;

    }

    vector<transaction>
    get_mempool_txs()
    {
        // get mempool data using rpc call
        vector<pair<tx_info, transaction>> mempool_data = search_mempool();

        // output only transactions
        vector<transaction> mempool_txs;

        mempool_txs.reserve(mempool_data.size());

        for (const auto& a_pair: mempool_data)
        {
            mempool_txs.push_back(a_pair.second);
        }

        return mempool_txs;
    }

    string
    show_search_results(const string& search_text,
        const vector<pair<string, vector<string>>>& all_possible_tx_hashes)
    {

        // initalise page tempate map with basic info about blockchain
        mstch::map context {
                {"testnet"         , testnet},
                {"search_text"     , search_text},
                {"no_results"      , true},
                {"to_many_results" , false},
                {"have_custom_lmdb", have_custom_lmdb}
        };

        for (const pair<string, vector<string>>& found_txs: all_possible_tx_hashes)
        {
            // define flag, e.g., has_key_images denoting that
            // tx hashes for key_image searched were found
            context.insert({"has_" + found_txs.first, !found_txs.second.empty()});

            // cout << "found_txs.first: " << found_txs.first << endl;

            // insert new array based on what we found to context if not exist
            pair< mstch::map::iterator, bool> res
                    = context.insert({found_txs.first, mstch::array{}});

            if (!found_txs.second.empty())
            {

                uint64_t tx_i {0};

                // for each found tx_hash, get the corresponding tx
                // and its details, and put into mstch for rendering
                for (const string& tx_hash: found_txs.second)
                {

                    crypto::hash tx_hash_pod;

                    epee::string_tools::hex_to_pod(tx_hash, tx_hash_pod);

                    transaction tx;

                    uint64_t blk_height {0};

                    int64_t blk_timestamp;

                    // first check in the blockchain
                    if (mcore->get_tx(tx_hash, tx))
                    {

                        // get timestamp of the tx's block
                        blk_height    = core_storage
                                ->get_db().get_tx_block_height(tx_hash_pod);

                        blk_timestamp = core_storage
                                ->get_db().get_block_timestamp(blk_height);

                    }
                    else
                    {
                        // check in mempool if tx_hash not found in the
                        // blockchain
                        vector<pair<tx_info, transaction>> found_txs
                                = search_mempool(tx_hash_pod);

                        if (!found_txs.empty())
                        {
                            // there should be only one tx found
                            tx = found_txs.at(0).second;
                        }
                        else
                        {
                            return string("Cant get tx of hash (show_search_results): " + tx_hash);
                        }

                        // tx in mempool have no blk_timestamp
                        // but can use their recive time
                         blk_timestamp = found_txs.at(0).first.receive_time;

                    }

                    tx_details txd = get_tx_details(tx);

                    mstch::map txd_map = txd.get_mstch_map();


                    // add the timestamp to tx mstch map
                    txd_map.insert({"timestamp", xmreg::timestamp_to_str(blk_timestamp)});

                    boost::get<mstch::array>((res.first)->second).push_back(txd_map);

                    // dont show more than 500 results
                    if (tx_i > 500)
                    {
                        context["to_many_results"] = true;
                        break;
                    }

                    ++tx_i;
                }

                // if found something, set this flag to indicate this fact
                context["no_results"] = false;
            }
        }

        // add header and footer
        string full_page = template_file["search_results"];

        // read partial for showing details of tx(s) found
        map<string, string> partials {
            {"tx_table_head", template_file["tx_table_head"]},
            {"tx_table_row" , template_file["tx_table_row"]}
        };

        add_css_style(context);

        // render the page
        return  mstch::render(full_page, context, partials);
    }


private:


    void
    mark_real_mixins_on_timescales(
            const vector<uint64_t>& real_output_indices,
            mstch::map& tx_context)
    {
        // mark real mixing in the mixins timescale graph
        mstch::array& mixins_timescales
                = boost::get<mstch::array>(tx_context["timescales"]);

        uint64_t idx {0};

        for (mstch::node& timescale_node: mixins_timescales)
        {

            string& timescale = boost::get<string>(
                    boost::get<mstch::map>(timescale_node)["timescale"]
            );

            // claculated number of timescale points
            // due to resolution, no of points might be lower than no of mixins
            size_t no_points = std::count(timescale.begin(), timescale.end(), '*');

            size_t point_to_find = real_output_indices.at(idx);

            // adjust point to find based on total number of points
            if (point_to_find >= no_points)
                point_to_find = no_points  - 1;

            boost::iterator_range<string::iterator> r
                    = boost::find_nth(timescale, "*", point_to_find);

            *(r.begin()) = 'R';

            ++idx;
        }
    }

    mstch::map
    construct_tx_context(transaction tx, uint16_t with_ring_signatures = 0)
    {
        tx_details txd = get_tx_details(tx);

        crypto::hash tx_hash = txd.hash;

        string tx_hash_str = REMOVE_HASH_BRAKETS(fmt::format("{:s}", tx_hash));

        uint64_t tx_blk_height {0};

        bool tx_blk_found {false};

        if (core_storage->have_tx(tx_hash))
        {
            // currently get_tx_block_height seems to return a block hight
            // +1. Before it was not like this.
            tx_blk_height = core_storage->get_db().get_tx_block_height(tx_hash) - 1;
            tx_blk_found = true;
        }

        // get block cointaining this tx
        block blk;

        if (tx_blk_found && !mcore->get_block_by_height(tx_blk_height, blk))
        {
            cerr << "Cant get block: " << tx_blk_height << endl;
        }

        string tx_blk_height_str {"N/A"};

        // tx age
        pair<string, string> age;

        string blk_timestamp {"N/A"};

        if (tx_blk_found)
        {
            // calculate difference between tx and server timestamps
            age = get_age(server_timestamp, blk.timestamp, FULL_AGE_FORMAT);

            blk_timestamp = xmreg::timestamp_to_str(blk.timestamp);

            tx_blk_height_str = std::to_string(tx_blk_height);
        }

        // payments id. both normal and encrypted (payment_id8)
        string pid_str   = REMOVE_HASH_BRAKETS(fmt::format("{:s}", txd.payment_id));
        string pid8_str  = REMOVE_HASH_BRAKETS(fmt::format("{:s}", txd.payment_id8));


        string tx_json = obj_to_json_str(tx);

        // initalise page tempate map with basic info about blockchain
        mstch::map context {
                {"testnet"               , testnet},
                {"have_custom_lmdb"      , have_custom_lmdb},
                {"tx_hash"               , tx_hash_str},
                {"tx_prefix_hash"        , pod_to_hex(txd.prefix_hash)},                
                {"tx_pub_key"            , REMOVE_HASH_BRAKETS(fmt::format("{:s}", txd.pk))},
                {"blk_height"            , tx_blk_height_str},
                {"tx_blk_height"         , tx_blk_height},
                {"tx_size"               , fmt::format("{:0.4f}",
                                                      static_cast<double>(txd.size) / 1024.0)},
                {"tx_fee"                , xmreg::xmr_amount_to_str(txd.fee)},
                {"tx_version"            , fmt::format("{:d}", txd.version)},
                {"blk_timestamp"         , blk_timestamp},
                {"blk_timestamp_uint"    , blk.timestamp},
                {"delta_time"            , age.first},
                {"inputs_no"             , static_cast<uint64_t>(txd.input_key_imgs.size())},
                {"has_inputs"            , !txd.input_key_imgs.empty()},
                {"outputs_no"            , static_cast<uint64_t>(txd.output_pub_keys.size())},
                {"has_payment_id"        , txd.payment_id  != null_hash},
                {"has_payment_id8"       , txd.payment_id8 != null_hash8},
                {"confirmations"         , txd.no_confirmations},
                {"payment_id"            , pid_str},
                {"payment_id8"           , pid8_str},
                {"extra"                 , txd.get_extra_str()},
                {"with_ring_signatures"  , static_cast<bool>(
                                                  with_ring_signatures)},
                {"tx_json"               , tx_json},
                {"is_ringct"             , (tx.version > 1)},
                {"rct_type"              , tx.rct_signatures.type},
                {"has_error"             , false},
                {"error_msg"             , string("")},
                {"have_raw_tx"           , false},
                {"show_more_details_link", true},
                {"from_cache"            , false},
                {"construction_time"     , string {}},
        };

        string server_time_str = xmreg::timestamp_to_str(server_timestamp, "%F");

        mstch::array inputs = mstch::array{};

        uint64_t input_idx {0};

        uint64_t inputs_xmr_sum {0};

        // ringct inputs can be mixture of known amounts (when old outputs)
        // are spent, and unknown umounts (makrked in explorer by '?') when
        // ringct outputs are spent. thus we totalling input amounts
        // in such case, we need to show sum of known umounts, and
        // indicate that this is minium sum, as we dont know the unknown
        // umounts.
        bool have_any_unknown_amount {false};

        vector<vector<uint64_t>> mixin_timestamp_groups;

        // make timescale maps for mixins in input
        for (const txin_to_key &in_key: txd.input_key_imgs)
        {
            // get absolute offsets of mixins
            std::vector<uint64_t> absolute_offsets
                    = cryptonote::relative_output_offsets_to_absolute(
                            in_key.key_offsets);

            // get public keys of outputs used in the mixins that match to the offests
            std::vector<cryptonote::output_data_t> outputs;

            try
            {
                core_storage->get_db().get_output_key(in_key.amount,
                                                      absolute_offsets,
                                                      outputs);
            }
            catch (const OUTPUT_DNE &e)
            {
                string out_msg = fmt::format(
                        "Outputs with amount {:d} do not exist and indexes ",
                        in_key.amount
                );

                for (auto offset: absolute_offsets)
                    out_msg += ", " + to_string(offset);

                out_msg += " don't exist!";

                cerr << out_msg << endl;

                context["has_error"] = true;
                context["error_msg"] = out_msg;

                return context;
            }

            inputs.push_back(mstch::map {
                    {"in_key_img", REMOVE_HASH_BRAKETS(fmt::format("{:s}", in_key.k_image))},
                    {"amount",        xmreg::xmr_amount_to_str(in_key.amount)},
                    {"input_idx",     fmt::format("{:02d}", input_idx)},
                    {"mixins",        mstch::array{}},
                    {"ring_sigs",     txd.get_ring_sig_for_input(input_idx)},
                    {"already_spent", false} // placeholder for later
            });

            inputs_xmr_sum += in_key.amount;

            if (in_key.amount == 0)
            {
                // if any input has amount equal to zero,
                // it is really an unkown amount
                have_any_unknown_amount = true;
            }

            vector<uint64_t> mixin_timestamps;

            // get reference to mixins array created above
            mstch::array &mixins = boost::get<mstch::array>(
                    boost::get<mstch::map>(inputs.back())["mixins"]);

            // mixin counter
            size_t count = 0;

            // for each found output public key find its block to get timestamp
            for (const uint64_t &i: absolute_offsets)
            {
                // get basic information about mixn's output
                cryptonote::output_data_t output_data = outputs.at(count);

                tx_out_index tx_out_idx;

                try
                {
                    // get pair pair<crypto::hash, uint64_t> where first is tx hash
                    // and second is local index of the output i in that tx
                    tx_out_idx = core_storage->get_db()
                            .get_output_tx_and_index(in_key.amount, i);
                }
                catch (const OUTPUT_DNE &e)
                {

                    string out_msg = fmt::format(
                            "Output with amount {:d} and index {:d} does not exist!",
                            in_key.amount, i
                    );

                    cerr << out_msg << endl;

                    context["has_error"] = true;
                    context["error_msg"] = out_msg;

                    return context;
                }


                if (enable_mixins_details)
                {
                    // get block of given height, as we want to get its timestamp
                    cryptonote::block blk;

                    if (!mcore->get_block_by_height(output_data.height, blk))
                    {
                        cerr << "- cant get block of height: " << output_data.height << endl;

                        context["has_error"] = true;
                        context["error_msg"] = fmt::format("- cant get block of height: {}",
                                                           output_data.height);
                    }

                    // get age of mixin relative to server time
                    pair<string, string> mixin_age = get_age(server_timestamp,
                                                             blk.timestamp,
                                                             FULL_AGE_FORMAT);
                    // get mixin transaction
                    transaction mixin_tx;

                    if (!mcore->get_tx(tx_out_idx.first, mixin_tx))
                    {
                        cerr << "Cant get tx: " << tx_out_idx.first << endl;

                        context["has_error"] = true;
                        context["error_msg"] = fmt::format("Cant get tx: {:s}", tx_out_idx.first);
                    }

                    // mixin tx details
                    tx_details mixin_txd = get_tx_details(mixin_tx, true);

                    mixins.push_back(mstch::map {
                            {"mix_blk",        fmt::format("{:08d}", output_data.height)},
                            {"mix_pub_key",     REMOVE_HASH_BRAKETS(fmt::format("{:s}",
                                                                            output_data.pubkey))},
                            {"mix_tx_hash",      REMOVE_HASH_BRAKETS(fmt::format("{:s}",
                                                                            tx_out_idx.first))},
                            {"mix_out_indx",   fmt::format("{:d}", tx_out_idx.second)},
                            {"mix_timestamp",  xmreg::timestamp_to_str(blk.timestamp)},
                            {"mix_age",        mixin_age.first},
                            {"mix_mixin_no",   mixin_txd.mixin_no},
                            {"mix_inputs_no",  static_cast<uint64_t>(mixin_txd.input_key_imgs.size())},
                            {"mix_outputs_no", static_cast<uint64_t>(mixin_txd.output_pub_keys.size())},
                            {"mix_age_format", mixin_age.second},
                            {"mix_idx",        fmt::format("{:02d}", count)},
                            {"mix_is_it_real", false}, // a placeholder for future
                    });

                    // get mixin timestamp from its orginal block
                    mixin_timestamps.push_back(blk.timestamp);
                }
                else
                {
                    mixins.push_back(mstch::map {
                            {"mix_blk",         fmt::format("{:08d}", output_data.height)},
                            {"mix_pub_key",     REMOVE_HASH_BRAKETS(fmt::format("{:s}",
                                                                                output_data.pubkey))},
                            {"mix_idx",        fmt::format("{:02d}", count)},
                            {"mix_is_it_real", false}, // a placeholder for future
                    });

                } // else  if (enable_mixins_details)

                ++count;

            } // for (const uint64_t &i: absolute_offsets)

            mixin_timestamp_groups.push_back(mixin_timestamps);

            input_idx++;
        } // for (const txin_to_key& in_key: txd.input_key_imgs)



        if (enable_mixins_details)
        {
            uint64_t min_mix_timestamp {0};
            uint64_t max_mix_timestamp {0};

            pair<mstch::array, double> mixins_timescales
                    = construct_mstch_mixin_timescales(
                            mixin_timestamp_groups,
                            min_mix_timestamp,
                            max_mix_timestamp
                    );


            context["min_mix_time"]     = xmreg::timestamp_to_str(min_mix_timestamp);
            context["max_mix_time"]     = xmreg::timestamp_to_str(max_mix_timestamp);

            context.emplace("timescales", mixins_timescales.first);


            context["timescales_scale"] = fmt::format("{:0.2f}",
                                                      mixins_timescales.second / 3600.0 / 24.0); // in days

        }


        context["have_any_unknown_amount"] = have_any_unknown_amount;
        context["inputs_xmr_sum_not_zero"] = (inputs_xmr_sum > 0);
        context["inputs_xmr_sum"]          = xmreg::xmr_amount_to_str(inputs_xmr_sum);
        context["server_time"]             = server_time_str;
        context["enable_mixins_details"]     = enable_mixins_details;

        context.emplace("inputs", inputs);

        // get indices of outputs in amounts tables
        vector<uint64_t> out_amount_indices;

        try
        {

            uint64_t tx_index;

            if (core_storage->get_db().tx_exists(txd.hash, tx_index))
            {
                out_amount_indices = core_storage->get_db()
                        .get_tx_amount_output_indices(tx_index);
            }
            else
            {
                cerr << "get_tx_outputs_gindexs failed to find transaction with id = " << txd.hash;
            }

        }
        catch(const exception& e)
        {
            cerr << e.what() << endl;
        }

        uint64_t output_idx {0};

        mstch::array outputs;

        uint64_t outputs_xmr_sum {0};

        for (pair<txout_to_key, uint64_t>& outp: txd.output_pub_keys)
        {

            // total number of ouputs in the blockchain for this amount
            uint64_t num_outputs_amount = core_storage->get_db()
                    .get_num_outputs(outp.second);

            string out_amount_index_str {"N/A"};

            // outputs in tx in them mempool dont have yet global indices
            // thus for them, we print N/A
            if (!out_amount_indices.empty())
            {
                out_amount_index_str = fmt::format("{:d}",
                                                   out_amount_indices.at(output_idx));
            }

            outputs_xmr_sum += outp.second;

            outputs.push_back(mstch::map {
                    {"out_pub_key"   , REMOVE_HASH_BRAKETS(fmt::format("{:s}", outp.first.key))},
                    {"amount"        , xmreg::xmr_amount_to_str(outp.second)},
                    {"amount_idx"    , out_amount_index_str},
                    {"num_outputs"   , fmt::format("{:d}", num_outputs_amount)},
                    {"output_idx"    , fmt::format("{:02d}", output_idx++)}
            });
        }

        context["outputs_xmr_sum"] = xmreg::xmr_amount_to_str(outputs_xmr_sum);

        context.emplace("outputs", outputs);


        return context;
    }

    pair<mstch::array, double>
    construct_mstch_mixin_timescales(
            const vector<vector<uint64_t>>& mixin_timestamp_groups,
            uint64_t& min_mix_timestamp,
            uint64_t& max_mix_timestamp
    )
    {
        mstch::array mixins_timescales;

        double timescale_scale {0.0};     // size of one '_' in days

        // initialize with some large and some numbers
        min_mix_timestamp = server_timestamp*2L;
        max_mix_timestamp = 0;

        // find min and maximum timestamps
        for (const vector<uint64_t>& mixn_timestamps : mixin_timestamp_groups)
        {

            uint64_t min_found = *min_element(mixn_timestamps.begin(), mixn_timestamps.end());
            uint64_t max_found = *max_element(mixn_timestamps.begin(), mixn_timestamps.end());

            if (min_found < min_mix_timestamp)
                min_mix_timestamp = min_found;

            if (max_found > max_mix_timestamp)
                max_mix_timestamp = max_found;
        }


        min_mix_timestamp -= 3600;
        max_mix_timestamp += 3600;

        // make timescale maps for mixins in input with adjusted range
        for (auto& mixn_timestamps : mixin_timestamp_groups)
        {
            // get mixins in time scale for visual representation
            pair<string, double> mixin_times_scale = xmreg::timestamps_time_scale(
                    mixn_timestamps,
                    max_mix_timestamp,
                    170,
                    min_mix_timestamp);

            // save resolution of mixin timescales
            timescale_scale = mixin_times_scale.second;

            // save the string timescales for later to show
            mixins_timescales.push_back(mstch::map {
                    {"timescale", mixin_times_scale.first}
            });
        }

        return make_pair(mixins_timescales, timescale_scale);
    }


    tx_details
    get_tx_details(const transaction& tx, bool coinbase = false)
    {
        tx_details txd;

        // get tx hash
        txd.hash = get_transaction_hash(tx);

        // get tx prefix hash
        txd.prefix_hash = get_transaction_prefix_hash(tx);

        // get tx public key from extra
        // this check if there are two public keys
        // due to previous bug with sining txs:
        // https://github.com/monero-project/monero/pull/1358/commits/7abfc5474c0f86e16c405f154570310468b635c2
        txd.pk = xmreg::get_tx_pub_key_from_received_outs(tx);

        // sum xmr in inputs and ouputs in the given tx
        txd.xmr_inputs  = sum_money_in_inputs(tx);
        txd.xmr_outputs = sum_money_in_outputs(tx);
        txd.num_nonrct_inputs = count_nonrct_inputs(tx);

        // get mixin number
        txd.mixin_no    = get_mixin_no(tx);

        txd.fee = 0;

        transaction tx_copy = tx;

        txd.json_representation = obj_to_json_str(tx_copy);


        if (!coinbase &&  tx.vin.size() > 0)
        {
            // check if not miner tx
            // i.e., for blocks without any user transactions
            if (tx.vin.at(0).type() != typeid(txin_gen))
            {
                // get tx fee
                txd.fee = get_tx_fee(tx);
            }
        }

        get_payment_id(tx, txd.payment_id, txd.payment_id8);


        //blobdata tx_blob = t_serializable_object_to_blob(tx);

        // get tx size in bytes
        txd.size = get_object_blobsize(tx);
        //txd.size = tx_blob.size();
        //txd.size = core_storage->get_db().get_block_size();

        txd.input_key_imgs  = get_key_images(tx);
        txd.output_pub_keys = get_ouputs(tx);

        txd.extra = tx.extra;

        // get tx signatures for each input
        txd.signatures = tx.signatures;

        // get tx version
        txd.version = tx.version;

        // get unlock time
        txd.unlock_time = tx.unlock_time;

        txd.no_confirmations = 0;

        if (core_storage->have_tx(txd.hash))
        {
            txd.blk_height = core_storage->get_db().get_tx_block_height(txd.hash);

            // get the current blockchain height. Just to check
            uint64_t bc_height = core_storage->get_current_blockchain_height();

            txd.no_confirmations = bc_height - (txd.blk_height - 1);
        }

        return txd;
    }

    void
    clean_post_data(string& raw_tx_data)
    {
        // remove white characters
        boost::trim(raw_tx_data);
        boost::erase_all(raw_tx_data, "\r\n");
        boost::erase_all(raw_tx_data, "\n");

        // remove header and footer from base64 data
        // produced by certutil.exe in windows
        boost::erase_all(raw_tx_data, "-----BEGIN CERTIFICATE-----");
        boost::erase_all(raw_tx_data, "-----END CERTIFICATE-----");
    }

    bool
    get_txs_from_timestamp_range(
            uint64_t timestamp_start,
            uint64_t timestamp_end,
            const unique_ptr<xmreg::MyLMDB>& mylmdb,
            vector<string>& out_txs)
    {

        vector<crypto::hash> txs_found;

        if (mylmdb->get_txs_from_timestamp_range(
                timestamp_start,
                timestamp_end,
                txs_found))
        {

            for (auto tf: txs_found)
            {
                out_txs.push_back(pod_to_hex(tf));
            }

            return true;
        }

        return false;
    }

    vector<pair<tx_info, transaction>>
    search_mempool(crypto::hash tx_hash = null_hash)
    {

        vector<pair<tx_info, transaction>> found_txs;

        // get txs in the mempool
        std::vector<tx_info> mempool_txs;

        if (!rpc.get_mempool(mempool_txs))
        {
          cerr << "Getting mempool failed " << endl;
          return found_txs;
        }

        // if we have tx blob disply more.
        // this info can also be obtained from json that is
        // normally returned by the RCP call (see below in detailed view)
        if (HAVE_TX_BLOB)
        {
            // get tx_blob if exists
            //string tx_blob = get_tx_blob(_tx_info);

            for (size_t i = 0; i < mempool_txs.size(); ++i)
            {
                // get transaction info of the tx in the mempool
                tx_info _tx_info = mempool_txs.at(i);

                // get tx_blob if exists
                string tx_blob = get_tx_blob(_tx_info);

                if (tx_blob.empty())
                {
                    cerr << "tx_blob is empty. Probably its not a custom deamon." << endl;
                    continue;
                }

                // pare tx_blob into tx class
                transaction tx;

                if (!parse_and_validate_tx_from_blob(
                        tx_blob, tx))
                {
                    cerr << "Cant get tx from blob" << endl;
                    continue;
                }


                // if we dont provide tx_hash, just get all txs in
                // the mempool
                if (tx_hash != null_hash)
                {
                    // check if tx hash matches, and if yes, save it for return
                    if (tx_hash == get_transaction_hash(tx))
                    {
                        found_txs.push_back(make_pair(_tx_info, tx));
                        break;
                    }
                }
                else
                {
                   found_txs.push_back(make_pair(_tx_info, tx));
                }

            }
        }
        else
        {
            // if dont have tx_blob member, construct tx
            // from json obtained from the rpc call

            for (size_t i = 0; i < mempool_txs.size(); ++i)
            {
                // get transaction info of the tx in the mempool
                tx_info _tx_info = mempool_txs.at(i);

                crypto::hash mem_tx_hash = null_hash;

                if (hex_to_pod(_tx_info.id_hash, mem_tx_hash))
                {
                    transaction tx;

                    //cout << "\n\n\n_tx_info.id_hash:" << _tx_info.id_hash << endl;

                    if (!xmreg::make_tx_from_json(_tx_info.tx_json, tx))
                    {
                        cerr << "Cant make tx from _tx_info.tx_json" << endl;
                        continue;
                    }

                    if (_tx_info.id_hash != pod_to_hex(get_transaction_hash(tx)))
                    {
                        cerr << "Hash of reconstructed tx from json does not match "
                                "what we should get!"
                             << endl;
                        continue;
                    }

                    if (tx_hash == mem_tx_hash)
                    {
                        found_txs.push_back(make_pair(_tx_info, tx));
                        break;
                    }
                }
            }
        }

        return found_txs;
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


    string
    get_full_page(const string& middle)
    {
        return template_file["header"]
               + middle
               + template_file["footer"];
    }

    string
    get_footer()
    {
        // set last git commit date based on
        // autogenrated version.h during compilation
        static const mstch::map footer_context {
                {"last_git_commit_hash", string {GIT_COMMIT_HASH}},
                {"last_git_commit_date", string {GIT_COMMIT_DATETIME}},
                {"monero_version_full" , string {MONERO_VERSION_FULL}},
        };

        string footer_html = mstch::render(xmreg::read(TMPL_FOOTER), footer_context);

        return footer_html;
    }

    void
    add_css_style(mstch::map& context)
    {
        context["css_styles"] = mstch::lambda{[&](const std::string& text) -> mstch::node {
            return template_file["css_styles"];
        }};
    }

};
}


#endif //CROWXMR_PAGE_H


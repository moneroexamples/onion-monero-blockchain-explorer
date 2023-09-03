//
// Created by mwo on 8/04/16.
//

#ifndef CROWXMR_PAGE_H
#define CROWXMR_PAGE_H



#include "mstch/mstch.hpp"

#include "monero_headers.h"
#include "randomx.h"
#include "common.hpp"
#include "blake2/blake2.h"
#include "virtual_machine.hpp"
#include "program.hpp"
#include "aes_hash.hpp"
#include "assembly_generator_x86.hpp"

#include "../gen/version.h"

#include "MicroCore.h"
#include "tools.h"
#include "rpccalls.h"

#include "CurrentBlockchainStatus.h"
#include "MempoolStatus.h"

#include "../ext/crow_all.h"

#include "../ext/json.hpp"

#include "../ext/mstch/src/visitor/render_node.hpp"

extern "C" uint64_t me_rx_seedheight(const uint64_t height);

// forked version of the rx_slow_hash from monero
extern "C" void me_rx_slow_hash(const uint64_t mainheight, const uint64_t seedheight,
                             const char *seedhash,
                             const void *data, size_t length,
                             char *hash, int miners, int is_alt);
//extern "C" void me_rx_reorg(const uint64_t split_height);

extern  __thread randomx_vm *main_vm_full;

#include <algorithm>
#include <limits>
#include <ctime>
#include <future>
#include <type_traits>


#define TMPL_DIR                    "./templates"
#define TMPL_PARIALS_DIR            TMPL_DIR "/partials"
#define TMPL_CSS_STYLES             TMPL_DIR "/css/style.css"
#define TMPL_INDEX                  TMPL_DIR "/index.html"
#define TMPL_INDEX2                 TMPL_DIR "/index2.html"
#define TMPL_MEMPOOL                TMPL_DIR "/mempool.html"
#define TMPL_ALTBLOCKS              TMPL_DIR "/altblocks.html"
#define TMPL_MEMPOOL_ERROR          TMPL_DIR "/mempool_error.html"
#define TMPL_HEADER                 TMPL_DIR "/header.html"
#define TMPL_FOOTER                 TMPL_DIR "/footer.html"
#define TMPL_BLOCK                  TMPL_DIR "/block.html"
#define TMPL_RANDOMX                TMPL_DIR "/randomx.html"
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

#define ONIONEXPLORER_RPC_VERSION_MAJOR 1
#define ONIONEXPLORER_RPC_VERSION_MINOR 2
#define MAKE_ONIONEXPLORER_RPC_VERSION(major,minor) (((major)<<16)|(minor))
#define ONIONEXPLORER_RPC_VERSION \
    MAKE_ONIONEXPLORER_RPC_VERSION(ONIONEXPLORER_RPC_VERSION_MAJOR, ONIONEXPLORER_RPC_VERSION_MINOR)



// helper to ignore any number of template parametrs
template<typename...> using VoidT = void;

// primary template;
template<typename, typename = VoidT<>>
struct HasSpanInGetOutputKeyT: std::false_type
{};

//partial specialization (myy be SFINAEed away)
template <typename T>
struct HasSpanInGetOutputKeyT<
    T, 
    VoidT<decltype(std::declval<T>()
            .get_output_key(
                std::declval<const epee::span<const uint64_t>&>(),
                std::declval<const std::vector<uint64_t>&>(),
                std::declval<std::vector<cryptonote::output_data_t>&>()))
    >>: std::true_type
{};


// primary template;
template<typename, typename = VoidT<>>
struct OutputIndicesReturnVectOfVectT : std::false_type 
{};

template<typename T>
struct OutputIndicesReturnVectOfVectT<
    T,
    VoidT<decltype(std::declval<T>()
            .get_tx_amount_output_indices(
                uint64_t{}, size_t{})
            .front().front())
    >>: std::true_type 
{};


/**
 * visitor to produce json representations of
 * values stored in mstch::node
 */
class mstch_node_to_json: public boost::static_visitor<nlohmann::json>
{
public:


    // enabled for numeric types
    template<typename T>
    typename  std::enable_if<std::is_arithmetic<T>::value, nlohmann::json>::type
    operator()(T const& value) const {
        return nlohmann::json {value};
    }

    nlohmann::json operator()(std::string const& value) const {
        return nlohmann::json {value};
    }

    nlohmann::json operator()(mstch::map const& n_map) const
    {
        nlohmann::json j;

        for (auto const& kv: n_map)
            j[kv.first] = boost::apply_visitor(mstch_node_to_json(), kv.second);

        return j;
    }

    nlohmann::json operator()(mstch::array const& n_array) const
    {
        nlohmann::json j;

        for (auto const& v:  n_array)
            j.push_back(boost::apply_visitor(mstch_node_to_json(), v));

        return j;

    }

    // catch other types that are non-numeric and not listed above
    template<typename T>
    typename std::enable_if<!std::is_arithmetic<T>::value, nlohmann::json>::type
    operator()(const T&) const {
        return nlohmann::json {};
    }

};

namespace mstch
{
namespace internal
{
    // add conversion from mstch::map to nlohmann::json
    void
    to_json(nlohmann::json& j, mstch::map const &m)
    {
        for (auto const& kv: m)
            j[kv.first] = boost::apply_visitor(mstch_node_to_json(), kv.second);
    }
}
}

namespace xmreg
{


using namespace cryptonote;
using namespace crypto;
using namespace std;

using epee::string_tools::pod_to_hex;
using epee::string_tools::hex_to_pod;

template< typename T >
std::string as_hex(T i)
{
  std::stringstream ss;

  ss << "0x" << setfill ('0') << setw(sizeof(T)*2) 
         << hex << i;
  return ss.str();
}

struct randomx_status
{
    randomx::Program prog;
    randomx::RegisterFile reg_file;

    randomx::AssemblyGeneratorX86 
    get_asm() 
    {
        randomx::AssemblyGeneratorX86 asmX86;
        asmX86.generateProgram(prog);
    	return asmX86;
    }

    mstch::map
    get_mstch() 
    {
        auto asmx86 = get_asm();

        stringstream ss1, ss2;

        ss1 << prog;
        asmx86.printCode(ss2);
        
        mstch::map rx_map {
            {"rx_code" , ss1.str()},
            {"rx_code_asm", ss2.str()}
        };

	for (size_t i = 0; i < randomx::RegistersCount; ++i)
	{
	    rx_map["r"+std::to_string(i)] = as_hex(reg_file.r[i]);
	}
	
	for (size_t i = 0; i < randomx::RegistersCount/2; ++i)
	{
	    rx_map["f"+std::to_string(i)] = rx_float_as_str(reg_file.f[i]);
	    rx_map["e"+std::to_string(i)] = rx_float_as_str(reg_file.e[i]);
	    rx_map["a"+std::to_string(i)] = rx_float_as_str(reg_file.a[i]);
	}

        return rx_map;
    }

    string
    rx_float_as_str(randomx::fpu_reg_t fpu)
    {
	uint64_t* lo = reinterpret_cast<uint64_t*>(&fpu.lo);	
	uint64_t* hi = reinterpret_cast<uint64_t*>(&fpu.hi);	

	return 	 "{" + as_hex(*lo) + ", " + as_hex(*hi)+ "}";
    }
};

// modified version of the get_block_longhash
// from monero to use me_rx_slow_hash
bool
me_get_block_longhash(const Blockchain *pbc,
                   const block& b,
                   crypto::hash& res,
                   const uint64_t height,
                   const int miners)
{
  // block 202612 bug workaround
  if (height == 202612)
  {
    static const std::string longhash_202612 = "84f64766475d51837ac9efbef1926486e58563c95a19fef4aec3254f03000000";
    epee::string_tools::hex_to_pod(longhash_202612, res);
    return true;
  }
  blobdata bd = get_block_hashing_blob(b);
  if (b.major_version >= RX_BLOCK_VERSION)
  {
    uint64_t seed_height, main_height;
    crypto::hash hash;

    if (pbc != NULL)
    {
      seed_height = me_rx_seedheight(height);
      hash = pbc->get_pending_block_id_by_height(seed_height);
      main_height = pbc->get_current_blockchain_height();
    } else
    {
      memset(&hash, 0, sizeof(hash));  // only happens when generating genesis block
      seed_height = 0;
      main_height = 0;
    }

    me_rx_slow_hash(main_height, seed_height,
                    hash.data, bd.data(),
                    bd.size(), res.data, miners, 0);
  }
  return true;
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
    std::vector<crypto::public_key> additional_pks;
    uint64_t xmr_inputs;
    uint64_t xmr_outputs;
    uint64_t num_nonrct_inputs;
    uint64_t fee;
    uint64_t mixin_no;
    uint64_t size;
    uint64_t blk_height;
    size_t   version;

    bool has_additional_tx_pub_keys {false};

    uint64_t unlock_time;
    uint64_t no_confirmations;
    vector<uint8_t> extra;

    crypto::hash  payment_id  = null_hash; // normal
    crypto::hash8 payment_id8 = null_hash8; // encrypted

    std::vector<std::vector<crypto::signature>> signatures;

    // key images of inputs
    vector<txin_to_key> input_key_imgs;

    // public keys and xmr amount of outputs
    vector<output_tuple_with_tag> output_pub_keys;

    mstch::map
    get_mstch_map() const
    {

        string mixin_str {"N/A"};
        string fee_str {"N/A"};
        string fee_short_str {"N/A"};
        string payed_for_kB_str {""};
        string fee_micro_str {"N/A"};
        string payed_for_kB_micro_str {""};

        const double& xmr_amount = XMR_AMOUNT(fee);

        // tx size in kB
        double tx_size =  static_cast<double>(size)/1024.0;


        if (!input_key_imgs.empty())
        {
            double payed_for_kB = xmr_amount / tx_size;

            mixin_str        = std::to_string(mixin_no);
            fee_str          = fmt::format("{:0.6f}", xmr_amount);
            fee_short_str    = fmt::format("{:0.4f}", xmr_amount);
            fee_micro_str    = fmt::format("{:04.0f}" , xmr_amount * 1e6);
            payed_for_kB_str = fmt::format("{:0.4f}", payed_for_kB);
            payed_for_kB_micro_str = fmt::format("{:04.0f}", payed_for_kB * 1e6);
        }


        mstch::map txd_map {
                {"hash"              , pod_to_hex(hash)},
                {"prefix_hash"       , pod_to_hex(prefix_hash)},
                {"pub_key"           , pod_to_hex(pk)},
                {"tx_fee"            , fee_str},
                {"tx_fee_short"      , fee_short_str},
                {"fee_micro"         , fee_micro_str},
                {"payed_for_kB"      , payed_for_kB_str},
                {"payed_for_kB_micro", payed_for_kB_micro_str},
                {"sum_inputs"        , xmr_amount_to_str(xmr_inputs , "{:0.6f}")},
                {"sum_outputs"       , xmr_amount_to_str(xmr_outputs, "{:0.6f}")},
                {"sum_inputs_short"  , xmr_amount_to_str(xmr_inputs , "{:0.3f}")},
                {"sum_outputs_short" , xmr_amount_to_str(xmr_outputs, "{:0.3f}")},
                {"no_inputs"         , static_cast<uint64_t>(input_key_imgs.size())},
                {"no_outputs"        , static_cast<uint64_t>(output_pub_keys.size())},
                {"no_nonrct_inputs"  , num_nonrct_inputs},
                {"mixin"             , mixin_str},
                {"blk_height"        , blk_height},
                {"version"           , static_cast<uint64_t>(version)},
                {"has_payment_id"    , payment_id  != null_hash},
                {"has_payment_id8"   , payment_id8 != null_hash8},
                {"payment_id"        , pod_to_hex(payment_id)},
                {"confirmations"     , no_confirmations},
                {"extra"             , get_extra_str()},
                {"payment_id8"       , pod_to_hex(payment_id8)},
                {"unlock_time"       , unlock_time},
                {"tx_size"           , fmt::format("{:0.4f}", tx_size)},
                {"tx_size_short"     , fmt::format("{:0.2f}", tx_size)},
                {"has_add_pks"       , !additional_pks.empty()}
        };


        return txd_map;
    }


    string
    get_extra_str() const
    {
        return epee::string_tools::buff_to_hex_nodelimer(
                string{reinterpret_cast<const char*>(extra.data()), extra.size()});
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

    ~tx_details() {};
};


class page
{

static const bool FULL_AGE_FORMAT {true};

MicroCore* mcore;
Blockchain* core_storage;
rpccalls rpc;

atomic<time_t> server_timestamp;


cryptonote::network_type nettype;
bool mainnet;
bool testnet;
bool stagenet;


bool enable_pusher;

bool enable_randomx;

bool enable_key_image_checker;
bool enable_output_key_checker;
bool enable_mixins_details;
bool enable_as_hex;
bool enable_mixin_guess;

bool enable_autorefresh_option;

uint64_t no_of_mempool_tx_of_frontpage;
uint64_t no_blocks_on_index;
uint64_t mempool_info_timeout;

string testnet_url;
string stagenet_url;
string mainnet_url;

string js_html_files;
string js_html_files_all_in_one;

// instead of constatnly reading template files
// from hard drive for each request, we can read
// them only once, when the explorer starts into this map
// this will improve performance of the explorer and reduce
// read operation in OS
map<string, string> template_file;

public:

page(MicroCore* _mcore,
     Blockchain* _core_storage,
     string _daemon_url,
     cryptonote::network_type _nettype,
     bool _enable_pusher,
     bool _enable_randomx,
     bool _enable_as_hex,
     bool _enable_key_image_checker,
     bool _enable_output_key_checker,
     bool _enable_autorefresh_option,
     bool _enable_mixins_details,
     bool _enable_mixin_guess,
     uint64_t _no_blocks_on_index,
     uint64_t _mempool_info_timeout,
     string _testnet_url,
     string _stagenet_url,
     string _mainnet_url,
     rpccalls::login_opt _daemon_rpc_login)
        : mcore {_mcore},
          core_storage {_core_storage},
          rpc {_daemon_url, _daemon_rpc_login},
          server_timestamp {std::time(nullptr)},
          nettype {_nettype},
          enable_pusher {_enable_pusher},
          enable_randomx {_enable_randomx},
          enable_as_hex {_enable_as_hex},
          enable_key_image_checker {_enable_key_image_checker},
          enable_output_key_checker {_enable_output_key_checker},
          enable_autorefresh_option {_enable_autorefresh_option},
          enable_mixins_details {_enable_mixins_details},
          enable_mixin_guess {_enable_mixin_guess},
          no_blocks_on_index {_no_blocks_on_index},
          mempool_info_timeout {_mempool_info_timeout},
          testnet_url {_testnet_url},
          stagenet_url {_stagenet_url},
          mainnet_url {_mainnet_url}
{
    mainnet = nettype == cryptonote::network_type::MAINNET;
    testnet = nettype == cryptonote::network_type::TESTNET;
    stagenet = nettype == cryptonote::network_type::STAGENET;

    no_of_mempool_tx_of_frontpage = 25;

    // read template files for all the pages
    // into template_file map

    template_file["css_styles"]      = xmreg::read(TMPL_CSS_STYLES);
    template_file["header"]          = xmreg::read(TMPL_HEADER);
    template_file["footer"]          = get_footer();
    template_file["index2"]          = get_full_page(xmreg::read(TMPL_INDEX2));
    template_file["mempool"]         = xmreg::read(TMPL_MEMPOOL);
    template_file["altblocks"]       = get_full_page(xmreg::read(TMPL_ALTBLOCKS));
    template_file["mempool_error"]   = xmreg::read(TMPL_MEMPOOL_ERROR);
    template_file["mempool_full"]    = get_full_page(template_file["mempool"]);
    template_file["block"]           = get_full_page(xmreg::read(TMPL_BLOCK));
    template_file["randomx"]         = get_full_page(xmreg::read(TMPL_RANDOMX));
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
    template_file["tx_table_header"] = xmreg::read(string(TMPL_PARIALS_DIR) + "/tx_table_header.html");
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
    // get mempool for the front page also using async future
    std::future<string> mempool_ftr = std::async(std::launch::async, [&]
    {
        // get memory pool rendered template
        return mempool(false, no_of_mempool_tx_of_frontpage);
    });

    //get current server timestamp
    server_timestamp = std::time(nullptr);

    uint64_t local_copy_server_timestamp = server_timestamp;

    // get the current blockchain height. Just to check
    uint64_t height = core_storage->get_current_blockchain_height();

    // number of last blocks to show
    uint64_t no_of_last_blocks = std::min(no_blocks_on_index + 1, height);

    // initalise page tempate map with basic info about blockchain
    mstch::map context {
            {"testnet"                  , testnet},
            {"stagenet"                 , stagenet},
            {"testnet_url"              , testnet_url},
            {"stagenet_url"             , stagenet_url},
            {"mainnet_url"              , mainnet_url},
            {"refresh"                  , refresh_page},
            {"height"                   , height},
            {"server_timestamp"         , xmreg::timestamp_to_str_gm(local_copy_server_timestamp)},
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

    int64_t end_height = start_height + no_of_last_blocks - 1;

    vector<double> blk_sizes;

    // loop index
    int64_t i = end_height;

    // iterate over last no_of_last_blocks of blocks
    while (i >= start_height)
    {
        // get block at the given height i
        block blk;

        if (!mcore->get_block_by_height(i, blk))
        {
            cerr << "Cant get block: " << i << endl;
            --i;
            continue;
        }

        // get block's hash
        crypto::hash blk_hash = core_storage->get_block_id_by_height(i);

        // get block size in kB
        double blk_size = static_cast<double>(core_storage->get_db().get_block_weight(i))/1024.0;

        string blk_size_str = fmt::format("{:0.2f}", blk_size);

        blk_sizes.push_back(blk_size);

        // remove "<" and ">" from the hash string
        string blk_hash_str = pod_to_hex(blk_hash);

        // get block age
        pair<string, string> age = get_age(local_copy_server_timestamp, blk.timestamp);

        context["age_format"] = age.second;

        // start measure time here
        auto start = std::chrono::steady_clock::now();

        // get all transactions in the block found
        // initialize the first list with transaction for solving
        // the block i.e. coinbase.
        vector<cryptonote::transaction> blk_txs {blk.miner_tx};
        vector<crypto::hash> missed_txs;

        if (!core_storage->get_transactions(blk.tx_hashes, blk_txs, missed_txs))
        {
            cerr << "Cant get transactions in block: " << i << endl;
            --i;
            continue;
        }

        uint64_t tx_i {0};

        //          tx_hash     , txd_map
        vector<pair<crypto::hash, mstch::node>> txd_pairs;

        for(auto it = blk_txs.begin(); it != blk_txs.end(); ++it)
        {
            const cryptonote::transaction& tx = *it;

            const tx_details& txd = get_tx_details(tx, false, i, height);

            mstch::map txd_map = txd.get_mstch_map();

            //add age to the txd mstch map
            txd_map.insert({"height"    , i});
            txd_map.insert({"blk_hash"  , blk_hash_str});
            txd_map.insert({"age"       , age.first});
            txd_map.insert({"is_ringct" , (tx.version > 1)});
            txd_map.insert({"rct_type"  , tx.rct_signatures.type});
            txd_map.insert({"blk_size"  , blk_size_str});


            // do not show block info for other than first tx in a block
            if (tx_i > 0)
            {
                txd_map["height"]     = string("");
                txd_map["age"]        = string("");
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

        --i; // go to next block number

    } // while (i <= end_height)

    // calculate median size of the blocks shown
    //double blk_size_median = xmreg::calc_median(blk_sizes.begin(), blk_sizes.end());

    // get current network info from MemoryStatus thread.
    MempoolStatus::network_info current_network_info
        = MempoolStatus::current_network_info;

    // perapre network info mstch::map for the front page
    string hash_rate;

    double hr_d;
    char metric_prefix;

    cryptonote::difficulty_type hr = make_difficulty(
            current_network_info.hash_rate, 
            current_network_info.hash_rate_top64);

    get_metric_prefix(hr, hr_d, metric_prefix);

    if (metric_prefix != 0)
        hash_rate = fmt::format("{:0.3f} {:c}H/s", hr_d, metric_prefix);
    else
        hash_rate = fmt::format("{:s} H/s", hr.str());

    pair<string, string> network_info_age = get_age(local_copy_server_timestamp,
                                                    current_network_info.info_timestamp);

    // if network info is younger than 2 minute, assume its current. No sense
    // showing that it is not current if its less then block time.
    if (local_copy_server_timestamp - current_network_info.info_timestamp < 120)
    {
        current_network_info.current = true;
    }

    context["network_info"] = mstch::map {
            {"difficulty"        , current_network_info.difficulty},
            {"hash_rate"         , hash_rate},
            {"fee_per_kb"        , print_money(current_network_info.fee_per_kb)},
            {"alt_blocks_no"     , current_network_info.alt_blocks_count},
            {"have_alt_block"    , (current_network_info.alt_blocks_count > 0)},
            {"tx_pool_size"      , current_network_info.tx_pool_size},
            {"block_size_limit"  , string {current_network_info.block_size_limit_str}},
            {"block_size_median" , string {current_network_info.block_size_median_str}},
            {"is_current_info"   , current_network_info.current},
            {"is_pool_size_zero" , (current_network_info.tx_pool_size == 0)},
            {"current_hf_version", current_network_info.current_hf_version},
            {"age"               , network_info_age.first},
            {"age_format"        , network_info_age.second},
    };

    // median size of 100 blocks
    context["blk_size_median"] = string {current_network_info.block_size_median_str};

    string mempool_html {"Cant get mempool_pool"};

    // get mempool data for the front page, if ready. If not, then just skip.
    std::future_status mempool_ftr_status = mempool_ftr.wait_for(
            std::chrono::milliseconds(mempool_info_timeout));

    if (mempool_ftr_status == std::future_status::ready)
    {
        mempool_html = mempool_ftr.get();
    }
    else
    {
        cerr  << "mempool future not ready yet, skipping." << endl;
        mempool_html = mstch::render(template_file["mempool_error"], context);
    }

    if (CurrentBlockchainStatus::is_thread_running())
    {
        CurrentBlockchainStatus::Emission current_values
                = CurrentBlockchainStatus::get_emission();

        string emission_blk_no   = std::to_string(current_values.blk_no - 1);
        string emission_coinbase = xmr_amount_to_str(current_values.coinbase, "{:0.3f}");
        string emission_fee      = xmr_amount_to_str(current_values.fee, "{:0.3f}");

        context["emission"] = mstch::map {
                {"blk_no"    , emission_blk_no},
                {"amount"    , emission_coinbase},
                {"fee_amount", emission_fee}
        };
    }
    else
    {
        cerr  << "emission thread not running, skipping." << endl;
    }


    // get memory pool rendered template
    //string mempool_html = mempool(false, no_of_mempool_tx_of_frontpage);

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
    std::vector<MempoolStatus::mempool_tx> mempool_txs;

    if (add_header_and_footer)
    {
        // get all memmpool txs
        mempool_txs = MempoolStatus::get_mempool_txs();
        no_of_mempool_tx = mempool_txs.size();
    }
    else
    {
        // get only first no_of_mempool_tx txs
        mempool_txs = MempoolStatus::get_mempool_txs(no_of_mempool_tx);
        no_of_mempool_tx = std::min<uint64_t>(no_of_mempool_tx, mempool_txs.size());
    }

    // total size of mempool in bytes
    uint64_t mempool_size_bytes = MempoolStatus::mempool_size;

    // reasign this number, in case no of txs in mempool is smaller
    // than what we requested or we want all txs.


    uint64_t total_no_of_mempool_tx = MempoolStatus::mempool_no;

    // initalise page tempate map with basic info about mempool
    mstch::map context {
            {"mempool_size"          , static_cast<uint64_t>(total_no_of_mempool_tx)}, // total no of mempool txs
            {"mempool_refresh_time"  , MempoolStatus::mempool_refresh_time}
    };

    context.emplace("mempooltxs" , mstch::array());

    // get reference to blocks template map to be field below
    mstch::array& txs = boost::get<mstch::array>(context["mempooltxs"]);

    uint64_t local_copy_server_timestamp = server_timestamp;

    // for each transaction in the memory pool
    for (size_t i = 0; i < no_of_mempool_tx; ++i)
    {
        // get transaction info of the tx in the mempool
        const MempoolStatus::mempool_tx& mempool_tx = mempool_txs.at(i);

        // calculate difference between tx in mempool and server timestamps
        array<size_t, 5> delta_time = timestamp_difference(
                local_copy_server_timestamp,
                mempool_tx.receive_time);

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


        // set output page template map
        txs.push_back(mstch::map {
                {"timestamp_no"    , mempool_tx.receive_time},
                {"timestamp"       , mempool_tx.timestamp_str},
                {"age"             , age_str},
                {"hash"            , pod_to_hex(mempool_tx.tx_hash)},
                {"fee"             , mempool_tx.fee_micro_str},
                {"payed_for_kB"    , mempool_tx.payed_for_kB_micro_str},
                {"xmr_inputs"      , mempool_tx.xmr_inputs_str},
                {"xmr_outputs"     , mempool_tx.xmr_outputs_str},
                {"no_inputs"       , mempool_tx.no_inputs},
                {"no_outputs"      , mempool_tx.no_outputs},
                {"no_nonrct_inputs", mempool_tx.num_nonrct_inputs},
                {"mixin"           , mempool_tx.mixin_no},
                {"txsize"          , mempool_tx.txsize}
        });
    }

    context.insert({"mempool_size_kB",
                    fmt::format("{:0.2f}",
                                static_cast<double>(mempool_size_bytes)/1024.0)});

    if (add_header_and_footer)
    {
        // this is when mempool is on its own page, /mempool
        add_css_style(context);

        context["partial_mempool_shown"] = false;

        // render the page
        return mstch::render(template_file["mempool_full"], context);
    }

    // this is for partial disply on front page.

    context["mempool_fits_on_front_page"]    = (total_no_of_mempool_tx <= mempool_txs.size());
    context["no_of_mempool_tx_of_frontpage"] = no_of_mempool_tx;

    context["partial_mempool_shown"] = true;

    // render the page
    return mstch::render(template_file["mempool"], context);
}


string
altblocks()
{

    // initalise page tempate map with basic info about blockchain
    mstch::map context {
            {"testnet"              , testnet},
            {"stagenet"             , stagenet},
            {"blocks"               , mstch::array()}
    };

    uint64_t local_copy_server_timestamp = server_timestamp;

    // get reference to alt blocks template map to be field below
    mstch::array& blocks = boost::get<mstch::array>(context["blocks"]);

    vector<string> atl_blks_hashes;

    if (!rpc.get_alt_blocks(atl_blks_hashes))
    {
        cerr << "rpc.get_alt_blocks(atl_blks_hashes) failed" << endl;
    }

    context.emplace("no_alt_blocks", (uint64_t)atl_blks_hashes.size());

    for (const string& alt_blk_hash: atl_blks_hashes)
    {
        block alt_blk;
        string error_msg;

        int64_t no_of_txs {-1};
        int64_t blk_height {-1};

        // get block age
        pair<string, string> age {"-1", "-1"};


        if (rpc.get_block(alt_blk_hash, alt_blk, error_msg))
        {
            no_of_txs  = alt_blk.tx_hashes.size();

            blk_height = get_block_height(alt_blk);

            age = get_age(local_copy_server_timestamp, alt_blk.timestamp);
        }

        blocks.push_back(mstch::map {
                {"height"   , blk_height},
                {"age"      , age.first},
                {"hash"     , alt_blk_hash},
                {"no_of_txs", no_of_txs}
        });

    }

    add_css_style(context);

    // render the page
    return mstch::render(template_file["altblocks"], context);
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
    string prev_hash_str = pod_to_hex(prev_hash);
    string next_hash_str = pod_to_hex(next_hash);

    // remove "<" and ">" from the hash string
    string blk_hash_str  = pod_to_hex(blk_hash);

    // get block timestamp in user friendly format
    string blk_timestamp = xmreg::timestamp_to_str_gm(blk.timestamp);

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
    uint64_t blk_size = core_storage->get_db().get_block_weight(_blk_height);

    // miner reward tx
    transaction coinbase_tx = blk.miner_tx;

    // transcation in the block
    vector<crypto::hash> tx_hashes = blk.tx_hashes;

    bool have_txs = !blk.tx_hashes.empty();

    // sum of all transactions in the block
    uint64_t sum_fees = 0;

    // get tx details for the coinbase tx, i.e., miners reward
    tx_details txd_coinbase = get_tx_details(blk.miner_tx, true,
                                             _blk_height, current_blockchain_height);

    // initalise page tempate map with basic info about blockchain

    string blk_pow_hash_str = pod_to_hex(get_block_longhash(core_storage, blk, _blk_height, 0));
    cryptonote::difficulty_type blk_difficulty = core_storage->get_db().get_block_difficulty(_blk_height);

    mstch::map context {
            {"testnet"              , testnet},
            {"stagenet"             , stagenet},
            {"blk_hash"             , blk_hash_str},
            {"blk_height"           , _blk_height},
            {"blk_timestamp"        , blk_timestamp},
            {"blk_timestamp_epoch"  , blk.timestamp},
            {"prev_hash"            , prev_hash_str},
            {"next_hash"            , next_hash_str},
            {"enable_as_hex"        , enable_as_hex},
            {"have_next_hash"       , have_next_hash},
            {"have_prev_hash"       , have_prev_hash},
            {"have_txs"             , have_txs},
            {"no_txs"               , std::to_string(
                                         blk.tx_hashes.size())},
            {"blk_age"              , age.first},
            {"delta_time"           , delta_time},
            {"blk_nonce"            , blk.nonce},
            {"blk_pow_hash"         , blk_pow_hash_str},
            {"is_randomx"           , (blk.major_version >= 12
                                            && enable_randomx == true)},
            {"blk_difficulty"       , blk_difficulty.str()},
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
        string tx_hash_str = pod_to_hex(tx_hash);


        // get transaction
        transaction tx;

        if (!mcore->get_tx(tx_hash, tx))
        {
            cerr << "Cant get tx: " << tx_hash << endl;
            continue;
        }

        tx_details txd = get_tx_details(tx, false,
                                        _blk_height,
                                        current_blockchain_height);

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
            = xmreg::xmr_amount_to_str(sum_fees, "{:0.6f}", false);

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
        return fmt::format("Cant get block {:s}", blk_hash);
    }

    return show_block(blk_height);
}

string
show_randomx(uint64_t _blk_height)
{
    if (enable_randomx == false)
    {
        return "RandomX code generation disabled! Use --enable-randomx"
               " flag to enable if.";
    }

    // get block at the given height i
    block blk;

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
    
    string blk_hash_str  = pod_to_hex(blk_hash);


    auto rx_code = get_randomx_code(_blk_height,
                                    blk, blk_hash);

    mstch::array rx_code_str = mstch::array{};
    int code_idx {1};

    for (auto& rxc: rx_code)
    {
        mstch::map rx_map = rxc.get_mstch();
        rx_map["first_program"] = (code_idx == 1);
        rx_map["rx_code_idx"] = code_idx++;
        rx_code_str.push_back(rx_map);
    }

    mstch::map context {
            {"testnet"              , testnet},
            {"stagenet"             , stagenet},
            {"blk_hash"             , blk_hash_str},
            {"blk_height"           , _blk_height},
            {"rx_codes"             , rx_code_str},
    };
    
    add_css_style(context);

    return mstch::render(template_file["randomx"], context);
}

string
show_tx(string tx_hash_str, uint16_t with_ring_signatures = 0, bool refresh_page = false)
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

        vector<MempoolStatus::mempool_tx> found_txs;

        search_mempool(tx_hash, found_txs);

        if (!found_txs.empty())
        {
            // there should be only one tx found
            tx = found_txs.at(0).tx;

            // since its tx in mempool, it has no blk yet
            // so use its recive_time as timestamp to show

            uint64_t tx_recieve_timestamp
                    = found_txs.at(0).receive_time;

            blk_timestamp = xmreg::timestamp_to_str_gm(tx_recieve_timestamp);

            age = get_age(server_timestamp, tx_recieve_timestamp,
                          FULL_AGE_FORMAT);

            // for mempool tx, we dont show more details, e.g., json tx representation
            // so no need for the link
            // show_more_details_link = false;
        }
        else
        {
            // tx is nowhere to be found :-(
            return string("Cant get tx: " + tx_hash_str);
        }
    }

    mstch::map tx_context;


    tx_context = construct_tx_context(tx, static_cast<bool>(with_ring_signatures));

    tx_context["show_more_details_link"] = show_more_details_link;

    if (boost::get<bool>(tx_context["has_error"]))
    {
        return boost::get<string>(tx_context["error_msg"]);
    }

    mstch::map context {
            {"testnet"          , this->testnet},
            {"stagenet"         , this->stagenet},
            {"txs"              , mstch::array{}},
            {"refresh"          , refresh_page},
            {"tx_hash"          , tx_hash_str}
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
show_tx_hex(string tx_hash_str)
{
    transaction tx;
    crypto::hash tx_hash;

    if (!get_tx(tx_hash_str, tx, tx_hash))
        return string {"Cant get tx: "} +  tx_hash_str;

    try
    {
        return tx_to_hex(tx);
    }
    catch (std::exception const& e)
    {
        cerr << e.what() << endl;
        return string {"Failed to obtain hex of tx due to: "} + e.what();
    }
}

string
show_block_hex(size_t block_height, bool complete_blk)
{

    // get transaction
    block blk;

    if (!mcore->get_block_by_height(block_height, blk))
    {
        cerr << "Cant get block in blockchain: " << block_height
             << ". \n Check mempool now\n";
    }

    try
    {
        if (complete_blk == false)
        {
            // get only block data as hex

            return epee::string_tools::buff_to_hex_nodelimer(
                        t_serializable_object_to_blob(blk));
        }
        else
        {
            // get block_complete_entry (block and its txs) as hex

            block_complete_entry complete_block_data;

            if (!mcore->get_block_complete_entry(blk, complete_block_data))
            {
                cerr << "Failed to obtain complete block data " << endl;
                return string {"Failed to obtain complete block data "};
            }

            epee::byte_slice complete_block_data_slice;


            if(!epee::serialization::store_t_to_binary(
                        complete_block_data, complete_block_data_slice))
            {
                cerr << "Failed to serialize complete_block_data\n";
                return string {"Failed to obtain complete block data"};
            }

            std::string block_data_str(
                    complete_block_data_slice.begin(),
                    complete_block_data_slice.end());


            return epee::string_tools
                    ::buff_to_hex_nodelimer(block_data_str);
        }
    }
    catch (std::exception const& e)
    {
        cerr << e.what() << endl;
        return string {"Failed to obtain hex of a block due to: "} + e.what();
    }
}

string
show_ringmembers_hex(string const& tx_hash_str)
{
    transaction tx;
    crypto::hash tx_hash;

    if (!get_tx(tx_hash_str, tx, tx_hash))
        return string {"Cant get tx: "} +  tx_hash_str;

    vector<txin_to_key> input_key_imgs = xmreg::get_key_images(tx);

    // key: vector of absolute_offsets and associated amount (last value),
    // value: vector of output_info_of_mixins
    std::map<vector<uint64_t>, vector<string>> all_mixin_outputs;

       // make timescale maps for mixins in input
    for (txin_to_key const& in_key: input_key_imgs)
    {      
        // get absolute offsets of mixins
        std::vector<uint64_t> absolute_offsets
                = cryptonote::relative_output_offsets_to_absolute(
                        in_key.key_offsets);

        // get public keys of outputs used in the mixins that
        // match to the offests
        std::vector<cryptonote::output_data_t> mixin_outputs;

        try
        {
            // before proceeding with geting the outputs based on
            // the amount and absolute offset
            // check how many outputs there are for that amount
            // go to next input if a too large offset was found
            if (are_absolute_offsets_good(absolute_offsets, in_key)
                    == false)
                continue;

            //core_storage->get_db().get_output_key(in_key.amount,
            //                                      absolute_offsets,
            //                                      mixin_outputs);
            get_output_key<BlockchainDB>(in_key.amount,
                                         absolute_offsets,
                                         mixin_outputs);
        }
        catch (OUTPUT_DNE const& e)
        {
            cerr << "get_output_keys: " << e.what() << endl;
            continue;
        }

        // add accociated amount to these offsets so that we can differentiate
        // between same offsets, but for different amounts
        absolute_offsets.push_back(in_key.amount);

        for (auto const& mo: mixin_outputs)
            all_mixin_outputs[absolute_offsets].emplace_back(pod_to_hex(mo));

    } // for (txin_to_key const& in_key: input_key_imgs)

    if (all_mixin_outputs.empty())
        return string {"No ring members to serialize"};

    // archive all_mixin_outputs vector
    std::ostringstream oss;
    boost::archive::portable_binary_oarchive archive(oss);
    archive << all_mixin_outputs;

    // return as all_mixin_outputs vector hex
    return epee::string_tools
            ::buff_to_hex_nodelimer(oss.str());
}

string
show_ringmemberstx_hex(string const& tx_hash_str)
{
    transaction tx;
    crypto::hash tx_hash;

    if (!get_tx(tx_hash_str, tx, tx_hash))
        return string {"Cant get tx: "} +  tx_hash_str;

    vector<txin_to_key> input_key_imgs = xmreg::get_key_images(tx);

    // key: constracted from concatenation of in_key.amount and absolute_offsets,
    // value: vector of string where string is transaction hash + output index + tx_hex
    // will have to cut this string when de-seraializing this data
    // later in the unit tests
    // transaction hash and output index represent tx_out_index
    std::map<string, vector<string>> all_mixin_txs;

    for (txin_to_key const& in_key: input_key_imgs)
    {
        // get absolute offsets of mixins
        std::vector<uint64_t> absolute_offsets
                = cryptonote::relative_output_offsets_to_absolute(
                        in_key.key_offsets);

        //tx_out_index is pair::<transaction hash, output index>
        vector<tx_out_index> indices;

        // get tx hashes and indices in the txs for the
        // given outputs of mixins
        //  this cant THROW DB_EXCEPTION
        try
        {
            // get tx of the real output
            core_storage->get_db().get_output_tx_and_index(
                        in_key.amount, absolute_offsets, indices);
        }
        catch (exception const& e)
        {

            string out_msg = fmt::format(
                    "Cant get ring member tx_out_index for tx {:s}", tx_hash_str
            );

            cerr << out_msg << endl;

            return string(out_msg);
        }

        string map_key = std::to_string(in_key.amount);

        for (auto const& ao: absolute_offsets)
            map_key += std::to_string(ao);

        // serialize each mixin tx
        for (auto const& txi : indices)
        {
           auto const& mixin_tx_hash = txi.first;
           auto const& output_index_in_tx = txi.second;

           transaction mixin_tx;

           if (!mcore->get_tx(mixin_tx_hash, mixin_tx))
           {
               throw std::runtime_error("Cant get tx: "
                                        + pod_to_hex(mixin_tx_hash));
           }

           // serialize tx
           string tx_hex = epee::string_tools::buff_to_hex_nodelimer(
                                   t_serializable_object_to_blob(mixin_tx));

           all_mixin_txs[map_key].push_back(
                       pod_to_hex(mixin_tx_hash)
                       + std::to_string(output_index_in_tx)
                       + tx_hex);
        }

    } // for (txin_to_key const& in_key: input_key_imgs)

    if (all_mixin_txs.empty())
        return string {"No ring members to serialize"};

    // archive all_mixin_outputs vector
    std::ostringstream oss;
    boost::archive::portable_binary_oarchive archive(oss);
    archive << all_mixin_txs;

    // return as all_mixin_outputs vector hex
    return epee::string_tools
            ::buff_to_hex_nodelimer(oss.str());
}

/**
 * @brief Get ring member tx data
 *
 * Used for generating json file of txs used in unit testing.
 * Thanks to json output from this function, we can mock
 * a number of blockchain quries about key images
 *
 * @param tx_hash_str
 * @return
 */
json
show_ringmemberstx_jsonhex(string const& tx_hash_str)
{
    transaction tx;
    crypto::hash tx_hash;

    if (!get_tx(tx_hash_str, tx, tx_hash))
        return string {"Cant get tx: "} +  tx_hash_str;

    vector<txin_to_key> input_key_imgs = xmreg::get_key_images(tx);

    json tx_json;

    string tx_hex;

    try
    {
        tx_hex = tx_to_hex(tx);
    }
    catch (std::exception const& e)
    {
        cerr << e.what() << endl;
        return json {"error", "Failed to obtain hex of tx"};
    }

    tx_json["hash"] = tx_hash_str;
    tx_json["hex"]  = tx_hex;
    tx_json["nettype"] = static_cast<size_t>(nettype);
    tx_json["is_ringct"] = (tx.version > 1);
    tx_json["rct_type"] = tx.rct_signatures.type;

    tx_json["_comment"] = "Just a placeholder for some comment if needed later";

    // add placeholder for sender and recipient details
    // this is most useful for unit testing on stagenet/testnet
    // private monero networks, so we can easly put these
    // networks accounts details here.
    tx_json["sender"] = json {
                            {"seed", ""},
                            {"address", ""},
                            {"viewkey", ""},
                            {"spendkey", ""},
                            {"amount", 0ull},
                            {"change", 0ull},
                            {"outputs", json::array({json::array(
                                            {"index placeholder",
                                             "public_key placeholder",
                                             "amount placeholder"}
                                        )})
                            },
                            {"_comment", ""}};

    tx_json["recipient"] = json::array();


    tx_json["recipient"].push_back(
                            json { {"seed", ""},
                                {"address", ""},
                                {"is_subaddress", false},
                                {"viewkey", ""},
                                {"spendkey", ""},
                                {"amount", 0ull},
                                {"outputs", json::array({json::array(
                                               {"index placeholder",
                                                "public_key placeholder",
                                                "amount placeholder"}
                                           )})
                                },
                                {"_comment", ""}});


    uint64_t tx_blk_height {0};

    try
    {
        tx_blk_height = core_storage->get_db().get_tx_block_height(tx_hash);
    }
    catch (exception& e)
    {
        cerr << "Cant get block height: " << tx_hash
             << e.what() << endl;

        return json {"error", "Cant get block height"};
    }

    // get block cointaining this tx
    block blk;

    if ( !mcore->get_block_by_height(tx_blk_height, blk))
    {
        cerr << "Cant get block: " << tx_blk_height << endl;
        return json {"error", "Cant get block"};
    }

    block_complete_entry complete_block_data;

    if (!mcore->get_block_complete_entry(blk, complete_block_data))
    {
        cerr << "Failed to obtain complete block data " << endl;
        return json {"error", "Failed to obtain complete block data "};
    }

    epee::byte_slice complete_block_data_slice;

    if(!epee::serialization::store_t_to_binary(
                complete_block_data, complete_block_data_slice))
    {
        cerr << "Failed to serialize complete_block_data\n";
        return json {"error", "Failed to obtain complete block data"};
    }

    tx_details txd = get_tx_details(tx);

    tx_json["payment_id"] = pod_to_hex(txd.payment_id);
    tx_json["payment_id8"] = pod_to_hex(txd.payment_id8);
    tx_json["payment_id8e"] = pod_to_hex(txd.payment_id8);

    std::string complete_block_data_str(
            complete_block_data_slice.begin(),
            complete_block_data_slice.end());

    tx_json["block"] = epee::string_tools
             ::buff_to_hex_nodelimer(complete_block_data_str);

    tx_json["block_version"] = json {blk.major_version, blk.minor_version};

    tx_json["inputs"] = json::array();


    // key: constracted from concatenation of in_key.amount and absolute_offsets,
    // value: vector of string where string is transaction hash + output index + tx_hex
    // will have to cut this string when de-seraializing this data
    // later in the unit tests
    // transaction hash and output index represent tx_out_index
    std::map<string, vector<string>> all_mixin_txs;

    for (txin_to_key const& in_key: input_key_imgs)
    {
        // get absolute offsets of mixins
        std::vector<uint64_t> absolute_offsets
                = cryptonote::relative_output_offsets_to_absolute(
                        in_key.key_offsets);

        //tx_out_index is pair::<transaction hash, output index>
        vector<tx_out_index> indices;
        std::vector<output_data_t> mixin_outputs;

        // get tx hashes and indices in the txs for the
        // given outputs of mixins
        //  this cant THROW DB_EXCEPTION
        try
        {
            // get tx of the real output
            core_storage->get_db().get_output_tx_and_index(
                        in_key.amount, absolute_offsets, indices);

            // get mining ouput info
            //core_storage->get_db().get_output_key(
                        //in_key.amount,
                        //absolute_offsets,
                        //mixin_outputs);

            get_output_key<BlockchainDB>(in_key.amount,
                                           absolute_offsets,
                                           mixin_outputs);
        }
        catch (exception const& e)
        {

            string out_msg = fmt::format(
                    "Cant get ring member tx_out_index for tx {:s}", tx_hash_str
            );

            cerr << out_msg << endl;

            return json {"error", out_msg};
        }


        tx_json["inputs"].push_back(json {{"key_image", pod_to_hex(in_key.k_image)},
                                          {"amount", in_key.amount},
                                          {"absolute_offsets", absolute_offsets},
                                          {"ring_members", json::array()}});

        json& ring_members = tx_json["inputs"].back()["ring_members"];


        if (indices.size() != mixin_outputs.size())
        {
            cerr << "indices.size() != mixin_outputs.size()\n";
            return json {"error", "indices.size() != mixin_outputs.size()"};
        }

        // serialize each mixin tx
        //for (auto const& txi : indices)
        for (size_t i = 0; i < indices.size(); ++i)
        {

           tx_out_index const& txi = indices[i];
           output_data_t const& mo = mixin_outputs[i];

           auto const& mixin_tx_hash = txi.first;
           auto const& output_index_in_tx = txi.second;

           transaction mixin_tx;

           if (!mcore->get_tx(mixin_tx_hash, mixin_tx))
           {
               throw std::runtime_error("Cant get tx: "
                                        + pod_to_hex(mixin_tx_hash));
           }

           // serialize tx
           string tx_hex = epee::string_tools::buff_to_hex_nodelimer(
                                   t_serializable_object_to_blob(mixin_tx));

           ring_members.push_back(
                   json {
                          {"ouput_pk", pod_to_hex(mo.pubkey)},
                          {"tx_hash", pod_to_hex(mixin_tx_hash)},
                          {"output_index_in_tx", txi.second},
                          {"tx_hex", tx_hex},
                   });

        }

    } // for (txin_to_key const& in_key: input_key_imgs)


    // archive all_mixin_outputs vector
    std::ostringstream oss;
    boost::archive::portable_binary_oarchive archive(oss);
    archive << all_mixin_txs;

    // return as all_mixin_outputs vector hex
    //return epee::string_tools
    //        ::buff_to_hex_nodelimer(oss.str());

    return tx_json;
}

string
show_my_outputs(string tx_hash_str,
                string xmr_address_str,
                string viewkey_str, /* or tx_prv_key_str when tx_prove == true */
                string raw_tx_data,
                string domain,
                bool tx_prove = false)
{

    // remove white characters
    boost::trim(tx_hash_str);
    boost::trim(xmr_address_str);
    boost::trim(viewkey_str);
    boost::trim(raw_tx_data);

    (void) domain; // not used

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
    cryptonote::address_parse_info address_info;

    if (!xmreg::parse_str_address(xmr_address_str,  address_info, nettype))
    {
        cerr << "Cant parse string address: " << xmr_address_str << endl;
        return string("Cant parse xmr address: " + xmr_address_str);
    }

    // parse string representing given private key
    crypto::secret_key prv_view_key;

    std::vector<crypto::secret_key> multiple_tx_secret_keys;

    if (!xmreg::parse_str_secret_key(viewkey_str, multiple_tx_secret_keys))
    {
        cerr << "Cant parse the private key: " << viewkey_str << endl;
        return string("Cant parse private key: " + viewkey_str);
    }
    if (multiple_tx_secret_keys.size() == 1)
    {
        prv_view_key = multiple_tx_secret_keys[0];
    }
    else if (!tx_prove)
    {
        cerr << "Concatenated secret keys are only for tx proving!" << endl;
        return string("Concatenated secret keys are only for tx proving!");
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

        vector<MempoolStatus::mempool_tx> found_txs;

        search_mempool(tx_hash, found_txs);

        if (!found_txs.empty())
        {
            // there should be only one tx found
            tx = found_txs.at(0).tx;

            // since its tx in mempool, it has no blk yet
            // so use its recive_time as timestamp to show

            uint64_t tx_recieve_timestamp
                    = found_txs.at(0).receive_time;

            blk_timestamp = xmreg::timestamp_to_str_gm(tx_recieve_timestamp);

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

        blk_timestamp = xmreg::timestamp_to_str_gm(blk.timestamp);

        tx_blk_height_str = std::to_string(tx_blk_height);
    }

    // payments id. both normal and encrypted (payment_id8)
    string pid_str   = pod_to_hex(txd.payment_id);
    string pid8_str  = pod_to_hex(txd.payment_id8);

    string shortcut_url = tx_prove 
                    ? string("/prove") : string("/myoutputs")
                          + '/' + tx_hash_str
                          + '/' + xmr_address_str
                          + '/' + viewkey_str;


    string viewkey_str_partial = viewkey_str;

    // dont show full private keys. Only file first and last letters
    for (size_t i = 3; i < viewkey_str_partial.length() - 2; ++i)
        viewkey_str_partial[i] = '*';

    // initalise page tempate map with basic info about blockchain
    mstch::map context {
            {"testnet"              , testnet},
            {"stagenet"             , stagenet},
            {"tx_hash"              , tx_hash_str},
            {"tx_prefix_hash"       , pod_to_hex(txd.prefix_hash)},
            {"xmr_address"          , xmr_address_str},
            {"viewkey"              , viewkey_str_partial},
            {"tx_pub_key"           , pod_to_hex(txd.pk)},
            {"blk_height"           , tx_blk_height_str},
            {"tx_size"              , fmt::format("{:0.4f}",
                                                  static_cast<double>(txd.size) / 1024.0)},
            {"tx_fee"               , xmreg::xmr_amount_to_str(txd.fee, "{:0.12f}", true)},
            {"blk_timestamp"        , blk_timestamp},
            {"delta_time"           , age.first},
            {"outputs_no"           , static_cast<uint64_t>(txd.output_pub_keys.size())},
            {"has_payment_id"       , txd.payment_id  != null_hash},
            {"has_payment_id8"      , txd.payment_id8 != null_hash8},
            {"payment_id"           , pid_str},
            {"payment_id8"          , pid8_str},
            {"decrypted_payment_id8", string{}},
            {"tx_prove"             , tx_prove},
            {"shortcut_url"         , shortcut_url}
    };

    string server_time_str = xmreg::timestamp_to_str_gm(server_timestamp, "%F");



    // public transaction key is combined with our viewkey
    // to create, so called, derived key.
    key_derivation derivation;
    std::vector<key_derivation> additional_derivations(txd.additional_pks.size());   

    if (tx_prove && multiple_tx_secret_keys.size()
            != txd.additional_pks.size() + 1)
    {
        return string("This transaction includes additional tx pubkeys whose "
                       "size doesn't match with the provided tx secret keys");
    }

    public_key pub_key = tx_prove ? address_info.address.m_view_public_key : txd.pk;

    //cout << "txd.pk: " << pod_to_hex(txd.pk) << endl;

    if (!generate_key_derivation(pub_key,
                                 tx_prove ? multiple_tx_secret_keys[0] : prv_view_key,
                                 derivation))
    {
        cerr << "Cant get derived key for: "  << "\n"
             << "pub_tx_key: " << pub_key << " and "
             << "prv_view_key" << prv_view_key << endl;

        return string("Cant get key_derivation");
    }

    for (size_t i = 0; i < txd.additional_pks.size(); ++i)
    {
        if (!generate_key_derivation(tx_prove ? pub_key : txd.additional_pks[i],
                                     tx_prove ? multiple_tx_secret_keys[i + 1] : prv_view_key,
                                     additional_derivations[i]))
        {
            cerr << "Cant get derived key for: "  << "\n"
                 << "pub_tx_key: " << txd.additional_pks[i] << " and "
                 << "prv_view_key" << prv_view_key << endl;

            return string("Cant get key_derivation");
        }
    }

    // decrypt encrypted payment id, as used in integreated addresses
    crypto::hash8 decrypted_payment_id8 = txd.payment_id8;

    if (decrypted_payment_id8 != null_hash8)
    {
        if (mcore->get_device()->decrypt_payment_id(
                    decrypted_payment_id8, pub_key, prv_view_key))
        {
            context["decrypted_payment_id8"] = pod_to_hex(decrypted_payment_id8);
        }
    }

    mstch::array outputs;

    uint64_t sum_xmr {0};

    std::vector<uint64_t> money_transfered(tx.vout.size(), 0);

    //std::deque<rct::key> mask(tx.vout.size());

    uint64_t output_idx {0};

    for (output_tuple_with_tag& outp: txd.output_pub_keys)
    {

        // get the tx output public key
        // that normally would be generated for us,
        // if someone had sent us some xmr.
        public_key tx_pubkey;

        derive_public_key(derivation,
                          output_idx,
                          address_info.address.m_spend_public_key,
                          tx_pubkey);

//        cout << pod_to_hex(derivation) << ", " << output_idx << ", "
//             << pod_to_hex(address_info.address.m_spend_public_key) << ", "
//             << pod_to_hex(outp.first) << " == "
//             << pod_to_hex(tx_pubkey) << '\n'  << '\n';


        // check if generated public key matches the current output's key
        bool mine_output = (std::get<0>(outp) == tx_pubkey);



        bool with_additional = false;

        if (!mine_output && txd.additional_pks.size()
                == txd.output_pub_keys.size())
        {
            derive_public_key(additional_derivations[output_idx],
                              output_idx,
                              address_info.address.m_spend_public_key,
                              tx_pubkey);


            mine_output = (std::get<0>(outp) == tx_pubkey);

            with_additional = true;
        }

        uint64_t xmr_amount = std::get<1>(outp);

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

                auto derivation_to_use = with_additional
                        ? additional_derivations[output_idx] : derivation;

                r = decode_ringct(
                            tx.rct_signatures,
                            derivation_to_use,
                            output_idx,
                            tx.rct_signatures.ecdhInfo[output_idx].mask,
                            rct_amount);

                if (!r)
                {
                    cerr << "\nshow_my_outputs: Cant decode RingCT!\n";
                }

                // decode view key
//                crypto::view_tag derived_view_tag;
//                crypto::derive_view_tag(derivation_to_use,
//                                        output_idx, derived_view_tag);

//                cout << derived_view_tag << endl;

                xmr_amount = rct_amount;
                money_transfered[output_idx] = rct_amount;
            }

        }

        if (mine_output)
        {
            sum_xmr += xmr_amount;
        }

        outputs.push_back(mstch::map {
                {"out_pub_key"           , pod_to_hex(std::get<0>(outp))},
                {"amount"                , xmreg::xmr_amount_to_str(xmr_amount)},
                {"mine_output"           , mine_output},
                {"output_idx"            , fmt::format("{:02d}", output_idx)}
        });

        ++output_idx;
    }


    context.emplace("outputs", outputs);

    context["found_our_outputs"] = (sum_xmr > 0);
    context["sum_xmr"]           = xmreg::xmr_amount_to_str(sum_xmr);

    // we can also test ouputs used in mixins for key images
    // this can show possible spending. Only possible, because
    // without a spend key, we cant know for sure. It might be
    // that our output was used by someone else for their mixins.


    if (enable_mixin_guess) {

        bool show_key_images {false};

        mstch::array inputs;

        vector<txin_to_key> input_key_imgs = xmreg::get_key_images(tx);

        // to hold sum of xmr in matched mixins, those that
        // perfectly match mixin public key with outputs in mixn_tx.
        uint64_t sum_mixin_xmr {0};

        // this is used for the final check. we assument that number of
        // parefct matches must be equal to number of inputs in a tx.
        uint64_t no_of_matched_mixins {0};

        // Hold all possible mixins that we found. This is only used so that
        // we get number of all posibilities, and their total xmr amount
        // (useful for unit testing)
        //                     public_key    , amount
        std::vector<std::pair<crypto::public_key, uint64_t>> all_possible_mixins;

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
                // before proceeding with geting the outputs based on
                // the amount and absolute offset
                // check how many outputs there are for that amount
                // go to next input if a too large offset was found
                if (are_absolute_offsets_good(absolute_offsets, in_key) == false)
                    continue;

                //core_storage->get_db().get_output_key(in_key.amount,
                                                      //absolute_offsets,
                                                      //mixin_outputs);

                get_output_key<BlockchainDB>(in_key.amount,
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
                            in_key.amount, abs_offset);

                    cerr << out_msg << '\n';

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
                        make_pair<string, mstch::array>("mixin_outputs"
                                                        , mstch::array{}),
                        {"has_mixin_outputs"  , false}});

                mstch::array& mixin_outputs = boost::get<mstch::array>(
                        boost::get<mstch::map>(mixins.back())["mixin_outputs"]);

                mstch::node& has_mixin_outputs
                        = boost::get<mstch::map>(mixins.back())["has_mixin_outputs"];

                bool found_something {false};

                public_key mixin_tx_pub_key
                        = xmreg::get_tx_pub_key_from_received_outs(mixin_tx);

                std::vector<public_key> mixin_additional_tx_pub_keys
                        = cryptonote::get_additional_tx_pub_keys_from_extra(mixin_tx);

                string mixin_tx_pub_key_str = pod_to_hex(mixin_tx_pub_key);

                // public transaction key is combined with our viewkey
                // to create, so called, derived key.
                key_derivation derivation;

                std::vector<key_derivation> additional_derivations(
                            mixin_additional_tx_pub_keys.size());

                if (!generate_key_derivation(mixin_tx_pub_key,
                                             prv_view_key, derivation))
                {
                    cerr << "Cant get derived key for: "  << "\n"
                         << "pub_tx_key: " << mixin_tx_pub_key << " and "
                         << "prv_view_key" << prv_view_key << endl;

                    continue;
                }
                for (size_t i = 0; i < mixin_additional_tx_pub_keys.size(); ++i)
                {
                    if (!generate_key_derivation(mixin_additional_tx_pub_keys[i],
                                                 prv_view_key,
                                                 additional_derivations[i]))
                    {
                        cerr << "Cant get derived key for: "  << "\n"
                             << "pub_tx_key: " << mixin_additional_tx_pub_keys[i]
                             << " and prv_view_key" << prv_view_key << endl;

                        continue;
                    }
                }

                //          <public_key  , amount  , out idx>
                vector<tuple<public_key, uint64_t, uint64_t>> output_pub_keys;

                output_pub_keys = xmreg::get_ouputs_tuple(mixin_tx);

                mixin_outputs.push_back(mstch::map{
                        {"mix_tx_hash"      , mixin_tx_hash_str},
                        {"mix_tx_pub_key"   , mixin_tx_pub_key_str},
                        make_pair<string, mstch::array>("found_outputs"
                                                        , mstch::array{}),
                        {"has_found_outputs", false}
                });

                mstch::array& found_outputs = boost::get<mstch::array>(
                        boost::get<mstch::map>(
                                mixin_outputs.back())["found_outputs"]
                );

                mstch::node& has_found_outputs
                        = boost::get<mstch::map>(
                            mixin_outputs.back())["has_found_outputs"];

                uint64_t ringct_amount {0};

                // for each output in mixin tx, find the one from key_image
                // and check if its ours.
                for (const auto& mix_out: output_pub_keys)
                {

                    public_key const& output_pub_key = std::get<0>(mix_out);
                    uint64_t amount           = std::get<1>(mix_out);
                    uint64_t output_idx_in_tx = std::get<2>(mix_out);

                    //cout << " - " << pod_to_hex(output_pub_key) << endl;

    //                        // analyze only those output keys
    //                        // that were used in mixins
    //                        if (output_pub_key != output_data.pubkey)
    //                        {
    //                            continue;
    //                        }

                    // get the tx output public key
                    // that normally would be generated for us,
                    // if someone had sent us some xmr.
                    public_key tx_pubkey_generated;

                    derive_public_key(derivation,
                                      output_idx_in_tx,
                                      address_info.address.m_spend_public_key,
                                      tx_pubkey_generated);

                    // check if generated public key matches the current output's key
                    bool mine_output = (output_pub_key == tx_pubkey_generated);

                    bool with_additional = false;

                    if (!mine_output && mixin_additional_tx_pub_keys.size()
                            == output_pub_keys.size())
                    {
                        derive_public_key(additional_derivations[output_idx_in_tx],
                                          output_idx_in_tx,
                                          address_info.address.m_spend_public_key,
                                          tx_pubkey_generated);

                        mine_output = (output_pub_key == tx_pubkey_generated);

                        with_additional = true;
                    }


                    if (mine_output && mixin_tx.version == 2)
                    {
                        // cointbase txs have amounts in plain sight.
                        // so use amount from ringct, only for non-coinbase txs
                        if (!is_coinbase(mixin_tx))
                        {
                            // initialize with regular amount
                            uint64_t rct_amount = amount;

                            bool r;

                            auto derivation_to_use = with_additional
                                    ? additional_derivations[output_idx] : derivation;

                            r = decode_ringct(
                                        mixin_tx.rct_signatures,
                                        derivation_to_use,
                                        output_idx_in_tx,
                                        mixin_tx.rct_signatures.ecdhInfo[output_idx_in_tx].mask,
                                        rct_amount);

                            if (!r)
                                cerr << "show_my_outputs: key images: "
                                        "Cant decode RingCT!\n";

                            amount = rct_amount;

                        } // if (mine_output && mixin_tx.version == 2)
                    }

                    // makre only
                    bool output_match = (output_pub_key == output_data.pubkey);

                    // mark only first output_match as the "real" one
                    // due to luck of better method of gussing which output
                    // is real if two are found in a single input.
                    output_match = output_match && no_of_output_matches_found == 0;

                    // save our mixnin's public keys
                    found_outputs.push_back(mstch::map {
                            {"my_public_key"   , pod_to_hex(output_pub_key)},
                            {"tx_hash"         , tx_hash_str},
                            {"mine_output"     , mine_output},
                            {"out_idx"         , output_idx_in_tx},
                            {"formed_output_pk", out_pub_key_str},
                            {"out_in_match"    , output_match},
                            {"amount"          , xmreg::xmr_amount_to_str(amount)}
                    });

                    //cout << "output_pub_key == output_data.pubkey" << endl;

                    if (mine_output)
                    {
                        found_something = true;
                        show_key_images = true;

                        // increase sum_mixin_xmr only when
                        // public key of an outputs used in ring signature,
                        // matches a public key in a mixin_tx
                        if (output_pub_key != output_data.pubkey)
                            continue;

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
                                ringct_amount += amount;
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

                    } // if (mine_output)

                } // for (const pair<public_key, uint64_t>& mix_out: txd.output_pub_keys)

                has_found_outputs = !found_outputs.empty();

                has_mixin_outputs = found_something;

                //   all_possible_mixins_amount += amount;

                if (found_something)
                    all_possible_mixins.push_back(
                        {mixin_tx_pub_key,
                         in_key.amount == 0 ? ringct_amount : in_key.amount});

                ++count;

            } // for (const cryptonote::output_data_t& output_data: mixin_outputs)

        } //  for (const txin_to_key& in_key: input_key_imgs)






        context.emplace("inputs", inputs);

        context["show_inputs"]   = show_key_images;
        context["inputs_no"]     = static_cast<uint64_t>(inputs.size());
        context["sum_mixin_xmr"] = xmreg::xmr_amount_to_str(
                sum_mixin_xmr, "{:0.12f}", false);


        uint64_t possible_spending  {0};

        //cout << "\nall_possible_mixins: " << all_possible_mixins.size() << '\n';

        // useful for unit testing as it provides total xmr sum
        // of possible mixins
        uint64_t all_possible_mixins_amount1  {0};

        for (auto& p: all_possible_mixins)
            all_possible_mixins_amount1 += p.second;

        //cout << "\all_possible_mixins_amount: " << all_possible_mixins_amount1 << '\n';

        //cout << "\nmixins: " << mix << '\n';

        context["no_all_possible_mixins"] = static_cast<uint64_t>(all_possible_mixins.size());
        context["all_possible_mixins_amount"] = all_possible_mixins_amount1;


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

    } // if (enable_mixin_guess)

    add_css_style(context);

    // render the page
    return mstch::render(template_file["my_outputs"], context);
}

string
show_prove(string tx_hash_str,
           string xmr_address_str,
           string tx_prv_key_str,
           string const& raw_tx_data,
           string domain)
{

    return show_my_outputs(tx_hash_str, xmr_address_str,
                           tx_prv_key_str, raw_tx_data,
                           domain, true);
}

string
show_rawtx()
{

    // initalise page tempate map with basic info about blockchain
    mstch::map context {
            {"testnet"              , testnet},
            {"stagenet"             , stagenet}
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
            {"stagenet"             , stagenet},
            {"unsigned_tx_given"    , unsigned_tx_given},
            {"have_raw_tx"          , true},
            {"has_error"            , false},
            {"error_msg"            , string {}},
            {"data_prefix"          , data_prefix},
    };

    context.emplace("txs", mstch::array{});

    string full_page = template_file["checkrawtx"];

    add_css_style(context);


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
                            {"dest_address"  , get_account_address_as_str(
                                    nettype, a_dest.is_subaddress, a_dest.addr)},
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

                    public_key real_out_pub_key = std::get<0>(real_txd.output_pub_keys[tx_source.real_output_in_tx_index]);

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

                        public_key out_pub_key = std::get<0>(txd.output_pub_keys[toi.second]);


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
                tx_cd_data["min_mix_time"]     = xmreg::timestamp_to_str_gm(min_mix_timestamp);
                tx_cd_data["max_mix_time"]     = xmreg::timestamp_to_str_gm(max_mix_timestamp);
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

                cerr << msg << endl;

                return string(msg);
            }

            crypto::hash tx_hash_from_blob;
            crypto::hash tx_prefix_hash_from_blob;
            cryptonote::transaction tx_from_blob;

//                std::stringstream ss;
//                ss << tx_data_blob;
//                binary_archive<false> ba(ss);
//                serialization::serialize(ba, tx_from_blob);

            if (!cryptonote::parse_and_validate_tx_from_blob(tx_data_blob,
                                                             tx_from_blob,
                                                             tx_hash_from_blob,
                                                             tx_prefix_hash_from_blob))
            {
                string error_msg = fmt::format("failed to validate transaction");

                context["has_error"] = true;
                context["error_msg"] = error_msg;

                return mstch::render(full_page, context);
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
            mstch::map tx_context = construct_tx_context(ptx.tx, 1);

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
                                {"dest_address"   , get_account_address_as_str(
                                        nettype, a_dest.is_subaddress, a_dest.addr)},
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
                                {"dest_address"   , get_account_address_as_str(
                                        nettype, ptx.construction_data.change_dts.is_subaddress, ptx.construction_data.change_dts.addr)},
                                {"dest_amount"    ,
                                        xmreg::xmr_amount_to_str(ptx.construction_data.change_dts.amount)},
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

                uint64_t index_of_real_output = std::get<0>(tx_source.outputs[tx_source.real_output]);

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
                        = std::get<0>(real_txd.output_pub_keys[tx_source.real_output_in_tx_index]);

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

    // render the page
    return mstch::render(full_page, context, partials);
}

string
show_pushrawtx(string raw_tx_data, string action)
{
    clean_post_data(raw_tx_data);

    // initalize page template context map
    mstch::map context {
            {"testnet"              , testnet},
            {"stagenet"             , stagenet},
            {"have_raw_tx"          , true},
            {"has_error"            , false},
            {"error_msg"            , string {}},
    };

    // add header and footer
    string full_page = template_file["pushrawtx"];

    add_css_style(context);

    std::vector<tools::wallet2::pending_tx> ptx_vector;

    // first try reading raw_tx_data as a raw hex string
    std::string tx_blob;
    cryptonote::transaction parsed_tx;
    crypto::hash parsed_tx_hash, parsed_tx_prefixt_hash;
    if (epee::string_tools::parse_hexstr_to_binbuff(raw_tx_data, tx_blob) && parse_and_validate_tx_from_blob(tx_blob, parsed_tx, parsed_tx_hash, parsed_tx_prefixt_hash))
    {
        ptx_vector.push_back({});
        ptx_vector.back().tx = parsed_tx;
    }
    // if failed, treat raw_tx_data as base64 encoding of signed_monero_tx
    else
    {
        string decoded_raw_tx_data = epee::string_encoding::base64_decode(raw_tx_data);

        const size_t magiclen = strlen(SIGNED_TX_PREFIX);

        string data_prefix = xmreg::make_printable(decoded_raw_tx_data.substr(0, magiclen));

        context["data_prefix"] = data_prefix;

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

        ptx_vector = signed_txs.ptx;
    }

    context.emplace("txs", mstch::array{});

    mstch::array& txs = boost::get<mstch::array>(context["txs"]);

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
        vector<MempoolStatus::mempool_tx> found_mempool_txs;

        search_mempool(txd.hash, found_mempool_txs);

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
            {"stagenet"           , stagenet},
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
            {"stagenet"           , stagenet}
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
            {"stagenet"        , stagenet},
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

    if (strncmp(decoded_raw_data.c_str(), KEY_IMAGE_EXPORT_FILE_MAGIC, magiclen) != 0)
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
    const size_t chacha_length = sizeof(crypto::chacha_key);

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

    address_parse_info address_info {*xmr_address, false};


    context.insert({"address"        , REMOVE_HASH_BRAKETS(
            xmreg::print_address(address_info, nettype))});
    context.insert({"viewkey"        , REMOVE_HASH_BRAKETS(
            fmt::format("{:s}", prv_view_key))});
    context.insert({"has_total_xmr"  , false});
    context.insert({"total_xmr"      , string{}});
    context.insert({"key_imgs"       , mstch::array{}});


    size_t no_key_images = (decoded_raw_data.size() - header_lenght) / record_lenght;

    //vector<pair<crypto::key_image, crypto::signature>> signed_key_images;

    mstch::array& key_imgs_ctx = boost::get<mstch::array>(context["key_imgs"]);

    for (size_t n = 0; n < no_key_images; ++n)
    {
        const char* record_ptr = decoded_raw_data.data() + header_lenght + n * record_lenght;

        crypto::key_image key_image
                = *reinterpret_cast<const crypto::key_image*>(record_ptr);

        crypto::signature signature
                = *reinterpret_cast<const crypto::signature*>(record_ptr + key_img_size);

        mstch::map key_img_info {
                {"key_no"              , fmt::format("{:03d}", n)},
                {"key_image"           , pod_to_hex(key_image)},
                {"signature"           , fmt::format("{:s}", signature)},
                {"address"             , xmreg::print_address(
                                            address_info, nettype)},
                {"is_spent"            , core_storage->have_tx_keyimg_as_spent(key_image)},
                {"tx_hash"             , string{}}
        };


        key_imgs_ctx.push_back(key_img_info);

    } // for (size_t n = 0; n < no_key_images; ++n)

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
            {"stagenet"        , stagenet},
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

    if (strncmp(decoded_raw_data.c_str(), OUTPUT_EXPORT_FILE_MAGIC, magiclen) != 0)
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

    address_parse_info address_info {*xmr_address, false, false, crypto::null_hash8};

    context.insert({"address"        , REMOVE_HASH_BRAKETS(
            xmreg::print_address(address_info, nettype))});
    context.insert({"viewkey"        , pod_to_hex(prv_view_key)});
    context.insert({"has_total_xmr"  , false});
    context.insert({"total_xmr"      , string{}});
    context.insert({"output_keys"    , mstch::array{}});

    mstch::array& output_keys_ctx = boost::get<mstch::array>(context["output_keys"]);


    std::vector<tools::wallet2::transfer_details> outputs;

    try
    {
        std::string body(decoded_raw_data, header_lenght);
        std::stringstream iss;
        iss << body;
        boost::archive::portable_binary_iarchive ar(iss);
        //boost::archive::binary_iarchive ar(iss);

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

        public_key output_pub_key = td.get_public_key();

        uint64_t xmr_amount = td.amount();

        // if the output is RingCT, i.e., tx version is 2
        // need to decode its amount
        if (td.is_rct())
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
            std::vector<public_key> additional_tx_pub_keys = cryptonote::get_additional_tx_pub_keys_from_extra(tx);

            // cointbase txs have amounts in plain sight.
            // so use amount from ringct, only for non-coinbase txs
            if (!is_coinbase(tx))
            {

                bool r = decode_ringct(tx.rct_signatures,
                                       tx_pub_key,
                                       prv_view_key,
                                       td.m_internal_output_index,
                                       tx.rct_signatures.ecdhInfo[td.m_internal_output_index].mask,
                                       xmr_amount);
                r = r || decode_ringct(tx.rct_signatures,
                                       additional_tx_pub_keys[td.m_internal_output_index],
                                       prv_view_key,
                                       td.m_internal_output_index,
                                       tx.rct_signatures.ecdhInfo[td.m_internal_output_index].mask,
                                       xmr_amount);

                if (!r)
                {
                    string error_msg = fmt::format(
                            "Cant decode RingCT for output: {:s}",
                            output_pub_key);

                    context["has_error"] = true;
                    context["error_msg"] = error_msg;

                    return mstch::render(full_page, context);
                }

            } //  if (!is_coinbase(tx))

        } // if (td.is_rct())

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
                {"output_pub_key"      , REMOVE_HASH_BRAKETS(fmt::format("{:s}", output_pub_key))},
                {"amount"              , xmreg::xmr_amount_to_str(xmr_amount)},
                {"tx_hash"             , REMOVE_HASH_BRAKETS(fmt::format("{:s}", td.m_txid))},
                {"timestamp"           , xmreg::timestamp_to_str_gm(blk_timestamp)},
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

    uint64_t search_str_length = search_text.length();

    // first let try searching for tx
    result_html = show_tx(search_text);

    // nasty check if output is "Cant get" as a sign of
    // a not found tx. Later need to think of something better.
    if (result_html.find("Cant get") == string::npos)
    {
        return result_html;
    }


    // first check if searching for block of given height
    if (search_text.size() < 12)
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

    // if tx search not successful, check if we are looking
    // for a block with given hash
    result_html = show_block(search_text);

    if (result_html.find("Cant get") == string::npos)
    {
        return result_html;
    }

    result_html = default_txt;


    // check if monero address is given based on its length
    // if yes, then we can only show its public components
    if (search_str_length == 95)
    {
        // parse string representing given monero address
        address_parse_info address_info;

        cryptonote::network_type nettype_addr {cryptonote::network_type::MAINNET};

        if (search_text[0] == '9' || search_text[0] == 'A' || search_text[0] == 'B')
            nettype_addr = cryptonote::network_type::TESTNET;
        if (search_text[0] == '5' || search_text[0] == '7')
            nettype_addr = cryptonote::network_type::STAGENET;

        if (!xmreg::parse_str_address(search_text, address_info, nettype_addr))
        {
            cerr << "Cant parse string address: " << search_text << endl;
            return string("Cant parse address (probably incorrect format): ")
                   + search_text;
        }

        return show_address_details(address_info, nettype_addr);
    }

    // check if integrated monero address is given based on its length
    // if yes, then show its public components search tx based on encrypted id
    if (search_str_length == 106)
    {

        cryptonote::account_public_address address;

        address_parse_info address_info;

        if (!get_account_address_from_str(address_info, nettype, search_text))
        {
            cerr << "Cant parse string integerated address: " << search_text << endl;
            return string("Cant parse address (probably incorrect format): ")
                   + search_text;
        }

        return show_integrated_address_details(address_info,
                                               address_info.payment_id,
                                               nettype);
    }

    // all_possible_tx_hashes was field using custom lmdb database
    // it was dropped, so all_possible_tx_hashes will be alwasy empty
    // for now
    vector<pair<string, vector<string>>> all_possible_tx_hashes;

    result_html = show_search_results(search_text, all_possible_tx_hashes);

    return result_html;
}

string
show_address_details(const address_parse_info& address_info, cryptonote::network_type nettype = cryptonote::network_type::MAINNET)
{

    string address_str      = xmreg::print_address(address_info, nettype);
    string pub_viewkey_str  = fmt::format("{:s}", address_info.address.m_view_public_key);
    string pub_spendkey_str = fmt::format("{:s}", address_info.address.m_spend_public_key);

    mstch::map context {
            {"xmr_address"        , REMOVE_HASH_BRAKETS(address_str)},
            {"public_viewkey"     , REMOVE_HASH_BRAKETS(pub_viewkey_str)},
            {"public_spendkey"    , REMOVE_HASH_BRAKETS(pub_spendkey_str)},
            {"is_integrated_addr" , false},
            {"testnet"            , testnet},
            {"stagenet"           , stagenet},
    };

    add_css_style(context);

    // render the page
    return mstch::render(template_file["address"], context);
}

// ;
string
show_integrated_address_details(const address_parse_info& address_info,
                                const crypto::hash8& encrypted_payment_id,
                                cryptonote::network_type nettype = cryptonote::network_type::MAINNET)
{

    string address_str        = xmreg::print_address(address_info, nettype);
    string pub_viewkey_str    = fmt::format("{:s}", address_info.address.m_view_public_key);
    string pub_spendkey_str   = fmt::format("{:s}", address_info.address.m_spend_public_key);
    string enc_payment_id_str = fmt::format("{:s}", encrypted_payment_id);

    mstch::map context {
            {"xmr_address"          , REMOVE_HASH_BRAKETS(address_str)},
            {"public_viewkey"       , REMOVE_HASH_BRAKETS(pub_viewkey_str)},
            {"public_spendkey"      , REMOVE_HASH_BRAKETS(pub_spendkey_str)},
            {"encrypted_payment_id" , REMOVE_HASH_BRAKETS(enc_payment_id_str)},
            {"is_integrated_addr"   , true},
            {"testnet"              , testnet},
            {"stagenet"             , stagenet},
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

        vector<output_tuple_with_tag>::iterator it2 =
                find_if(begin(txd.output_pub_keys), end(txd.output_pub_keys),
                        [&](const output_tuple_with_tag& tx_out_pk)
                        {
                            return pod_to_hex(std::get<0>(tx_out_pk)) == search_text;
                        });

        if (it2 != txd.output_pub_keys.end())
        {
            tx_hashes["output_public_keys"].push_back(tx_hash_str);
        }

    }

    return  tx_hashes;

}

string
show_search_results(const string& search_text,
                    const vector<pair<string, vector<string>>>& all_possible_tx_hashes)
{

    // initalise page tempate map with basic info about blockchain
    mstch::map context {
            {"testnet"         , testnet},
            {"stagenet"        , stagenet},
            {"search_text"     , search_text},
            {"no_results"      , true},
            {"to_many_results" , false}
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
                    vector<MempoolStatus::mempool_tx> found_txs;

                    search_mempool(tx_hash_pod, found_txs);

                    if (!found_txs.empty())
                    {
                        // there should be only one tx found
                        tx = found_txs.at(0).tx;
                    }
                    else
                    {
                        return string("Cant get tx of hash (show_search_results): " + tx_hash);
                    }

                    // tx in mempool have no blk_timestamp
                    // but can use their recive time
                    blk_timestamp = found_txs.at(0).receive_time;

                }

                tx_details txd = get_tx_details(tx);

                mstch::map txd_map = txd.get_mstch_map();


                // add the timestamp to tx mstch map
                txd_map.insert({"timestamp", xmreg::timestamp_to_str_gm(blk_timestamp)});

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
            {"tx_table_head", template_file["tx_table_header"]},
            {"tx_table_row" , template_file["tx_table_row"]}
    };

    add_css_style(context);

    // render the page
    return  mstch::render(full_page, context, partials);
}

string
get_js_file(string const& fname)
{
    if (template_file.count(fname))
        return template_file[fname];

    return string{};
}




/*
 * Lets use this json api convention for success and error
 * https://labs.omniti.com/labs/jsend
 */
json
json_transaction(string tx_hash_str)
{
    json j_response {
            {"status", "fail"},
            {"data"  , json {}}
    };

    json& j_data = j_response["data"];

    // parse tx hash string to hash object
    crypto::hash tx_hash;

    if (!xmreg::parse_str_secret_key(tx_hash_str, tx_hash))
    {
        j_data["title"] = fmt::format("Cant parse tx hash: {:s}", tx_hash_str);
        return j_response;
    }

    // get transaction
    transaction tx;

    // flag to indicate if tx is in mempool
    bool found_in_mempool {false};

    // for tx in blocks we get block timestamp
    // for tx in mempool we get recievive time
    uint64_t tx_timestamp {0};

    if (!find_tx(tx_hash, tx, found_in_mempool, tx_timestamp))
    {
        j_data["title"] = fmt::format("Cant find tx hash: {:s}", tx_hash_str);
        return j_response;
    }

    uint64_t block_height {0};
    uint64_t is_coinbase_tx = is_coinbase(tx);
    uint64_t no_confirmations {0};

    if (found_in_mempool == false)
    {

        block blk;

        try
        {
            // get block cointaining this tx
            block_height = core_storage->get_db().get_tx_block_height(tx_hash);

            if (!mcore->get_block_by_height(block_height, blk))
            {
                j_data["title"] = fmt::format("Cant get block: {:d}", block_height);
                return j_response;
            }

            tx_timestamp = blk.timestamp;
        }
        catch (const exception& e)
        {
            j_response["status"]  = "error";
            j_response["message"] = fmt::format("Tx does not exist in blockchain, "
                                                        "but was there before: {:s}", tx_hash_str);
            return j_response;
        }
    }

    string blk_timestamp_utc = xmreg::timestamp_to_str_gm(tx_timestamp);

    // get the current blockchain height. Just to check
    uint64_t bc_height = core_storage->get_current_blockchain_height();

    tx_details txd = get_tx_details(tx, is_coinbase_tx, block_height, bc_height);

    json outputs;

    for (const auto& output: txd.output_pub_keys)
    {
        outputs.push_back(json {
                {"public_key", pod_to_hex(std::get<0>(output))},
                {"amount"    , std::get<1>(output)}
        });
    }

    json inputs;

    for (const txin_to_key &in_key: txd.input_key_imgs)
    {

        // get absolute offsets of mixins
        std::vector<uint64_t> absolute_offsets
                = cryptonote::relative_output_offsets_to_absolute(
                        in_key.key_offsets);

        // get public keys of outputs used in the mixins that match to the offests
        std::vector<output_data_t> outputs;

        try
        {
            // before proceeding with geting the outputs based on the amount and absolute offset
            // check how many outputs there are for that amount
            // go to next input if a too large offset was found
            if (are_absolute_offsets_good(absolute_offsets, in_key) == false)
                continue;

            //core_storage->get_db().get_output_key(in_key.amount,
                                                  //absolute_offsets,
                                                  //outputs);

            get_output_key<BlockchainDB>(in_key.amount,
                                           absolute_offsets,
                                           outputs);
        }
        catch (const OUTPUT_DNE &e)
        {
            j_response["status"]  = "error";
            j_response["message"] = "Failed to retrive outputs (mixins) used in key images";
            return j_response;
        }

        inputs.push_back(json {
                {"key_image"  , pod_to_hex(in_key.k_image)},
                {"amount"     , in_key.amount},
                {"mixins"     , json {}}
        });

        json& mixins = inputs.back()["mixins"];

        // mixin counter
        size_t count = 0;

        for (const uint64_t& abs_offset: absolute_offsets)
        {

            // get basic information about mixn's output
            cryptonote::output_data_t output_data = outputs.at(count++);

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
                        in_key.amount, abs_offset);

                cerr << out_msg << '\n';

                break;
            }

            string out_pub_key_str = pod_to_hex(output_data.pubkey);

            mixins.push_back(json {
                    {"public_key"  , pod_to_hex(output_data.pubkey)},
                    {"tx_hash"     , pod_to_hex(tx_out_idx.first)},
                    {"block_no"    , output_data.height},
            });
        }
    }

    if (found_in_mempool == false)
    {
        no_confirmations = txd.no_confirmations;
    }

    // get basic tx info
    j_data = get_tx_json(tx, txd);

    // append additional info from block, as we don't
    // return block data in this function
    j_data["timestamp"]      = tx_timestamp;
    j_data["timestamp_utc"]  = blk_timestamp_utc;
    j_data["block_height"]   = block_height;
    j_data["confirmations"]  = no_confirmations;
    j_data["outputs"]        = outputs;
    j_data["inputs"]         = inputs;
    j_data["current_height"] = bc_height;

    j_response["status"] = "success";

    return j_response;
}



/*
 * Lets use this json api convention for success and error
 * https://labs.omniti.com/labs/jsend
 */
json
json_rawtransaction(string tx_hash_str)
{
    json j_response {
            {"status", "fail"},
            {"data"  , json {}}
    };

    json& j_data = j_response["data"];

    // parse tx hash string to hash object
    crypto::hash tx_hash;

    if (!xmreg::parse_str_secret_key(tx_hash_str, tx_hash))
    {
        j_data["title"] = fmt::format("Cant parse tx hash: {:s}", tx_hash_str);
        return j_response;
    }

    // get transaction
    transaction tx;

    // flag to indicate if tx is in mempool
    bool found_in_mempool {false};

    // for tx in blocks we get block timestamp
    // for tx in mempool we get recievive time
    uint64_t tx_timestamp {0};

    if (!find_tx(tx_hash, tx, found_in_mempool, tx_timestamp))
    {
        j_data["title"] = fmt::format("Cant find tx hash: {:s}", tx_hash_str);
        return j_response;
    }

    if (found_in_mempool == false)
    {

        block blk;

        try
        {
            // get block cointaining this tx
            uint64_t block_height = core_storage->get_db().get_tx_block_height(tx_hash);

            if (!mcore->get_block_by_height(block_height, blk))
            {
                j_data["title"] = fmt::format("Cant get block: {:d}", block_height);
                return j_response;
            }
        }
        catch (const exception& e)
        {
            j_response["status"]  = "error";
            j_response["message"] = fmt::format("Tx does not exist in blockchain, "
                                                "but was there before: {:s}",
                                                tx_hash_str);
            return j_response;
        }
    }

    // get raw tx json as in monero

    try
    {
        j_data = json::parse(obj_to_json_str(tx));
    }
    catch (std::invalid_argument& e)
    {
        j_response["status"]  = "error";
        j_response["message"] = "Faild parsing raw tx data into json";
        return j_response;
    }

    j_response["status"] = "success";

    return j_response;
}


json
json_detailedtransaction(string tx_hash_str)
{
    json j_response {
            {"status", "fail"},
            {"data"  , json {}}
    };

    json& j_data = j_response["data"];

    transaction tx;

    bool found_in_mempool {false};
    uint64_t tx_timestamp {0};
    string error_message;

    if (!find_tx_for_json(tx_hash_str, tx, found_in_mempool, tx_timestamp, error_message))
    {
        j_data["title"] = error_message;
        return j_response;
    }

    // get detailed tx information
    mstch::map tx_context = construct_tx_context(tx, 1 /*full detailed */);

    // remove some page specific and html stuff
    tx_context.erase("timescales");
    tx_context.erase("tx_json");
    tx_context.erase("tx_json_raw");
    tx_context.erase("enable_mixins_details");
    tx_context.erase("with_ring_signatures");
    tx_context.erase("show_part_of_inputs");
    tx_context.erase("show_more_details_link");
    tx_context.erase("max_no_of_inputs_to_show");
    tx_context.erase("inputs_xmr_sum_not_zero");
    tx_context.erase("have_raw_tx");
    tx_context.erase("have_any_unknown_amount");
    tx_context.erase("has_error");
    tx_context.erase("error_msg");
    tx_context.erase("server_time");
    tx_context.erase("construction_time");

    j_data = tx_context;

    j_response["status"] = "success";

    return j_response;
}

/*
 * Lets use this json api convention for success and error
 * https://labs.omniti.com/labs/jsend
 */
json
json_block(string block_no_or_hash)
{
    json j_response {
            {"status", "fail"},
            {"data"  , json {}}
    };

    nlohmann::json& j_data = j_response["data"];

    uint64_t current_blockchain_height
            =  core_storage->get_current_blockchain_height();

    uint64_t block_height {0};

    crypto::hash blk_hash;

    block blk;

    if (block_no_or_hash.length() <= 8)
    {
        // we have something that seems to be a block number
        try
        {
            block_height  = boost::lexical_cast<uint64_t>(block_no_or_hash);
        }
        catch (const boost::bad_lexical_cast& e)
        {
            j_data["title"] = fmt::format(
                    "Cant parse block number: {:s}", block_no_or_hash);
            return j_response;
        }

        if (block_height > current_blockchain_height)
        {
            j_data["title"] = fmt::format(
                    "Requested block is higher than blockchain:"
                            " {:d}, {:d}", block_height,current_blockchain_height);
            return j_response;
        }

        if (!mcore->get_block_by_height(block_height, blk))
        {
            j_data["title"] = fmt::format("Cant get block: {:d}", block_height);
            return j_response;
        }

        blk_hash = core_storage->get_block_id_by_height(block_height);

    }
    else if (block_no_or_hash.length() == 64)
    {
        // this seems to be block hash
        if (!xmreg::parse_str_secret_key(block_no_or_hash, blk_hash))
        {
            j_data["title"] = fmt::format("Cant parse blk hash: {:s}", block_no_or_hash);
            return j_response;
        }

        if (!core_storage->get_block_by_hash(blk_hash, blk))
        {
            j_data["title"] = fmt::format("Cant get block: {:s}", blk_hash);
            return j_response;
        }

        block_height = core_storage->get_db().get_block_height(blk_hash);
    }
    else
    {
        j_data["title"] = fmt::format("Cant find blk using search string: {:s}", block_no_or_hash);
        return j_response;
    }


    // get block size in bytes
    uint64_t blk_size = core_storage->get_db().get_block_weight(block_height);

    // miner reward tx
    transaction coinbase_tx = blk.miner_tx;

    // transcation in the block
    vector<crypto::hash> tx_hashes = blk.tx_hashes;

    // sum of all transactions in the block
    uint64_t sum_fees = 0;

    // get tx details for the coinbase tx, i.e., miners reward
    tx_details txd_coinbase = get_tx_details(blk.miner_tx, true,
                                             block_height,
                                             current_blockchain_height);

    json j_txs;

    j_txs.push_back(get_tx_json(coinbase_tx, txd_coinbase));

    // for each transaction in the block
    for (size_t i = 0; i < blk.tx_hashes.size(); ++i)
    {
        const crypto::hash &tx_hash = blk.tx_hashes.at(i);

        // get transaction
        transaction tx;

        if (!mcore->get_tx(tx_hash, tx))
        {
            j_response["status"]  = "error";
            j_response["message"]
                    = fmt::format("Cant get transactions in block: {:d}", block_height);
            return j_response;
        }

        tx_details txd = get_tx_details(tx, false,
                                        block_height,
                                        current_blockchain_height);

        j_txs.push_back(get_tx_json(tx, txd));

        // add fee to the rest
        sum_fees += txd.fee;
    }

    j_data = json {
            {"block_height"  , block_height},
            {"hash"          , pod_to_hex(blk_hash)},
            {"timestamp"     , blk.timestamp},
            {"timestamp_utc" , xmreg::timestamp_to_str_gm(blk.timestamp)},
            {"block_height"  , block_height},
            {"size"          , blk_size},
            {"txs"           , j_txs},
            {"current_height", current_blockchain_height}
    };

    j_response["status"] = "success";

    return j_response;
}



/*
 * Lets use this json api convention for success and error
 * https://labs.omniti.com/labs/jsend
 */
json
json_rawblock(string block_no_or_hash)
{
    json j_response {
            {"status", "fail"},
            {"data"  , json {}}
    };

    json& j_data = j_response["data"];

    uint64_t current_blockchain_height
            =  core_storage->get_current_blockchain_height();

    uint64_t block_height {0};

    crypto::hash blk_hash;

    block blk;

    if (block_no_or_hash.length() <= 8)
    {
        // we have something that seems to be a block number
        try
        {
            block_height  = boost::lexical_cast<uint64_t>(block_no_or_hash);
        }
        catch (const boost::bad_lexical_cast& e)
        {
            j_data["title"] = fmt::format(
                    "Cant parse block number: {:s}", block_no_or_hash);
            return j_response;
        }

        if (block_height > current_blockchain_height)
        {
            j_data["title"] = fmt::format(
                    "Requested block is higher than blockchain:"
                            " {:d}, {:d}", block_height,current_blockchain_height);
            return j_response;
        }

        if (!mcore->get_block_by_height(block_height, blk))
        {
            j_data["title"] = fmt::format("Cant get block: {:d}", block_height);
            return j_response;
        }

        blk_hash = core_storage->get_block_id_by_height(block_height);

    }
    else if (block_no_or_hash.length() == 64)
    {
        // this seems to be block hash
        if (!xmreg::parse_str_secret_key(block_no_or_hash, blk_hash))
        {
            j_data["title"] = fmt::format("Cant parse blk hash: {:s}", block_no_or_hash);
            return j_response;
        }

        if (!core_storage->get_block_by_hash(blk_hash, blk))
        {
            j_data["title"] = fmt::format("Cant get block: {:s}", blk_hash);
            return j_response;
        }

        block_height = core_storage->get_db().get_block_height(blk_hash);
    }
    else
    {
        j_data["title"] = fmt::format("Cant find blk using search string: {:s}", block_no_or_hash);
        return j_response;
    }

    // get raw tx json as in monero

    try
    {
        j_data = json::parse(obj_to_json_str(blk));
    }
    catch (std::invalid_argument& e)
    {
        j_response["status"]  = "error";
        j_response["message"] = "Faild parsing raw blk data into json";
        return j_response;
    }

    j_response["status"] = "success";

    return j_response;
}


/*
 * Lets use this json api convention for success and error
 * https://labs.omniti.com/labs/jsend
 */
json
json_transactions(string _page, string _limit)
{
    json j_response {
            {"status", "fail"},
            {"data",   json {}}
    };

    json& j_data = j_response["data"];

    // parse page and limit into numbers

    uint64_t page {0};
    uint64_t limit {0};

    try
    {
        page  = boost::lexical_cast<uint64_t>(_page);
        limit = boost::lexical_cast<uint64_t>(_limit);
    }
    catch (const boost::bad_lexical_cast& e)
    {
        j_data["title"] = fmt::format(
                "Cant parse page and/or limit numbers: {:s}, {:s}", _page, _limit);
        return j_response;
    }

    // enforce maximum number of blocks per page to 100
    limit = limit > 100 ? 100 : limit;

    //get current server timestamp
    server_timestamp = std::time(nullptr);

    uint64_t local_copy_server_timestamp = server_timestamp;

    uint64_t height = core_storage->get_current_blockchain_height();

    // calculate starting and ending block numbers to show
    int64_t start_height = height - limit * (page + 1);

    // check if start height is not below range
    start_height = start_height < 0 ? 0 : start_height;

    int64_t end_height = start_height + limit - 1;

    // loop index
    int64_t i = end_height;

    j_data["blocks"] = json::array();
    json& j_blocks = j_data["blocks"];

    // iterate over last no_of_last_blocks of blocks
    while (i >= start_height)
    {
        // get block at the given height i
        block blk;

        if (!mcore->get_block_by_height(i, blk))
        {
            j_response["status"]  = "error";
            j_response["message"] = fmt::format("Cant get block: {:d}", i);
            return j_response;
        }

        // get block size in bytes
        double blk_size = core_storage->get_db().get_block_weight(i);

        crypto::hash blk_hash = core_storage->get_block_id_by_height(i);

        // get block age
        pair<string, string> age = get_age(local_copy_server_timestamp, blk.timestamp);

        j_blocks.push_back(json {
                {"height"       , i},
                {"hash"         , pod_to_hex(blk_hash)},
                {"age"          , age.first},
                {"size"         , blk_size},
                {"timestamp"    , blk.timestamp},
                {"timestamp_utc", xmreg::timestamp_to_str_gm(blk.timestamp)},
                {"txs"          , json::array()}
        });

        json& j_txs = j_blocks.back()["txs"];

        vector<cryptonote::transaction> blk_txs {blk.miner_tx};
        vector<crypto::hash> missed_txs;

        if (!core_storage->get_transactions(blk.tx_hashes, blk_txs, missed_txs))
        {
            j_response["status"]  = "error";
            j_response["message"] = fmt::format("Cant get transactions in block: {:d}", i);
            return j_response;
        }

        (void) missed_txs;

        for(auto it = blk_txs.begin(); it != blk_txs.end(); ++it)
        {
            const cryptonote::transaction &tx = *it;

            const tx_details& txd = get_tx_details(tx, false, i, height);

            j_txs.push_back(get_tx_json(tx, txd));
        }

        --i;
    }

    j_data["page"]           = page;
    j_data["limit"]          = limit;
    j_data["current_height"] = height;

    j_data["total_page_no"]  = limit > 0 ? (height / limit) : 0;

    j_response["status"] = "success";

    return j_response;
}


/*
* Lets use this json api convention for success and error
* https://labs.omniti.com/labs/jsend
*/
json
json_mempool(string _page, string _limit)
{
    json j_response {
            {"status", "fail"},
            {"data",   json {}}
    };

    json& j_data = j_response["data"];

    // parse page and limit into numbers

    uint64_t page {0};
    uint64_t limit {0};

    try
    {
        page  = boost::lexical_cast<uint64_t>(_page);
        limit = boost::lexical_cast<uint64_t>(_limit);
    }
    catch (const boost::bad_lexical_cast& e)
    {
        j_data["title"] = fmt::format(
                "Cant parse page and/or limit numbers: {:s}, {:s}", _page, _limit);
        return j_response;
    }

    //get current server timestamp
    server_timestamp = std::time(nullptr);

    uint64_t local_copy_server_timestamp = server_timestamp;

    uint64_t height = core_storage->get_current_blockchain_height();

    // get mempool tx from mempoolstatus thread
    vector<MempoolStatus::mempool_tx> mempool_data
            = MempoolStatus::get_mempool_txs();

    uint64_t no_mempool_txs = mempool_data.size();

    // calculate starting and ending block numbers to show
    int64_t start_height = limit * page;

    int64_t end_height = start_height + limit;

    end_height = end_height > no_mempool_txs ? no_mempool_txs : end_height;

    // check if start height is not below range
    start_height = start_height > end_height ? end_height - limit : start_height;

    start_height = start_height < 0 ? 0 : start_height;

    // loop index
    int64_t i = start_height;

    json j_txs = json::array();

    // for each transaction in the memory pool in current page
    while (i < end_height)
    {
        const MempoolStatus::mempool_tx* mempool_tx {nullptr};

        try
        {
            mempool_tx = &(mempool_data.at(i));
        }
        catch (const std::out_of_range& e)
        {
            j_response["status"]  = "error";
            j_response["message"] = fmt::format("Getting mempool txs failed due to std::out_of_range");

            return j_response;
        }

        const tx_details& txd = get_tx_details(mempool_tx->tx, false, 1, height); // 1 is dummy here

        // get basic tx info
        json j_tx = get_tx_json(mempool_tx->tx, txd);

        // we add some extra data, for mempool txs, such as recieve timestamp
        j_tx["timestamp"]     = mempool_tx->receive_time;
        j_tx["timestamp_utc"] = mempool_tx->timestamp_str;

        j_txs.push_back(j_tx);

       ++i;
    }

    j_data["txs"]            = j_txs;
    j_data["page"]           = page;
    j_data["limit"]          = limit;
    j_data["txs_no"]         = no_mempool_txs;
    j_data["total_page_no"]  = limit > 0 ? (no_mempool_txs / limit) : 0;

    j_response["status"] = "success";

    return j_response;
}


/*
 * Lets use this json api convention for success and error
 * https://labs.omniti.com/labs/jsend
 */
json
json_search(const string& search_text)
{
    json j_response {
            {"status", "fail"},
            {"data",   json {}}
    };

    json& j_data = j_response["data"];

    //get current server timestamp
    server_timestamp = std::time(nullptr);

    uint64_t local_copy_server_timestamp = server_timestamp;

    uint64_t height = core_storage->get_current_blockchain_height();

    uint64_t search_str_length = search_text.length();

    // first let check if the search_text matches any tx or block hash
    if (search_str_length == 64)
    {
        // first check for tx
        json j_tx = json_transaction(search_text);

        if (j_tx["status"] == "success")
        {
            j_response["data"]   = j_tx["data"];
            j_response["data"]["title"]  = "transaction";
            j_response["status"] = "success";
            return j_response;
        }

        // now check for block

        json j_block = json_block(search_text);

        if (j_block["status"] == "success")
        {
            j_response["data"]  = j_block["data"];
            j_response["data"]["title"]  = "block";
            j_response["status"] = "success";
            return j_response;
        }
    }

    // now lets see if this is a block number
    if (search_str_length <= 8)
    {
        json j_block = json_block(search_text);

        if (j_block["status"] == "success")
        {
            j_response["data"]   = j_block["data"];
            j_response["data"]["title"]  = "block";
            j_response["status"] = "success";
            return j_response;
        }
    }

    j_data["title"] = "Nothing was found that matches search string: " + search_text;

    return j_response;
}

json
json_outputs(string tx_hash_str,
             string address_str,
             string viewkey_str,
             bool tx_prove = false)
{
    boost::trim(tx_hash_str);
    boost::trim(address_str);
    boost::trim(viewkey_str);

    json j_response {
            {"status", "fail"},
            {"data",   json {}}
    };

    json& j_data = j_response["data"];


    if (tx_hash_str.empty())
    {
        j_response["status"]  = "error";
        j_response["message"] = "Tx hash not provided";
        return j_response;
    }

    if (address_str.empty())
    {
        j_response["status"]  = "error";
        j_response["message"] = "Monero address not provided";
        return j_response;
    }

    if (viewkey_str.empty())
    {
        if (!tx_prove)
        {
            j_response["status"]  = "error";
            j_response["message"] = "Viewkey not provided";
            return j_response;
        }
        else
        {
            j_response["status"]  = "error";
            j_response["message"] = "Tx private key not provided";
            return j_response;
        }
    }


    // parse tx hash string to hash object
    crypto::hash tx_hash;

    if (!xmreg::parse_str_secret_key(tx_hash_str, tx_hash))
    {
        j_response["status"]  = "error";
        j_response["message"] = "Cant parse tx hash: " + tx_hash_str;
        return j_response;
    }

    // parse string representing given monero address
    address_parse_info address_info;

    if (!xmreg::parse_str_address(address_str,  address_info, nettype))
    {
        j_response["status"]  = "error";
        j_response["message"] = "Cant parse monero address: " + address_str;
        return j_response;

    }

    // parse string representing given private key
    crypto::secret_key prv_view_key;

    if (!xmreg::parse_str_secret_key(viewkey_str, prv_view_key))
    {
        j_response["status"]  = "error";
        j_response["message"] = "Cant parse view key or tx private key: "
                                + viewkey_str;
        return j_response;
    }

    // get transaction
    transaction tx;

    // flag to indicate if tx is in mempool
    bool found_in_mempool {false};

    // for tx in blocks we get block timestamp
    // for tx in mempool we get recievive time
    uint64_t tx_timestamp {0};

    if (!find_tx(tx_hash, tx, found_in_mempool, tx_timestamp))
    {
        j_data["title"] = fmt::format("Cant find tx hash: {:s}", tx_hash_str);
        return j_response;
    }

    (void) tx_timestamp;
    (void) found_in_mempool;

    tx_details txd = get_tx_details(tx);

    // public transaction key is combined with our viewkey
    // to create, so called, derived key.
    key_derivation derivation;
    std::vector<key_derivation> additional_derivations(txd.additional_pks.size());

    public_key pub_key = tx_prove ? address_info.address.m_view_public_key : txd.pk;

    //cout << "txd.pk: " << pod_to_hex(txd.pk) << endl;

    if (!generate_key_derivation(pub_key, prv_view_key, derivation))
    {
        j_response["status"]  = "error";
        j_response["message"] = "Cant calculate key_derivation";
        return j_response;
    }
    for (size_t i = 0; i < txd.additional_pks.size(); ++i)
    {
        if (!generate_key_derivation(txd.additional_pks[i], prv_view_key, additional_derivations[i]))
        {
            j_response["status"]  = "error";
            j_response["message"] = "Cant calculate key_derivation";
            return j_response;
        }
    }

    uint64_t output_idx {0};

    std::vector<uint64_t> money_transfered(tx.vout.size(), 0);

    j_data["outputs"] = json::array();
    json& j_outptus   = j_data["outputs"];

    for (output_tuple_with_tag& outp: txd.output_pub_keys)
    {

        // get the tx output public key
        // that normally would be generated for us,
        // if someone had sent us some xmr.
        public_key tx_pubkey;

        derive_public_key(derivation,
                          output_idx,
                          address_info.address.m_spend_public_key,
                          tx_pubkey);

        // check if generated public key matches the current output's key
        bool mine_output = (std::get<0>(outp) == tx_pubkey);
        bool with_additional = false;
        if (!mine_output && txd.additional_pks.size() == txd.output_pub_keys.size())
        {
            derive_public_key(additional_derivations[output_idx],
                              output_idx,
                              address_info.address.m_spend_public_key,
                              tx_pubkey);
            mine_output = (std::get<0>(outp) == tx_pubkey);
            with_additional = true;
        }

        uint64_t xmr_amount  = std::get<1>(outp);

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

                auto derivation_to_use = with_additional
                        ? additional_derivations[output_idx] : derivation;

                r = decode_ringct(tx.rct_signatures,
                                  derivation_to_use,
                                  output_idx,
                                  tx.rct_signatures.ecdhInfo[output_idx].mask,
                                  rct_amount);

                if (!r)
                {
                    cerr << "\nshow_my_outputs: Cant decode ringCT! " << endl;
                }

                xmr_amount         = rct_amount;
                money_transfered[output_idx] = rct_amount;

            } // if (!is_coinbase(tx))

        }  // if (mine_output && tx.version == 2)

        j_outptus.push_back(json {
                {"output_pubkey", pod_to_hex(std::get<0>(outp))},
                {"amount"       , xmr_amount},
                {"match"        , mine_output},
                {"output_idx"   , output_idx},
        });

        ++output_idx;

    } // for (pair<public_key, uint64_t>& outp: txd.output_pub_keys)

    // if we don't already have the tx_timestamp from the mempool
    // then read it from the block that the transaction is in
    if (!tx_timestamp && txd.blk_height > 0) {
        block blk;
        mcore->get_block_by_height(txd.blk_height, blk);
        tx_timestamp = blk.timestamp;
    }

    // return parsed values. can be use to double
    // check if submited data in the request
    // matches to what was used to produce response.
    j_data["tx_hash"]  = pod_to_hex(txd.hash);
    j_data["address"]  = pod_to_hex(address_info.address);
    j_data["viewkey"]  = pod_to_hex(prv_view_key);
    j_data["tx_prove"] = tx_prove;
    j_data["tx_confirmations"] = txd.no_confirmations;
    j_data["tx_timestamp"] = tx_timestamp;

    j_response["status"] = "success";

    return j_response;
}



json
json_outputsblocks(string _limit,
                   string address_str,
                   string viewkey_str,
                   bool in_mempool_aswell = false)
{
    boost::trim(_limit);
    boost::trim(address_str);
    boost::trim(viewkey_str);

    json j_response {
            {"status", "fail"},
            {"data",   json {}}
    };

    json& j_data = j_response["data"];

    uint64_t no_of_last_blocks {3};

    try
    {
        no_of_last_blocks = boost::lexical_cast<uint64_t>(_limit);
    }
    catch (const boost::bad_lexical_cast& e)
    {
        j_data["title"] = fmt::format(
                "Cant parse page and/or limit numbers: {:s}", _limit);
        return j_response;
    }

    // maxium five last blocks
    no_of_last_blocks = std::min<uint64_t>(no_of_last_blocks, 5ul);

    if (address_str.empty())
    {
        j_response["status"]  = "error";
        j_response["message"] = "Monero address not provided";
        return j_response;
    }

    if (viewkey_str.empty())
    {
        j_response["status"]  = "error";
        j_response["message"] = "Viewkey not provided";
        return j_response;
    }

    // parse string representing given monero address
    address_parse_info address_info;

    if (!xmreg::parse_str_address(address_str, address_info, nettype))
    {
        j_response["status"]  = "error";
        j_response["message"] = "Cant parse monero address: " + address_str;
        return j_response;

    }

    // parse string representing given private key
    crypto::secret_key prv_view_key;

    if (!xmreg::parse_str_secret_key(viewkey_str, prv_view_key))
    {
        j_response["status"]  = "error";
        j_response["message"] = "Cant parse view key: "
                                + viewkey_str;
        return j_response;
    }

    string error_msg;

    j_data["outputs"] = json::array();
    json& j_outptus   = j_data["outputs"];


    if (in_mempool_aswell)
    {
        // first check if there is something for us in the mempool
        // get mempool tx from mempoolstatus thread
        vector<MempoolStatus::mempool_tx> mempool_txs
                = MempoolStatus::get_mempool_txs();

        uint64_t no_mempool_txs = mempool_txs.size();

        // need to use vector<transactions>,
        // not vector<MempoolStatus::mempool_tx>
        vector<transaction> tmp_vector;
        tmp_vector.reserve(no_mempool_txs);

        for (size_t i = 0; i < no_mempool_txs; ++i)
        {
            // get transaction info of the tx in the mempool
            tmp_vector.push_back(std::move(mempool_txs.at(i).tx));
        }

        if (!find_our_outputs(
                address_info.address, prv_view_key,
                0 /* block_no */, true /*is mempool*/,
                tmp_vector.cbegin(), tmp_vector.cend(),
                j_outptus /* found outputs are pushed to this*/,
                error_msg))
        {
            j_response["status"] = "error";
            j_response["message"] = error_msg;
            return j_response;
        }

    } // if (in_mempool_aswell)


    // and now serach for outputs in last few blocks in the blockchain

    uint64_t height = core_storage->get_current_blockchain_height();

    // calculate starting and ending block numbers to show
    int64_t start_height = height - no_of_last_blocks;

    // check if start height is not below range
    start_height = start_height < 0 ? 0 : start_height;

    int64_t end_height = start_height + no_of_last_blocks - 1;

    // loop index
    int64_t block_no = end_height;


    // iterate over last no_of_last_blocks of blocks
    while (block_no >= start_height)
    {
        // get block at the given height block_no
        block blk;

        if (!mcore->get_block_by_height(block_no, blk))
        {
            j_response["status"] = "error";
            j_response["message"] = fmt::format("Cant get block: {:d}", block_no);
            return j_response;
        }

        // get transactions in the given block
        vector<cryptonote::transaction> blk_txs{blk.miner_tx};
        vector<crypto::hash> missed_txs;

        if (!core_storage->get_transactions(blk.tx_hashes, blk_txs, missed_txs))
        {
            j_response["status"] = "error";
            j_response["message"] = fmt::format("Cant get transactions in block: {:d}", block_no);
            return j_response;
        }

        (void) missed_txs;

        if (!find_our_outputs(
                address_info.address, prv_view_key,
                block_no, false /*is mempool*/,
                blk_txs.cbegin(), blk_txs.cend(),
                j_outptus /* found outputs are pushed to this*/,
                error_msg))
        {
            j_response["status"] = "error";
            j_response["message"] = error_msg;
            return j_response;
        }

        --block_no;

    }  //  while (block_no >= start_height)

    // return parsed values. can be use to double
    // check if submited data in the request
    // matches to what was used to produce response.
    j_data["address"]  = pod_to_hex(address_info.address);
    j_data["viewkey"]  = pod_to_hex(prv_view_key);
    j_data["limit"]    = _limit;
    j_data["height"]   = height;
    j_data["mempool"]  = in_mempool_aswell;

    j_response["status"] = "success";

    return j_response;
}

/*
 * Lets use this json api convention for success and error
 * https://labs.omniti.com/labs/jsend
 */
json
json_networkinfo()
{
    json j_response {
            {"status", "fail"},
            {"data",   json {}}
    };

    json& j_data = j_response["data"];

    json j_info;

    // get basic network info
    if (!get_monero_network_info(j_info))
    {
        j_response["status"]  = "error";
        j_response["message"] = "Cant get monero network info";
    //    return j_response;
    }

    uint64_t per_kb_fee_estimated {0};

    // get dynamic fee estimate from last 10 blocks
    if (!get_dynamic_per_kb_fee_estimate(per_kb_fee_estimated))
    {
        j_response["status"]  = "error";
        j_response["message"] = "Cant get per kb dynamic fee esimate";
    //    return j_response;
    }

    uint64_t fee_estimated {0};

    j_info["fee_per_kb"] = per_kb_fee_estimated;
    j_info["fee_estimate"] = fee_estimated;

    j_info["tx_pool_size"]        = MempoolStatus::mempool_no.load();
    j_info["tx_pool_size_kbytes"] = MempoolStatus::mempool_size.load();

    j_data = j_info;

    j_response["status"]  = "success";

    return j_response;
}


/*
* Lets use this json api convention for success and error
* https://labs.omniti.com/labs/jsend
*/
json
json_emission()
{
    json j_response {
            {"status", "fail"},
            {"data",   json {}}
    };

    json& j_data = j_response["data"];

    json j_info;

    // get basic network info
    if (!CurrentBlockchainStatus::is_thread_running())
    {
        j_data["title"] = "Emission monitoring thread not enabled.";
        return j_response;
    }
    else
    {
        CurrentBlockchainStatus::Emission current_values
                = CurrentBlockchainStatus::get_emission();

        string emission_blk_no   = std::to_string(current_values.blk_no - 1);
        string emission_coinbase = xmr_amount_to_str(current_values.coinbase, "{:0.3f}");
        string emission_fee      = xmr_amount_to_str(current_values.fee, "{:0.4f}", false);

        j_data = json {
                {"blk_no"  , current_values.blk_no - 1},
                {"coinbase", current_values.coinbase},
                {"fee"     , current_values.fee},
        };
    }

    j_response["status"]  = "success";

    return j_response;
}


/*
      * Lets use this json api convention for success and error
      * https://labs.omniti.com/labs/jsend
      */
json
json_version()
{
    json j_response {
            {"status", "fail"},
            {"data",   json {}}
    };

    json& j_data = j_response["data"];

    j_data = json {
            {"last_git_commit_hash", string {GIT_COMMIT_HASH}},
            {"last_git_commit_date", string {GIT_COMMIT_DATETIME}},
            {"git_branch_name"     , string {GIT_BRANCH_NAME}},
            {"monero_version_full" , string {MONERO_VERSION_FULL}},
            {"api"                 , ONIONEXPLORER_RPC_VERSION},
            {"blockchain_height"   , core_storage->get_current_blockchain_height()}
    };

    j_response["status"]  = "success";

    return j_response;
}


private:


string
get_payment_id_as_string(
        tx_details const& txd,
        secret_key const& prv_view_key)
{
    string payment_id;

    // decrypt encrypted payment id, as used in integreated addresses
    crypto::hash8 decrypted_payment_id8 = txd.payment_id8;

    if (decrypted_payment_id8 != null_hash8)
    {
        if (mcore->get_device()->decrypt_payment_id(decrypted_payment_id8, txd.pk, prv_view_key))
        {
            payment_id = pod_to_hex(decrypted_payment_id8);
        }
    }
    else if(txd.payment_id != null_hash)
    {
        payment_id = pod_to_hex(txd.payment_id);
    }

    return payment_id;
}

template <typename Iterator>
bool
find_our_outputs(
        account_public_address const& address,
        secret_key const& prv_view_key,
        uint64_t const& block_no,
        bool const& is_mempool,
        Iterator const& txs_begin,
        Iterator const& txs_end,
        json& j_outptus,
        string& error_msg)
{

    // for each tx, perform output search using provided
    // address and viewkey
    for (auto it = txs_begin; it != txs_end; ++it)
    {
        cryptonote::transaction const& tx = *it;

        tx_details txd = get_tx_details(tx);

        // public transaction key is combined with our viewkey
        // to create, so called, derived key.
        key_derivation derivation;

        if (!generate_key_derivation(txd.pk, prv_view_key, derivation))
        {
            error_msg = "Cant calculate key_derivation";
            return false;
        }

        std::vector<key_derivation> additional_derivations(txd.additional_pks.size());
        for (size_t i = 0; i < txd.additional_pks.size(); ++i)
        {
            if (!generate_key_derivation(txd.additional_pks[i], prv_view_key, additional_derivations[i]))
            {
                error_msg = "Cant calculate key_derivation";
                return false;
            }
        }

        uint64_t output_idx{0};

        std::vector<uint64_t> money_transfered(tx.vout.size(), 0);

        //j_data["outputs"] = json::array();
        //json& j_outptus   = j_data["outputs"];

        for (output_tuple_with_tag &outp: txd.output_pub_keys)
        {

            // get the tx output public key
            // that normally would be generated for us,
            // if someone had sent us some xmr.
            public_key tx_pubkey;

            derive_public_key(derivation,
                              output_idx,
                              address.m_spend_public_key,
                              tx_pubkey);

            // check if generated public key matches the current output's key
            bool mine_output = (std::get<0>(outp) == tx_pubkey);
            bool with_additional = false;
            if (!mine_output && txd.additional_pks.size() == txd.output_pub_keys.size())
            {
                derive_public_key(additional_derivations[output_idx],
                                  output_idx,
                                  address.m_spend_public_key,
                                  tx_pubkey);
                mine_output = (std::get<0>(outp) == tx_pubkey);
                with_additional = true;
            }

            uint64_t xmr_amount = std::get<1>(outp);

            // if mine output has RingCT, i.e., tx version is 2
            if (mine_output && tx.version == 2)
            {
                // cointbase txs have amounts in plain sight.
                // so use amount from ringct, only for non-coinbase txs
                if (!is_coinbase(tx))
                {

                    // initialize with regular amount
                    uint64_t rct_amount = money_transfered[output_idx];

                    bool r {false};

                    rct::key mask = tx.rct_signatures.ecdhInfo[output_idx].mask;

                    auto derivation_to_use = with_additional
                            ? additional_derivations[output_idx] : derivation;

                    r = decode_ringct(tx.rct_signatures,
                                      derivation_to_use,
                                      output_idx,
                                      mask,
                                      rct_amount);

                    if (!r)
                    {
                        error_msg = "Cant decode ringct for tx: "
                                                + pod_to_hex(txd.hash);
                        return false;
                    }

                    xmr_amount = rct_amount;
                    money_transfered[output_idx] = rct_amount;

                } // if (!is_coinbase(tx))

            }  // if (mine_output && tx.version == 2)

            if (mine_output)
            {
                string payment_id_str = get_payment_id_as_string(txd, prv_view_key);

                j_outptus.push_back(json {
                        {"output_pubkey" , pod_to_hex(std::get<0>(outp))},
                        {"amount"        , xmr_amount},
                        {"block_no"      , block_no},
                        {"in_mempool"    , is_mempool},
                        {"output_idx"    , output_idx},
                        {"tx_hash"       , pod_to_hex(txd.hash)},
                        {"payment_id"    , payment_id_str}
                });
            }

            ++output_idx;

        } //  for (pair<public_key, uint64_t>& outp: txd.output_pub_keys)

    } // for (auto it = blk_txs.begin(); it != blk_txs.end(); ++it)

    return true;
}

json
get_tx_json(const transaction& tx, const tx_details& txd)
{

    json j_tx {
            {"tx_hash"     , pod_to_hex(txd.hash)},
            {"tx_fee"      , txd.fee},
            {"mixin"       , txd.mixin_no},
            {"tx_size"     , txd.size},
            {"xmr_outputs" , txd.xmr_outputs},
            {"xmr_inputs"  , txd.xmr_inputs},
            {"tx_version"  , static_cast<uint64_t>(txd.version)},
            {"rct_type"    , tx.rct_signatures.type},
            {"coinbase"    , is_coinbase(tx)},
            {"mixin"       , txd.mixin_no},
            {"extra"       , txd.get_extra_str()},
            {"payment_id"  , (txd.payment_id  != null_hash  ? pod_to_hex(txd.payment_id)  : "")},
            {"payment_id8" , (txd.payment_id8 != null_hash8 ? pod_to_hex(txd.payment_id8) : "")},
    };

    return j_tx;
}


bool
find_tx(const crypto::hash& tx_hash,
        transaction& tx,
        bool& found_in_mempool,
        uint64_t& tx_timestamp)
{

    found_in_mempool = false;

    if (!mcore->get_tx(tx_hash, tx))
    {
        cerr << "Cant get tx in blockchain: " << tx_hash
             << ". \n Check mempool now" << endl;

        vector<MempoolStatus::mempool_tx> found_txs;

        search_mempool(tx_hash, found_txs);

        if (!found_txs.empty())
        {
            // there should be only one tx found
            tx = found_txs.at(0).tx;
            found_in_mempool = true;
            tx_timestamp = found_txs.at(0).receive_time;
        }
        else
        {
            // tx is nowhere to be found :-(
            return false;
        }
    }

    return true;
}


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

    const crypto::hash& tx_hash = txd.hash;

    string tx_hash_str = pod_to_hex(tx_hash);

    uint64_t tx_blk_height {0};

    bool tx_blk_found {false};

    bool detailed_view {enable_mixins_details || static_cast<bool>(with_ring_signatures)};

    if (core_storage->have_tx(tx_hash))
    {
        // currently get_tx_block_height seems to return a block hight
        // +1. Before it was not like this.
        tx_blk_height = core_storage->get_db().get_tx_block_height(tx_hash);
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

        blk_timestamp = xmreg::timestamp_to_str_gm(blk.timestamp);

        tx_blk_height_str = std::to_string(tx_blk_height);
    }

    // payments id. both normal and encrypted (payment_id8)
    string pid_str   = pod_to_hex(txd.payment_id);
    string pid8_str  = pod_to_hex(txd.payment_id8);


    string tx_json = obj_to_json_str(tx);

    double tx_size = static_cast<double>(txd.size) / 1024.0;

    double payed_for_kB = XMR_AMOUNT(txd.fee) / tx_size;

    // initalise page tempate map with basic info about blockchain
    mstch::map context {
            {"testnet"               , testnet},
            {"stagenet"              , stagenet},
            {"tx_hash"               , tx_hash_str},
            {"tx_prefix_hash"        , string{}},
            {"tx_pub_key"            , pod_to_hex(txd.pk)},
            {"blk_height"            , tx_blk_height_str},
            {"tx_blk_height"         , tx_blk_height},
            {"tx_size"               , fmt::format("{:0.4f}", tx_size)},
            {"tx_fee"                , xmreg::xmr_amount_to_str(txd.fee, "{:0.12f}", false)},
            {"tx_fee_micro"          , xmreg::xmr_amount_to_str(txd.fee*1e6, "{:0.4f}", false)},
            {"payed_for_kB"          , fmt::format("{:0.12f}", payed_for_kB)},
            {"tx_version"            , static_cast<uint64_t>(txd.version)},
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
            {"construction_time"     , string {}},
    };

    // append tx_json as in raw format to html
    context["tx_json_raw"] = mstch::lambda{[=](const std::string& text) -> mstch::node {
        return tx_json;
    }};

    // append additional public tx keys, if there are any, to the html context

    string add_tx_pub_keys;

    for (auto const& apk: txd.additional_pks)
        add_tx_pub_keys += pod_to_hex(apk) + ";";

    context["add_tx_pub_keys"] = add_tx_pub_keys;

    string server_time_str = xmreg::timestamp_to_str_gm(server_timestamp, "%F");

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

    uint64_t max_no_of_inputs_to_show {10};

    // if a tx has more inputs than max_no_of_inputs_to_show,
    // we only show 10 first.
    bool show_part_of_inputs = (txd.input_key_imgs.size() > max_no_of_inputs_to_show);

    // but if we show all details, i.e.,
    // the show all inputs, regardless of their number
    if (detailed_view)
    {
        show_part_of_inputs = false;
    }

    vector<vector<uint64_t>> mixin_timestamp_groups;

    // make timescale maps for mixins in input
    for (const txin_to_key &in_key: txd.input_key_imgs)
    {

        if (show_part_of_inputs && (input_idx > max_no_of_inputs_to_show))
            break;

        // get absolute offsets of mixins
        std::vector<uint64_t> absolute_offsets
                = cryptonote::relative_output_offsets_to_absolute(
                        in_key.key_offsets);

        // get public keys of outputs used in the mixins that match to the offests
        std::vector<cryptonote::output_data_t> outputs;

        try
        {
            // before proceeding with geting the outputs based on the amount and absolute offset
            // check how many outputs there are for that amount
            // go to next input if a too large offset was found
            if (are_absolute_offsets_good(absolute_offsets, in_key) == false)
                continue;

            // offsets seems good, so try to get the outputs for the amount and
            // offsets given
            //core_storage->get_db().get_output_key(in_key.amount,
                                                  //absolute_offsets,
                                                  //outputs);
            
            get_output_key<BlockchainDB>(in_key.amount,
                                           absolute_offsets,
                                           outputs);
        }
        catch (const std::exception& e)
        {
            string out_msg = fmt::format(
                    "Outputs with amount {:d} do not exist and indexes ",
                    in_key.amount
            );

            for (auto offset: absolute_offsets)
                out_msg += ", " + to_string(offset);

            out_msg += " don't exist! " + string {e.what()};

            cerr << out_msg << '\n';

            context["has_error"] = true;
            context["error_msg"] = out_msg;

            return context;
        }

        inputs.push_back(mstch::map {
                {"in_key_img"   , pod_to_hex(in_key.k_image)},
                {"amount"       , xmreg::xmr_amount_to_str(in_key.amount)},
                {"input_idx"    , fmt::format("{:02d}", input_idx)},
                {"mixins"       , mstch::array{}},
                {"ring_sigs"    , mstch::array{}},
                {"already_spent", false} // placeholder for later
        });

        if (detailed_view)
        {
            boost::get<mstch::map>(inputs.back())["ring_sigs"]
                    = txd.get_ring_sig_for_input(input_idx);
        }


        inputs_xmr_sum += in_key.amount;

        if (in_key.amount == 0)
        {
            // if any input has amount equal to zero,
            // it is really an unkown amount
            have_any_unknown_amount = true;
        }

        vector<uint64_t> mixin_timestamps;

        // get reference to mixins array created above
        mstch::array& mixins = boost::get<mstch::array>(
                boost::get<mstch::map>(inputs.back())["mixins"]);

        // mixin counter
        size_t count = 0;

        // for each found output public key find its block to get timestamp
        for (const uint64_t& i: absolute_offsets)
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


            if (detailed_view)
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
                        {"mix_pub_key",    pod_to_hex(output_data.pubkey)},
                        {"mix_tx_hash",    pod_to_hex(tx_out_idx.first)},
                        {"mix_out_indx",   tx_out_idx.second},
                        {"mix_timestamp",  xmreg::timestamp_to_str_gm(blk.timestamp)},
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
            else //  if (detailed_view)
            {
                mixins.push_back(mstch::map {
                        {"mix_blk",        fmt::format("{:08d}", output_data.height)},
                        {"mix_pub_key",    pod_to_hex(output_data.pubkey)},
                        {"mix_idx",        fmt::format("{:02d}", count)},
                        {"mix_is_it_real", false}, // a placeholder for future
                });

            } // else  if (enable_mixins_details)

            ++count;

        } // for (const uint64_t &i: absolute_offsets)

        mixin_timestamp_groups.push_back(mixin_timestamps);

        input_idx++;

    } // for (const txin_to_key& in_key: txd.input_key_imgs)



    if (detailed_view)
    {
        uint64_t min_mix_timestamp {0};
        uint64_t max_mix_timestamp {0};

        pair<mstch::array, double> mixins_timescales
                = construct_mstch_mixin_timescales(
                        mixin_timestamp_groups,
                        min_mix_timestamp,
                        max_mix_timestamp
                );


        context["min_mix_time"]     = xmreg::timestamp_to_str_gm(min_mix_timestamp);
        context["max_mix_time"]     = xmreg::timestamp_to_str_gm(max_mix_timestamp);

        context.emplace("timescales", mixins_timescales.first);


        context["timescales_scale"] = fmt::format("{:0.2f}",
                                                  mixins_timescales.second / 3600.0 / 24.0); // in days

        context["tx_prefix_hash"] = pod_to_hex(get_transaction_prefix_hash(tx));

    }


    context["have_any_unknown_amount"]  = have_any_unknown_amount;
    context["inputs_xmr_sum_not_zero"]  = (inputs_xmr_sum > 0);
    context["inputs_xmr_sum"]           = xmreg::xmr_amount_to_str(inputs_xmr_sum);
    context["server_time"]              = server_time_str;
    context["enable_mixins_details"]    = detailed_view;
    context["enable_as_hex"]            = enable_as_hex;
    context["show_part_of_inputs"]      = show_part_of_inputs;
    context["max_no_of_inputs_to_show"] = max_no_of_inputs_to_show;


    context.emplace("inputs", inputs);

    // get indices of outputs in amounts tables
    vector<uint64_t> out_amount_indices;

    try
    {

        uint64_t tx_index;

        if (core_storage->get_db().tx_exists(txd.hash, tx_index))
        {
            //out_amount_indices = core_storage->get_db()
                    //.get_tx_amount_output_indices(tx_index).front();
            get_tx_amount_output_indices<BlockchainDB>(
                   out_amount_indices, 
                   tx_index);
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

    for (output_tuple_with_tag& outp: txd.output_pub_keys)
    {

        // total number of ouputs in the blockchain for this amount
        uint64_t num_outputs_amount = core_storage->get_db()
                .get_num_outputs(std::get<1>(outp));

        string out_amount_index_str {"N/A"};

        // outputs in tx in them mempool dont have yet global indices
        // thus for them, we print N/A
        if (!out_amount_indices.empty())
        {
            out_amount_index_str
                    = std::to_string(out_amount_indices.at(output_idx));
        }

        outputs_xmr_sum += std::get<1>(outp);

        std::stringstream ss;
        if (std::get<2>(outp)) {
            ss << *(std::get<2>(outp));
        }
        else {
          ss << "-";
        }
        string view_tag_str = ss.str();


        outputs.push_back(mstch::map {
                {"out_pub_key"           , pod_to_hex(std::get<0>(outp))},
                {"amount"                , xmreg::xmr_amount_to_str(std::get<1>(outp))},
                {"amount_idx"            , out_amount_index_str},
                {"num_outputs"           , num_outputs_amount},
                {"output_tag"            , view_tag_str},
                {"unformated_output_idx" , output_idx},
                {"output_idx"            , fmt::format("{:02d}", output_idx++)}
        });

    } //  for (pair<public_key, uint64_t>& outp: txd.output_pub_keys)

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
get_tx_details(const transaction& tx,
               bool coinbase = false,
               uint64_t blk_height = 0,
               uint64_t bc_height = 0)
{
    tx_details txd;

    // get tx hash
    
    if (!tx.pruned)
    {
        txd.hash = get_transaction_hash(tx);
    }
    else
    {
        txd.hash = get_pruned_transaction_hash(tx, tx.prunable_hash);
    }

    // get tx public key from extra
    // this check if there are two public keys
    // due to previous bug with sining txs:
    // https://github.com/monero-project/monero/pull/1358/commits/7abfc5474c0f86e16c405f154570310468b635c2
    txd.pk = xmreg::get_tx_pub_key_from_received_outs(tx);
    txd.additional_pks = cryptonote::get_additional_tx_pub_keys_from_extra(tx);


    // sum xmr in inputs and ouputs in the given tx
    const array<uint64_t, 4>& sum_data = summary_of_in_out_rct(
            tx, txd.output_pub_keys, txd.input_key_imgs);

    txd.xmr_outputs       = sum_data[0];
    txd.xmr_inputs        = sum_data[1];
    txd.mixin_no          = sum_data[2];
    txd.num_nonrct_inputs = sum_data[3];

    txd.fee = 0;

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

    // get tx size in bytes
    txd.size = get_object_blobsize(tx);

    txd.extra = tx.extra;

    // get tx signatures for each input
    txd.signatures = tx.signatures;

    // get tx version
    txd.version = tx.version;

    // get unlock time
    txd.unlock_time = tx.unlock_time;

    txd.no_confirmations = 0;

    if (blk_height == 0 && core_storage->have_tx(txd.hash))
    {
        // if blk_height is zero then search for tx block in
        // the blockchain. but since often block height is know a priory
        // this is not needed

        txd.blk_height = core_storage->get_db().get_tx_block_height(txd.hash);

        // get the current blockchain height. Just to check
        uint64_t bc_height = core_storage->get_current_blockchain_height();

        txd.no_confirmations = bc_height - (txd.blk_height);
    }
    else
    {
        // if we know blk_height, and current blockchan height
        // just use it to get no_confirmations.

        txd.no_confirmations = bc_height - (blk_height);
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
find_tx_for_json(
        string const& tx_hash_str,
        transaction& tx,
        bool& found_in_mempool,
        uint64_t& tx_timestamp,
        string& error_message)
{
    // parse tx hash string to hash object
    crypto::hash tx_hash;

    if (!xmreg::parse_str_secret_key(tx_hash_str, tx_hash))
    {
        error_message = fmt::format("Cant parse tx hash: {:s}", tx_hash_str);
        return false;
    }

    if (!find_tx(tx_hash, tx, found_in_mempool, tx_timestamp))
    {
        error_message = fmt::format("Cant find tx hash: {:s}", tx_hash_str);
        return false;
    }

    return true;
}

bool
search_mempool(crypto::hash tx_hash,
               vector<MempoolStatus::mempool_tx>& found_txs)
{
    // if tx_hash == null_hash then this method
    // will just return the vector containing all
    // txs in mempool



    // get mempool tx from mempoolstatus thread
    vector<MempoolStatus::mempool_tx> mempool_txs
            = MempoolStatus::get_mempool_txs();

    // if dont have tx_blob member, construct tx
    // from json obtained from the rpc call

    for (size_t i = 0; i < mempool_txs.size(); ++i)
    {
        // get transaction info of the tx in the mempool
        const MempoolStatus::mempool_tx& mempool_tx = mempool_txs.at(i);

        if (tx_hash == mempool_tx.tx_hash || tx_hash == null_hash)
        {
            found_txs.push_back(mempool_tx);

            if (tx_hash != null_hash)
                break;
        }

    } // for (size_t i = 0; i < mempool_txs.size(); ++i)

    return true;
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

bool
get_monero_network_info(json& j_info)
{
    MempoolStatus::network_info local_copy_network_info
        = MempoolStatus::current_network_info;

    j_info = json {
       {"status"                    , local_copy_network_info.current},
       {"current"                   , local_copy_network_info.current},
       {"height"                    , local_copy_network_info.height},
       {"target_height"             , local_copy_network_info.target_height},
       {"difficulty"                , make_difficulty(local_copy_network_info.difficulty, local_copy_network_info.difficulty_top64).str()},
       {"target"                    , local_copy_network_info.target},
       {"hash_rate"                 , local_copy_network_info.hash_rate},
       {"tx_count"                  , local_copy_network_info.tx_count},
       {"tx_pool_size"              , local_copy_network_info.tx_pool_size},
       {"alt_blocks_count"          , local_copy_network_info.alt_blocks_count},
       {"outgoing_connections_count", local_copy_network_info.outgoing_connections_count},
       {"incoming_connections_count", local_copy_network_info.incoming_connections_count},
       {"white_peerlist_size"       , local_copy_network_info.white_peerlist_size},
       {"grey_peerlist_size"        , local_copy_network_info.grey_peerlist_size},
       {"testnet"                   , local_copy_network_info.nettype == cryptonote::network_type::TESTNET},
       {"stagenet"                  , local_copy_network_info.nettype == cryptonote::network_type::STAGENET},
       {"top_block_hash"            , pod_to_hex(local_copy_network_info.top_block_hash)},
       {"cumulative_difficulty"     , make_difficulty(local_copy_network_info.cumulative_difficulty, local_copy_network_info.cumulative_difficulty_top64).str()},
       {"block_size_limit"          , local_copy_network_info.block_size_limit},
       {"block_size_median"         , local_copy_network_info.block_size_median},
       {"start_time"                , local_copy_network_info.start_time},
       {"fee_per_kb"                , local_copy_network_info.fee_per_kb},
       {"current_hf_version"        , local_copy_network_info.current_hf_version}
    };

    return local_copy_network_info.current;
}

bool
get_dynamic_per_kb_fee_estimate(uint64_t& fee_estimated)
{

    string error_msg;

    if (!rpc.get_dynamic_per_kb_fee_estimate(
            FEE_ESTIMATE_GRACE_BLOCKS,
            fee_estimated, error_msg))
    {
        cerr << "rpc.get_dynamic_per_kb_fee_estimate failed" << endl;
        return false;
    }

    (void) error_msg;

    return true;
}

bool
get_base_fee_estimate(uint64_t& fee_estimated)
{

    string error_msg;

    if (!rpc.get_base_fee_estimate(
            FEE_ESTIMATE_GRACE_BLOCKS,
            fee_estimated))
    {
        cerr << "rpc.get_base_fee_estimate failed" << endl;
        return false;
    }

    (void) error_msg;

    return true;
}

bool
are_absolute_offsets_good(
        std::vector<uint64_t> const& absolute_offsets,
        txin_to_key const& in_key)
{
    // before proceeding with geting the outputs based on the amount and absolute offset
    // check how many outputs there are for that amount
    uint64_t no_outputs = core_storage->get_db().get_num_outputs(in_key.amount);

    bool offset_too_large {false};

    int offset_idx {-1};

    for (auto o: absolute_offsets)
    {
        offset_idx++;

        if (o >= no_outputs)
        {
            offset_too_large = true;
            cerr << "Absolute offset (" << o << ") of an output in a key image "
                 << pod_to_hex(in_key.k_image)
                 << " (ring member no: " << offset_idx << ") "
                 << "for amount "  << in_key.amount
                 << " is too large. There are only "
                 << no_outputs << " such outputs!\n";
            continue;
        }
    }

    return !offset_too_large;
}

string
get_footer()
{
    // set last git commit date based on
    // autogenrated version.h during compilation
    static const mstch::map footer_context {
            {"last_git_commit_hash", string {GIT_COMMIT_HASH}},
            {"last_git_commit_date", string {GIT_COMMIT_DATETIME}},
            {"git_branch_name"     , string {GIT_BRANCH_NAME}},
            {"monero_version_full" , string {MONERO_VERSION_FULL}},
            {"api"                 , std::to_string(ONIONEXPLORER_RPC_VERSION_MAJOR)
                                     + "."
                                     + std::to_string(ONIONEXPLORER_RPC_VERSION_MINOR)},
    };

    string footer_html = mstch::render(xmreg::read(TMPL_FOOTER), footer_context);

    return footer_html;
}

void
add_css_style(mstch::map& context)
{
    // add_css_style goes to every subpage so here we mark

    context["css_styles"] = mstch::lambda{[&](const std::string& text) -> mstch::node {
        return template_file["css_styles"];
    }};
}

bool
get_tx(string const& tx_hash_str,
       transaction& tx,
       crypto::hash& tx_hash)
{
    if (!epee::string_tools::hex_to_pod(tx_hash_str, tx_hash))
    {
        string msg = fmt::format("Cant parse {:s} as tx hash!", tx_hash_str);
        cerr << msg << endl;
        return false;
    }

    // get transaction

    if (!mcore->get_tx(tx_hash, tx))
    {
        cerr << "Cant get tx in blockchain: " << tx_hash
             << ". \n Check mempool now\n";

        vector<MempoolStatus::mempool_tx> found_txs;

        search_mempool(tx_hash, found_txs);

        if (found_txs.empty())
        {
            // tx is nowhere to be found :-(
            return false;
        }

        tx = found_txs.at(0).tx;
    }

    return true;
}

vector<randomx_status>
get_randomx_code(uint64_t blk_height, 
                 block const& blk,
                 crypto::hash const& blk_hash)
{
    static std::mutex mtx;

    vector<randomx_status> rx_code;

    blobdata bd = get_block_hashing_blob(blk);

    std::lock_guard<std::mutex> lk {mtx};

    if (!main_vm_full)
    {

        crypto::hash block_hash;

        // this will create main_vm_full instance if one
        // does not exist
        me_get_block_longhash(core_storage, blk, block_hash, blk_height, 0);

        if (!main_vm_full)
        {
            cerr << "main_vm_full is still null!";
            return {};
        }
    }


     // based on randomx calculate hash
    // the hash is seed used to generated scrachtpad and program
    alignas(16) uint64_t tempHash[8];
    blake2b(tempHash, sizeof(tempHash), bd.data(), bd.size(), nullptr, 0); 

    main_vm_full->initScratchpad(&tempHash);
    main_vm_full->resetRoundingMode();

    for (int chain = 0; chain < RANDOMX_PROGRAM_COUNT - 1; ++chain) 
    {
        main_vm_full->run(&tempHash);

        blake2b(tempHash, sizeof(tempHash), 
                main_vm_full->getRegisterFile(),
                sizeof(randomx::RegisterFile), nullptr, 0); 

        rx_code.push_back({});

        rx_code.back().prog = main_vm_full->getProgram();
        rx_code.back().reg_file = *(main_vm_full->getRegisterFile());
    }   

    main_vm_full->run(&tempHash);

    rx_code.push_back({});

    rx_code.back().prog = main_vm_full->getProgram();
    rx_code.back().reg_file = *(main_vm_full->getRegisterFile());

    //crypto::hash res2;
    //main_vm_full->getFinalResult(res2.data, RANDOMX_HASH_SIZE);
    //cout << "pow2: " << pod_to_hex(res2) << endl;

    return rx_code;
}

template <typename T, typename... Args>
typename std::enable_if<
    HasSpanInGetOutputKeyT<T>::value, void>::type
get_output_key(uint64_t amount, Args&&... args)
{
  core_storage->get_db().get_output_key(
          epee::span<const uint64_t>(&amount, 1), 
          std::forward<Args>(args)...);
}

template <typename T, typename... Args>
typename std::enable_if<
    !HasSpanInGetOutputKeyT<T>::value, void>::type
get_output_key(uint64_t amount, Args&&... args)
{
  core_storage->get_db().get_output_key(
          amount, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
typename std::enable_if<
    !OutputIndicesReturnVectOfVectT<T>::value, void>::type
get_tx_amount_output_indices(vector<uint64_t>& out_amount_indices, Args&&... args)
{
    out_amount_indices = core_storage->get_db()
       .get_tx_amount_output_indices(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
typename std::enable_if<
    OutputIndicesReturnVectOfVectT<T>::value, void>::type
get_tx_amount_output_indices(vector<uint64_t>& out_amount_indices, Args&&... args)
{
    out_amount_indices = core_storage->get_db()
       .get_tx_amount_output_indices(std::forward<Args>(args)...).front();
}

};
}

#endif //CROWXMR_PAGE_H


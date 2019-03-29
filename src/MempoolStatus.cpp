//
// Created by mwo on 28/05/17.
//

#include "MempoolStatus.h"

#include "rpccalls.h"

namespace xmreg
{

using namespace std;


void
MempoolStatus::set_blockchain_variables(MicroCore *_mcore,
                                        Blockchain *_core_storage)
{
    mcore = _mcore;
    core_storage = _core_storage;
}

void
MempoolStatus::start_mempool_status_thread()
{

    // to protect from deviation by zero below.
    mempool_refresh_time = std::max<uint64_t>(1, mempool_refresh_time);

    if (!is_running)
    {
        m_thread = boost::thread{[]()
        {
         try
         {
             uint64_t loop_index {0};

            // so that network status is checked every minute
            uint64_t loop_index_divider = std::max<uint64_t>(1, 60 / mempool_refresh_time);

            while (true)
            {

             // we just query network status every minute. No sense
             // to do it as frequently as getting mempool data.
             if (loop_index % loop_index_divider == 0)
             {
                 if (!MempoolStatus::read_network_info())
                 {
                     network_info local_copy = current_network_info;

                     cerr << " Cant read network info "<< endl;

                     local_copy.current = false;

                     current_network_info = local_copy;
                 }
                 else
                 {
                     cout << "Current network info read, ";
                     loop_index = 0;
                 }
             }

             if (MempoolStatus::read_mempool())
             {
                 vector<mempool_tx> current_mempool_txs = get_mempool_txs();

                 cout << "mempool status txs: "
                      << current_mempool_txs.size()
                      << endl;
             }

             // when we reach top of the blockchain, update
             // the emission amount every minute.
             boost::this_thread::sleep_for(
                     boost::chrono::seconds(mempool_refresh_time));

             ++loop_index;

             } // while (true)
         }
         catch (boost::thread_interrupted&)
         {
             cout << "Mempool status thread interrupted." << endl;
             return;
        }

        }}; //  m_thread = boost::thread{[]()

        is_running = true;

    } //  if (!is_running)
}


bool
MempoolStatus::read_mempool()
{
    rpccalls rpc {deamon_url};

    string error_msg;

    // we populate this variable instead of global mempool_txs
    // mempool_txs will be changed only when this function completes.
    // this ensures that we don't sent out partial mempool txs to
    // other places.
    vector<mempool_tx> local_copy_of_mempool_txs;

    // get txs in the mempool
    std::vector<tx_info> mempool_tx_info;

    if (!rpc.get_mempool(mempool_tx_info))
    {
        cerr << "Getting mempool failed " << endl;
        return false;
    }

    // if dont have tx_blob member, construct tx
    // from json obtained from the rpc call

    uint64_t mempool_size_kB {0};

    for (size_t i = 0; i < mempool_tx_info.size(); ++i)
    {
        // get transaction info of the tx in the mempool
        const tx_info& _tx_info = mempool_tx_info.at(i);

        transaction tx;
        crypto::hash tx_hash;
        crypto::hash tx_prefix_hash;

        if (!parse_and_validate_tx_from_blob(
                _tx_info.tx_blob, tx, tx_hash, tx_prefix_hash))
        {
            cerr << "Cant make tx from _tx_info.tx_blob" << endl;
            return false;
        }

        mempool_size_kB += _tx_info.blob_size;

        local_copy_of_mempool_txs.push_back(mempool_tx{});

        mempool_tx& last_tx = local_copy_of_mempool_txs.back();

        last_tx.tx_hash = tx_hash;
        last_tx.tx = tx;

        // key images of inputs
        vector<txin_to_key> input_key_imgs;

        // public keys and xmr amount of outputs
        vector<pair<txout_to_key, uint64_t>> output_pub_keys;

        // sum xmr in inputs and ouputs in the given tx
        const array<uint64_t, 4>& sum_data = summary_of_in_out_rct(
               tx, output_pub_keys, input_key_imgs);



        double tx_size =  static_cast<double>(_tx_info.blob_size)/1024.0;

        double payed_for_kB = XMR_AMOUNT(_tx_info.fee) / tx_size;

        last_tx.receive_time = _tx_info.receive_time;

        last_tx.sum_outputs       = sum_data[0];
        last_tx.sum_inputs        = sum_data[1];
        last_tx.no_outputs        = output_pub_keys.size();
        last_tx.no_inputs         = input_key_imgs.size();
        last_tx.mixin_no          = sum_data[2];
        last_tx.num_nonrct_inputs = sum_data[3];

        last_tx.fee_str          = xmreg::xmr_amount_to_str(_tx_info.fee, "{:0.4f}", false);
        last_tx.payed_for_kB_str = fmt::format("{:0.4f}", payed_for_kB);
        last_tx.xmr_inputs_str   = xmreg::xmr_amount_to_str(last_tx.sum_inputs , "{:0.3f}");
        last_tx.xmr_outputs_str  = xmreg::xmr_amount_to_str(last_tx.sum_outputs, "{:0.3f}");
        last_tx.timestamp_str    = xmreg::timestamp_to_str_gm(_tx_info.receive_time);

        last_tx.txsize           = fmt::format("{:0.2f}", tx_size);

        last_tx.pID              = '-';

        crypto::hash payment_id;
        crypto::hash8 payment_id8;

        get_payment_id(tx, payment_id, payment_id8);

        if (payment_id != null_hash)
            last_tx.pID = 'l'; // legacy payment id
        else if (payment_id8 != null_hash8)
            last_tx.pID = 'e'; // encrypted payment id
        else if (!get_additional_tx_pub_keys_from_extra(tx).empty())
        {
            // if multioutput tx have additional public keys,
            // mark it so that it represents that it has at least
            // one sub-address
            last_tx.pID = 's';
        }
       // } // if (hex_to_pod(_tx_info.id_hash, mem_tx_hash))

    } // for (size_t i = 0; i < mempool_tx_info.size(); ++i)



    Guard lck (mempool_mutx);

    // clear current mempool txs vector
    // repopulate it with each execution of read_mempool()
    // not very efficient but good enough for now.

    mempool_no   = local_copy_of_mempool_txs.size();
    mempool_size = mempool_size_kB;

    mempool_txs = std::move(local_copy_of_mempool_txs);

    return true;
}


bool
MempoolStatus::read_network_info()
{
    rpccalls rpc {deamon_url};

    COMMAND_RPC_GET_INFO::response rpc_network_info;

    if (!rpc.get_network_info(rpc_network_info))
        return false;

    uint64_t fee_estimated;

    string error_msg;

    if (!rpc.get_dynamic_per_kb_fee_estimate(
            FEE_ESTIMATE_GRACE_BLOCKS,
            fee_estimated, error_msg))
    {
        cerr << "rpc.get_dynamic_per_kb_fee_estimate failed" << endl;
        return false;
    }

    (void) error_msg;

    COMMAND_RPC_HARD_FORK_INFO::response rpc_hardfork_info;

    if (!rpc.get_hardfork_info(rpc_hardfork_info))
        return false;


    network_info local_copy;

    local_copy.status                     = network_info::get_status_uint(rpc_network_info.status);
    local_copy.height                     = rpc_network_info.height;
    local_copy.target_height              = rpc_network_info.target_height;
    local_copy.difficulty                 = rpc_network_info.difficulty;
    local_copy.difficulty_top64           = rpc_network_info.difficulty_top64;
    local_copy.target                     = rpc_network_info.target;
    cryptonote::difficulty_type hash_rate = cryptonote::difficulty_type(rpc_network_info.wide_difficulty) / rpc_network_info.target;
    local_copy.hash_rate                  = (hash_rate & 0xFFFFFFFFFFFFFFFF).convert_to<uint64_t>();
    local_copy.hash_rate_top64            = ((hash_rate >> 64) & 0xFFFFFFFFFFFFFFFF).convert_to<uint64_t>();
    local_copy.tx_count                   = rpc_network_info.tx_count;
    local_copy.tx_pool_size               = rpc_network_info.tx_pool_size;
    local_copy.alt_blocks_count           = rpc_network_info.alt_blocks_count;
    local_copy.outgoing_connections_count = rpc_network_info.outgoing_connections_count;
    local_copy.incoming_connections_count = rpc_network_info.incoming_connections_count;
    local_copy.white_peerlist_size        = rpc_network_info.white_peerlist_size;
    local_copy.nettype                    = rpc_network_info.testnet ? cryptonote::network_type::TESTNET : 
                                            rpc_network_info.stagenet ? cryptonote::network_type::STAGENET : cryptonote::network_type::MAINNET;
    local_copy.cumulative_difficulty      = rpc_network_info.cumulative_difficulty;
    local_copy.cumulative_difficulty_top64 = rpc_network_info.cumulative_difficulty_top64;
    local_copy.block_size_limit           = rpc_network_info.block_size_limit;
    local_copy.block_size_median          = rpc_network_info.block_size_median;
    local_copy.start_time                 = rpc_network_info.start_time;


    strncpy(local_copy.block_size_limit_str, fmt::format("{:0.2f}",
                                             static_cast<double>(
                                             local_copy.block_size_limit ) / 2.0 / 1024.0).c_str(),
                                             sizeof(local_copy.block_size_limit_str));


    strncpy(local_copy.block_size_median_str, fmt::format("{:0.2f}",
                                              static_cast<double>(
                                              local_copy.block_size_median) / 1024.0).c_str(),
                                              sizeof(local_copy.block_size_median_str));

    epee::string_tools::hex_to_pod(rpc_network_info.top_block_hash,
                                   local_copy.top_block_hash);

    local_copy.fee_per_kb                 = fee_estimated;
    local_copy.info_timestamp             = static_cast<uint64_t>(std::time(nullptr));

    local_copy.current_hf_version         = rpc_hardfork_info.version;

    local_copy.current                    = true;

    current_network_info = local_copy;

    return true;
}

vector<MempoolStatus::mempool_tx>
MempoolStatus::get_mempool_txs()
{
    Guard lck (mempool_mutx);
    return mempool_txs;
}

vector<MempoolStatus::mempool_tx>
MempoolStatus::get_mempool_txs(uint64_t no_of_tx)
{
    Guard lck (mempool_mutx);

    no_of_tx = std::min<uint64_t>(no_of_tx, mempool_txs.size());

    return vector<mempool_tx>(mempool_txs.begin(), mempool_txs.begin() + no_of_tx);
}

bool
MempoolStatus::is_thread_running()
{
    return is_running;
}

bf::path MempoolStatus::blockchain_path {"/home/mwo/.bitmonero/lmdb"};
string MempoolStatus::deamon_url {"http:://127.0.0.1:11181"};
cryptonote::network_type MempoolStatus::nettype {cryptonote::network_type::MAINNET};
atomic<bool>       MempoolStatus::is_running {false};
boost::thread      MempoolStatus::m_thread;
Blockchain*        MempoolStatus::core_storage {nullptr};
xmreg::MicroCore*  MempoolStatus::mcore {nullptr};
vector<MempoolStatus::mempool_tx> MempoolStatus::mempool_txs;
atomic<MempoolStatus::network_info> MempoolStatus::current_network_info;
atomic<uint64_t> MempoolStatus::mempool_no {0};   // no of txs
atomic<uint64_t> MempoolStatus::mempool_size {0}; // size in bytes.
uint64_t MempoolStatus::mempool_refresh_time {10};
mutex MempoolStatus::mempool_mutx;
}

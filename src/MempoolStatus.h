//
// Created by mwo on 28/05/17.
//

#ifndef XMRBLOCKS_MEMPOOLSTATUS_H
#define XMRBLOCKS_MEMPOOLSTATUS_H


#include "MicroCore.h"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace xmreg
{

struct MempoolStatus
{

    using Guard = std::lock_guard<std::mutex>;

    struct mempool_tx
    {
        crypto::hash tx_hash;
        transaction tx;

        uint64_t receive_time {0};
        uint64_t sum_inputs {0};
        uint64_t sum_outputs {0};
        uint64_t no_inputs {0};
        uint64_t no_outputs {0};
        uint64_t num_nonrct_inputs {0};
        uint64_t mixin_no {0};

        string fee_str;
        string payed_for_kB_str;
        string xmr_inputs_str;
        string xmr_outputs_str;
        string timestamp_str;
        string txsize;

        char     pID; // '-' - no payment ID,
                      // 'l' - legacy, long 64 character payment id,
                      // 'e' - encrypted, short, from integrated addresses
    };


    // to keep network_info in cache
    // and to show previous info in case current querry for
    // the current info timesout.
    struct network_info
    {
        uint64_t status {0};
        uint64_t height  {0};
        uint64_t target_height  {0};
        uint64_t difficulty  {0};
        uint64_t target  {0};
        uint64_t tx_count  {0};
        uint64_t tx_pool_size  {0};
        uint64_t alt_blocks_count  {0};
        uint64_t outgoing_connections_count  {0};
        uint64_t incoming_connections_count  {0};
        uint64_t white_peerlist_size  {0};
        uint64_t grey_peerlist_size  {0};
        cryptonote::network_type nettype {cryptonote::network_type::MAINNET};
        crypto::hash top_block_hash;
        uint64_t cumulative_difficulty  {0};
        uint64_t block_size_limit  {0};
        uint64_t block_size_median  {0};
        char block_size_limit_str[10];   // needs to be trivially copyable
        char block_size_median_str[10];  // std::string is not trivially copyable
        uint64_t start_time  {0};
        uint64_t current_hf_version {0};

        uint64_t hash_rate  {0};
        uint64_t fee_per_kb  {0};
        uint64_t info_timestamp  {0};

        bool current {false};

        static uint64_t
        get_status_uint(const string& status)
        {
            if (status == CORE_RPC_STATUS_OK)
                return 1;

            if (status == CORE_RPC_STATUS_BUSY)
                return 2;

            // default
            return 0;
        }

        static string
        get_status_string(const uint64_t& status)
        {
            if (status == 1)
                return CORE_RPC_STATUS_OK;

            if (status == 2)
                return CORE_RPC_STATUS_BUSY;

            // default
            return 0;
        }
    };

    static boost::thread m_thread;

    static mutex mempool_mutx;

    static atomic<bool> is_running;

    static uint64_t mempool_refresh_time;

    static atomic<uint64_t> mempool_no;   // no of txs
    static atomic<uint64_t> mempool_size; // size in bytes.

    static bf::path blockchain_path;
    static string deamon_url;
    static cryptonote::network_type nettype;

    // make object for accessing the blockchain here
    static MicroCore* mcore;
    static Blockchain* core_storage;

    // vector of mempool transactions that all threads
    // can refer to
    //           <recieved_time, transaction>
    static vector<mempool_tx> mempool_txs;

    static atomic<network_info> current_network_info;

    static void
    set_blockchain_variables(MicroCore* _mcore,
                             Blockchain* _core_storage);

    static void
    start_mempool_status_thread();

    static bool
    read_mempool();

    static bool
    read_network_info();

    static vector<mempool_tx>
    get_mempool_txs();

    // get first no_of_tx from the vector
    static vector<mempool_tx>
    get_mempool_txs(uint64_t no_of_tx);

    static bool
    is_thread_running();
};

}
#endif //XMRBLOCKS_MEMPOOLSTATUS_H

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

    struct mempool_tx
    {
        crypto::hash tx_hash;
        tx_info info;
        transaction tx;

        uint64_t sum_inputs {0};
        uint64_t sum_outputs {0};
        uint64_t no_inputs {0};
        uint64_t no_outputs {0};
        uint64_t num_nonrct_inputs {0};
        uint64_t mixin_no {0};

        string fee_str;
        string xmr_inputs_str;
        string xmr_outputs_str;
        string timestamp_str;
        string txsize;
    };

    static boost::thread m_thread;

    static mutex mempool_mutx;

    static atomic<bool> is_running;

    static uint64_t mempool_refresh_time;

    static atomic<uint64_t> mempool_no;   // no of txs
    static atomic<uint64_t> mempool_size; // size in bytes.

    static bf::path blockchain_path;
    static string deamon_url;
    static bool testnet;

    // make object for accessing the blockchain here
    static MicroCore* mcore;
    static Blockchain* core_storage;

    // vector of mempool transactions that all threads
    // can refer to
    //           <recieved_time, transaction>
    static vector<mempool_tx> mempool_txs;

    static void
    set_blockchain_variables(MicroCore* _mcore,
                             Blockchain* _core_storage);

    static void
    start_mempool_status_thread();

    static bool
    read_mempool();

    static vector<mempool_tx>
    get_mempool_txs();

    static bool
    is_thread_running();
};

}
#endif //XMRBLOCKS_MEMPOOLSTATUS_H

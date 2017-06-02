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

        mempool_tx(
            crypto::hash _tx_hash,
            tx_info _info,
            transaction _tx)
            : tx_hash(_tx_hash), info(_info), tx(_tx)
        {}

    };

    static boost::thread m_thread;

    static mutex mempool_mutx;

    static atomic<bool> is_running;

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

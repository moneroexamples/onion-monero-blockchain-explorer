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

    static boost::thread m_thread;

    static atomic<bool> is_running;

    static bf::path blockchain_path;
    static string deamon_url;
    static bool testnet;

    // make object for accessing the blockchain here
    static MicroCore* mcore;
    static Blockchain* core_storage;

    static void
    set_blockchain_variables(MicroCore* _mcore,
                             Blockchain* _core_storage);

    static void
    start_mempool_status_thread();

    static bool
    is_thread_running();
};

}
#endif //XMRBLOCKS_MEMPOOLSTATUS_H

//
// Created by mwo on 16/05/17.
//

#ifndef XMRBLOCKS_CURRENTBLOCKCHAINSTATUS_H
#define XMRBLOCKS_CURRENTBLOCKCHAINSTATUS_H

#include "MicroCore.h"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace xmreg
{

using namespace std;

class CurrentBlockchainStatus
{
    static string blockchain_path;

    static string output_file;

    static string deamon_url;

    static atomic<uint64_t> current_height;

    static atomic<uint64_t> total_emission_amount;
    static atomic<uint64_t> total_fee_amount;

    static atomic<uint64_t> searched_blk_no;

    static std::thread m_thread;

    static atomic<bool> is_running;

    // make object for accessing the blockchain here
    static unique_ptr<xmreg::MicroCore> mcore;
    static cryptonote::Blockchain *core_storage;


public:
    static void
    start_monitor_blockchain_thread();

    static bool
    init_monero_blockchain();

    static void
    update_current_emission_amount();

    static bool
    save_current_emission_amount();

    static bool
    load_current_emission_amount();

    static vector<uint64_t>
    get_emission_amount();

    static bool
    is_thread_running();

};

}

#endif //XMRBLOCKS_CURRENTBLOCKCHAINSTATUS_H

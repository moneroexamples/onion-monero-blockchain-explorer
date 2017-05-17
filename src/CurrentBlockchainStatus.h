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

struct CurrentBlockchainStatus
{

    struct Emission
    {
        uint64_t coinbase;
        uint64_t fee;
        uint64_t blk_no;

        inline uint64_t
        checksum() const
        {
            return coinbase + fee + blk_no;
        }

        operator
        std::string() const
        {
            return to_string(blk_no) + "," + to_string(coinbase)
                   + "," + to_string(fee) + "," + to_string(checksum());
        }
    };

    static string blockchain_path;

    static bool testnet;

    static string output_file;

    static string deamon_url;

    static uint64_t blockchain_chunk_size;

    static atomic<uint64_t> current_height;

    static atomic<Emission> total_emission_atomic;

    static std::thread m_thread;

    static atomic<bool> is_running;

    // make object for accessing the blockchain here
    static unique_ptr<xmreg::MicroCore> mcore;

    static cryptonote::Blockchain *core_storage;

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

    static Emission
    get_emission();

    static string
    get_output_file_path();

    static bool
    is_thread_running();
};

}

#endif //XMRBLOCKS_CURRENTBLOCKCHAINSTATUS_H

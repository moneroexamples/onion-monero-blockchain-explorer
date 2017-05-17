//
// Created by mwo on 16/05/17.
//

#include "CurrentBlockchainStatus.h"

namespace xmreg
{

using namespace std;



bool
CurrentBlockchainStatus::init_monero_blockchain()
{
    // set  monero log output level
    uint32_t log_level = 0;
    mlog_configure(mlog_get_default_log_path(""), true);

    mcore = unique_ptr<xmreg::MicroCore>(new xmreg::MicroCore{});

    // initialize the core using the blockchain path
    if (!mcore->init(blockchain_path))
    {
        cerr << "Error accessing blockchain." << endl;
        return false;
    }

    // get the high level Blockchain object to interact
    // with the blockchain lmdb database
    core_storage = &(mcore->get_core());

    return true;
}


void
CurrentBlockchainStatus::start_monitor_blockchain_thread()
{
    total_emission_atomic = Emission {0, 0, 0};

    string emmision_saved_file = get_output_file_path();

    if (boost::filesystem::exists(emmision_saved_file))
    {
        if (!load_current_emission_amount())
        {
            cerr << "Emission file cant be read, got corrupted or has incorrect format:\n " << emmision_saved_file
                 << "\nEmission monitoring thread is not started.\nDelete the file and"
                 << " restart the explorer or disable emission monitoring."
                 << endl;

            cerr << "Press ENTER to continue without emission monitoring or Ctr+C to exit" << endl;

            cin.get();

            return;
        }
    }

    if (!is_running)
    {
        m_thread = std::thread{[]()
           {
               while (true)
               {
                   Emission current_emission = total_emission_atomic;

                   current_height = core_storage->get_current_blockchain_height();

                   update_current_emission_amount();

                   save_current_emission_amount();

                   if (current_emission.blk_no < current_height - blockchain_chunk_size)
                   {
                       std::this_thread::sleep_for(std::chrono::seconds(1));
                   }
                   else
                   {
                       std::this_thread::sleep_for(std::chrono::seconds(60));
                   }
               }
           }};

        is_running = true;
    }
}


void
CurrentBlockchainStatus::update_current_emission_amount()
{

    Emission current_emission = total_emission_atomic;

    uint64_t blk_no = current_emission.blk_no;

    uint64_t end_block = blk_no + blockchain_chunk_size;

    uint64_t current_blockchain_height = current_height;

    end_block = end_block > current_blockchain_height ? current_blockchain_height : end_block;

    uint64_t temp_emission_amount {0};
    uint64_t temp_fee_amount {0};

    while (blk_no < end_block)
    {

        block blk;

        mcore->get_block_by_height(blk_no, blk);

        uint64_t coinbase_amount = get_outs_money_amount(blk.miner_tx);

        std::list<transaction> txs;
        std::list<crypto::hash> missed_txs;

        uint64_t tx_fee_amount = 0;

        core_storage->get_transactions(blk.tx_hashes, txs, missed_txs);

        for(const auto& tx: txs)
        {
            tx_fee_amount += get_tx_fee(tx);
        }

        temp_emission_amount += coinbase_amount - tx_fee_amount;
        temp_fee_amount      += tx_fee_amount;

        ++blk_no;
    }

    current_emission.coinbase += temp_emission_amount;
    current_emission.fee      += temp_fee_amount;
    current_emission.blk_no    = blk_no;

    total_emission_atomic = current_emission;

    cout << "total emission: " << string(current_emission) << endl;
}



bool
CurrentBlockchainStatus::save_current_emission_amount()
{

    string emmision_saved_file = get_output_file_path();

    ofstream out(emmision_saved_file);

    if( !out )
    {
        cerr << "Couldn't open file."  << endl;
        return false;
    }

    Emission current_emission = total_emission_atomic;

    out << string(current_emission) << flush;

    return true;
}


bool
CurrentBlockchainStatus::load_current_emission_amount()
{
    string emmision_saved_file = get_output_file_path();

    string last_saved_emmision = xmreg::read(emmision_saved_file);

    if (last_saved_emmision.empty())
    {
        cerr << "Couldn't open file." << endl;
        return false;
    }

    last_saved_emmision.erase(last_saved_emmision.find_last_not_of(" \n\r\t")+1);

    vector<string> strs;
    boost::split(strs, last_saved_emmision, boost::is_any_of(","));

    if (strs.empty())
    {
        cerr << "Problem spliting string values form  emission_amount." << endl;
        return false;
    }

    Emission emission_loaded {0, 0, 0};

    uint64_t read_check_sum {0};

    try
    {
        emission_loaded.blk_no   = boost::lexical_cast<uint64_t>(strs.at(0));
        emission_loaded.coinbase = boost::lexical_cast<uint64_t>(strs.at(1));
        emission_loaded.fee      = boost::lexical_cast<uint64_t>(strs.at(2));
        read_check_sum           = boost::lexical_cast<uint64_t>(strs.at(3));
    }
    catch (boost::bad_lexical_cast &e)
    {
        cerr << "Cant parse to number date from string: " << last_saved_emmision << endl;
        return false;
    }

    if (read_check_sum != emission_loaded.checksum())
    {
        cerr << "read_check_sum != check_sum: "
             << read_check_sum << " != " << emission_loaded.checksum()
             << endl;

        return false;
    }

    total_emission_atomic = emission_loaded;

    return true;

}

string
CurrentBlockchainStatus::get_output_file_path()
{
    string emmision_saved_file = blockchain_path + output_file;

    return emmision_saved_file;
}


CurrentBlockchainStatus::Emission
CurrentBlockchainStatus::get_emission()
{
    return total_emission_atomic.load();
}

bool
CurrentBlockchainStatus::is_thread_running()
{
   return is_running;
}

string CurrentBlockchainStatus::blockchain_path{"/home/mwo/.bitmonero/lmdb"};

bool   CurrentBlockchainStatus::testnet {false};

string CurrentBlockchainStatus::output_file {"/emission_amount.txt"};

string CurrentBlockchainStatus::deamon_url{"http:://127.0.0.1:18081"};

uint64_t  CurrentBlockchainStatus::blockchain_chunk_size {10000};

atomic<uint64_t> CurrentBlockchainStatus::current_height {0};

atomic<CurrentBlockchainStatus::Emission> CurrentBlockchainStatus::total_emission_atomic;

std::thread      CurrentBlockchainStatus::m_thread;

atomic<bool>     CurrentBlockchainStatus::is_running {false};

cryptonote::Blockchain*       CurrentBlockchainStatus::core_storage;
unique_ptr<xmreg::MicroCore>  CurrentBlockchainStatus::mcore;

}

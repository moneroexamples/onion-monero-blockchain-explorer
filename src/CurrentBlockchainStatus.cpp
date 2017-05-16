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

    string emmision_saved_file = blockchain_path + output_file;

    if (boost::filesystem::exists(emmision_saved_file))
    {
        if (!load_current_emission_amount())
        {
            cerr << "Cant read or has incorrect format: " << emmision_saved_file << endl;
            return;
        }
    }

    if (!is_running)
    {
        m_thread = std::thread{[]()
           {
               while (true)
               {
                   current_height = core_storage->get_current_blockchain_height();

                   update_current_emission_amount();

                   save_current_emission_amount();

                   if (searched_blk_no < current_height - 1000)
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

    cout << "updating emission rate: " << current_height << endl;

    uint64_t no_of_blocks_to_search_at_once {1000};

    uint64_t blk_no = searched_blk_no;

    uint64_t end_block = blk_no + no_of_blocks_to_search_at_once;

    uint64_t current_blockchain_height = current_height;

    end_block = end_block > current_blockchain_height ? current_blockchain_height : end_block;

    uint64_t current_emission_amount {0};
    uint64_t current_fee_amount {0};

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

        current_emission_amount += coinbase_amount - tx_fee_amount;
        current_fee_amount += tx_fee_amount;

        ++blk_no;
    }

    searched_blk_no = blk_no;
    total_emission_amount += current_emission_amount;
    total_fee_amount += current_fee_amount;

    cout << "blk: " << blk_no
         << " total_emission_amount: " << xmr_amount_to_str(uint64_t(total_emission_amount))
         << " total_fee_amount: " << xmr_amount_to_str(uint64_t(total_fee_amount))
         << endl;
}



bool
CurrentBlockchainStatus::save_current_emission_amount()
{

    string emmision_saved_file = blockchain_path + output_file;

    ofstream out(emmision_saved_file);

    if( !out )
    {
        cerr << "Couldn't open file."  << endl;
        return false;
    }

    uint64_t check_sum = uint64_t(searched_blk_no)
                         + uint64_t(total_emission_amount)
                         + uint64_t(total_fee_amount);

    string out_line = to_string(searched_blk_no)
                      + "," + to_string(total_emission_amount)
                      + "," + to_string(total_fee_amount)
                      + "," + to_string(check_sum);

    out << out_line << flush;

    return true;
}


bool
CurrentBlockchainStatus::load_current_emission_amount()
{
    string emmision_saved_file = blockchain_path + output_file;

    string last_saved_emmision = xmreg::read(emmision_saved_file);

    if (last_saved_emmision.empty()) {
        cerr << "Couldn't open file." << endl;
        return false;
    }

    vector<string> strs;
    boost::split(strs, last_saved_emmision, boost::is_any_of(","));

    if (strs.empty()) {
        cerr << "Problem spliting string values form  emission_amount." << endl;
        return false;
    }

    uint64_t read_check_sum{0};

    try {
        searched_blk_no = boost::lexical_cast<uint64_t>(strs.at(0));
        total_emission_amount = boost::lexical_cast<uint64_t>(strs.at(1));
        total_fee_amount = boost::lexical_cast<uint64_t>(strs.at(2));
        read_check_sum = boost::lexical_cast<uint64_t>(strs.at(3));
    }
    catch (boost::bad_lexical_cast &e) {
        cerr << "Cant parse to number: " << last_saved_emmision << endl;
        return false;
    }

    uint64_t check_sum = uint64_t(searched_blk_no)
                         + uint64_t(total_emission_amount)
                         + uint64_t(total_fee_amount);

    if (read_check_sum != check_sum) {
        cerr << "read_check_sum != check_sum: "
             << read_check_sum << " != " << check_sum
             << endl;

        return false;
    }

    return true;

}

vector<uint64_t>
CurrentBlockchainStatus::get_emission_amount()
{

    uint64_t searched_block  = searched_blk_no;
    uint64_t emission_amount = total_emission_amount;
    uint64_t fee_amount      = total_fee_amount;

    return {searched_block, emission_amount, fee_amount};

}

bool
CurrentBlockchainStatus::is_thread_running()
{
   return is_running;
}

string CurrentBlockchainStatus::blockchain_path{"/home/mwo/.bitmonero/lmdb"};

string CurrentBlockchainStatus::output_file {"/emission_amount.txt"};

string CurrentBlockchainStatus::deamon_url{"http:://127.0.0.1:18081"};

atomic<uint64_t> CurrentBlockchainStatus::current_height {0};

atomic<uint64_t> CurrentBlockchainStatus::total_emission_amount {0} ;

atomic<uint64_t> CurrentBlockchainStatus::total_fee_amount {0} ;

atomic<uint64_t> CurrentBlockchainStatus::searched_blk_no {0};

std::thread      CurrentBlockchainStatus::m_thread;

atomic<bool>     CurrentBlockchainStatus::is_running {false};

cryptonote::Blockchain*       CurrentBlockchainStatus::core_storage;
unique_ptr<xmreg::MicroCore>  CurrentBlockchainStatus::mcore;

}

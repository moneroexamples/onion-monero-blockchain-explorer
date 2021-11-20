//
// Created by mwo on 16/05/17.
//

#include "CurrentBlockchainStatus.h"

namespace xmreg
{

using namespace std;



void
CurrentBlockchainStatus::set_blockchain_variables(MicroCore* _mcore,
                                                  Blockchain* _core_storage)
{
    mcore = _mcore;
    core_storage =_core_storage;
}


void
CurrentBlockchainStatus::start_monitor_blockchain_thread()
{
    total_emission_atomic = Emission {0, 0, 0};

    string emmision_saved_file = get_output_file_path().string();

    // read stored emission data if possible
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
        m_thread = boost::thread{[]()
           {
               try
               {
                   while (true)
                   {
                       Emission current_emission = total_emission_atomic;

                       current_height = core_storage->get_current_blockchain_height();

                       // scan 10000 blocks for emissiom or if we are at the top of
                       // the blockchain, only few top blocks
                       update_current_emission_amount();

                       cout << "current emission: " << string(current_emission) << endl;

                       save_current_emission_amount();

                       if (current_emission.blk_no < current_height - blockchain_chunk_size)
                       {
                           // while we scan the blockchain from scrach, every 10000
                           // blocks take 1 second break
                           boost::this_thread::sleep_for(boost::chrono::seconds(1));
                       }
                       else
                       {
                           // when we reach top of the blockchain, update
                           // the emission amount every minute.
                           boost::this_thread::sleep_for(boost::chrono::seconds(60));
                       }

                   } // while (true)
               }
               catch (boost::thread_interrupted&)
               {
                   cout << "Emission monitoring thread interrupted." << endl;
                   return;
               }

           }}; //  m_thread = boost::thread{[]()

        is_running = true;

    } //  if (!is_running)
}


void
CurrentBlockchainStatus::update_current_emission_amount()
{

    Emission current_emission = total_emission_atomic;

    uint64_t blk_no = current_emission.blk_no;

    uint64_t end_block = blk_no + blockchain_chunk_size;

    uint64_t current_blockchain_height = current_height;

    // blockchain_chunk_gap is used so that we
    // never read and store few top blocks
    // the emission in the top few blocks will be calcalted
    // later
    end_block = end_block > current_blockchain_height
                ? current_blockchain_height - blockchain_chunk_gap
                : end_block;

    Emission emission_calculated = calculate_emission_in_blocks(blk_no, end_block);

    current_emission.coinbase += emission_calculated.coinbase;
    current_emission.fee      += emission_calculated.fee;
    current_emission.blk_no    = emission_calculated.blk_no;

    total_emission_atomic = current_emission;
}

CurrentBlockchainStatus::Emission
CurrentBlockchainStatus::calculate_emission_in_blocks(
        uint64_t start_blk, uint64_t end_blk)
{
    Emission emission_calculated {0, 0, 0};

    while (start_blk < end_blk)
    {
        block blk;

        mcore->get_block_by_height(start_blk, blk);

        uint64_t coinbase_amount = get_outs_money_amount(blk.miner_tx);

        vector<transaction> txs;
        vector<crypto::hash> missed_txs;

        uint64_t tx_fee_amount = 0;

        core_storage->get_transactions(blk.tx_hashes, txs, missed_txs);

        for(const auto& tx: txs)
        {
            tx_fee_amount += get_tx_fee(tx);
        }

        (void) missed_txs;

        emission_calculated.coinbase += coinbase_amount - tx_fee_amount;
        emission_calculated.fee      += tx_fee_amount;

        ++start_blk;
    }

    emission_calculated.blk_no  = start_blk;

    return emission_calculated;
}


bool
CurrentBlockchainStatus::save_current_emission_amount()
{

    string emmision_saved_file = get_output_file_path().string();

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
    string emmision_saved_file = get_output_file_path().string();

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

bf::path
CurrentBlockchainStatus::get_output_file_path()
{
    return blockchain_path / output_file;
}


CurrentBlockchainStatus::Emission
CurrentBlockchainStatus::get_emission()
{
    // get current emission
    Emission current_emission = total_emission_atomic;

    // this emission will be few blocks behind current blockchain
    // height. By default 3 blocks. So we need to calcualate here
    // the emission from the top missing blocks, to have complete
    // emission data.

    uint64_t current_blockchain_height = current_height;

    uint64_t start_blk = current_emission.blk_no;

    // this should be at current hight or above
    // as we calculate missing blocks only for top blockchain
    // height
    uint64_t end_block = start_blk + blockchain_chunk_gap;

    if (end_block >= current_blockchain_height
        && start_blk < current_blockchain_height)
    {
        // make sure we are not over the blockchain height
        end_block = end_block > current_blockchain_height
                    ? current_blockchain_height : end_block;

        // calculated emission for missing blocks
        Emission gap_emission_calculated
                = calculate_emission_in_blocks(start_blk, end_block);

        //cout << "gap_emission_calculated: " << std::string(gap_emission_calculated) << endl;

        current_emission.coinbase += gap_emission_calculated.coinbase;
        current_emission.fee      += gap_emission_calculated.fee;
        current_emission.blk_no    = gap_emission_calculated.blk_no > 0
                                     ? gap_emission_calculated.blk_no
                                     : current_emission.blk_no;
    }

    return current_emission;
}

bool
CurrentBlockchainStatus::is_thread_running()
{
   return is_running;
}

bf::path CurrentBlockchainStatus::blockchain_path {"/home/mwo/.bitmonero/lmdb"};

cryptonote::network_type CurrentBlockchainStatus::nettype {cryptonote::network_type::MAINNET};

string CurrentBlockchainStatus::output_file {"emission_amount.txt"};

string CurrentBlockchainStatus::daemon_url {"http:://127.0.0.1:18081"};

uint64_t  CurrentBlockchainStatus::blockchain_chunk_size {10000};

uint64_t  CurrentBlockchainStatus::blockchain_chunk_gap {3};

atomic<uint64_t> CurrentBlockchainStatus::current_height {0};

atomic<CurrentBlockchainStatus::Emission> CurrentBlockchainStatus::total_emission_atomic;

boost::thread      CurrentBlockchainStatus::m_thread;

atomic<bool>     CurrentBlockchainStatus::is_running {false};

Blockchain*       CurrentBlockchainStatus::core_storage {nullptr};
xmreg::MicroCore*  CurrentBlockchainStatus::mcore {nullptr};
}

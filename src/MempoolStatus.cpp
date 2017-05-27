//
// Created by mwo on 28/05/17.
//

#include "MempoolStatus.h"

namespace xmreg
{

using namespace std;


void
MempoolStatus::set_blockchain_variables(MicroCore *_mcore,
                                        Blockchain *_core_storage) {
    mcore = _mcore;
    core_storage = _core_storage;
}



void
MempoolStatus::start_mempool_status_thread()
{

    if (!is_running)
    {
        m_thread = boost::thread{[]()
        {
         try
         {
             while (true)
             {

                 cout << "mempool status: " << endl;


                 // when we reach top of the blockchain, update
                 // the emission amount every minute.
                 boost::this_thread::sleep_for(boost::chrono::seconds(10));


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
MempoolStatus::is_thread_running()
{
    return is_running;
}

bf::path MempoolStatus::blockchain_path {"/home/mwo/.bitmonero/lmdb"};
string MempoolStatus::deamon_url {"http:://127.0.0.1:18081"};
bool   MempoolStatus::testnet {false};
atomic<bool>     MempoolStatus::is_running {false};
boost::thread      MempoolStatus::m_thread;
Blockchain*        MempoolStatus::core_storage {nullptr};
xmreg::MicroCore*  MempoolStatus::mcore {nullptr};

}
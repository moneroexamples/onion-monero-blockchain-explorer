
#include "src/MicroCore.h"

#include "src/page.h"

#include "ext/crow/crow.h"
#include "mstch/mstch.hpp"
#include "ext/format.h"

#include <fstream>

using boost::filesystem::path;

using namespace std;


namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}


int main() {

    path blockchain_path {"/home/mwo/.bitmonero/lmdb"};

     // enable basic monero log output
    xmreg::enable_monero_log();

    // create instance of our MicroCore
    xmreg::MicroCore mcore;
    cryptonote::Blockchain* core_storage;

    if (!xmreg::init_blockchain(blockchain_path.string(),
                               mcore, core_storage))
    {
        cerr << "Error accessing blockchain." << endl;
        return 1;
    }

    xmreg::page xmrblocks(&mcore, core_storage);


    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
            ([&]() {
                return xmrblocks.index();
            });

    app.port(8080).multithreaded().run();

    return 0;
}
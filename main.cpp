
#include "src/MicroCore.h"

#include "ext/crow/crow.h"
#include "mstch/mstch.hpp"
#include "ext/format.h"

#include <iostream>

using boost::filesystem::path;

using namespace std;


namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}



int main() {

    path blockchain_path {"/home/mwo/.bitmonero/lmdb"};

    fmt::print("Blockchain path      : {}\n", blockchain_path);

     // enable basic monero log output
    xmreg::enable_monero_log();

    // create instance of our MicroCore
    xmreg::MicroCore mcore;

    // initialize the core using the blockchain path
    if (!mcore.init(blockchain_path.string()))
    {
        cerr << "Error accessing blockchain." << endl;
        return 1;
    }

    // get the high level cryptonote::Blockchain object to interact
    // with the blockchain lmdb database
    cryptonote::Blockchain& core_storage = mcore.get_core();


    // get the current blockchain height. Just to check
    // if it reads ok.
    uint64_t height = core_storage.get_current_blockchain_height() - 1;

    fmt::print("\n\n"
          "Top block height      : {:d}\n", height);


    std::string view {"Blockchain height {{height}}"};

    mstch::map context {
                    {"height",  fmt::format("{:d}", height)}
    };

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
            ([&]() {
                return mstch::render(view, context);
            });

    app.port(8080).multithreaded().run();

    return 0;
}
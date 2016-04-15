
#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/page.h"

#include "ext/crow/crow.h"

#include <fstream>

using boost::filesystem::path;

using namespace std;

// needed for log system of momero
namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}


int main(int ac, const char* av[]) {

    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    auto help_opt = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return 0;
    }

    auto port_opt      = opts.get_option<string>("port");
    auto bc_path_opt   = opts.get_option<string>("bc-path");

    //cast port number in string to uint16
    uint16_t app_port = boost::lexical_cast<uint16_t>(*port_opt);


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

    // create instance of page class which
    // coins logic for the website
    xmreg::page xmrblocks(&mcore, core_storage);

    // crow instance
    crow::SimpleApp app;


    CROW_ROUTE(app, "/")
    ([&]() {
        return xmrblocks.index();
    });


    CROW_ROUTE(app, "/page/<uint>")
    ([&](size_t page_no) {
        return xmrblocks.index(page_no);
    });

    CROW_ROUTE(app, "/block/<uint>")
    ([&](size_t block_height) {
        return xmrblocks.show_block(block_height);
    });

    CROW_ROUTE(app, "/block/<string>")
            ([&](string block_hash) {
                return xmrblocks.show_block(block_hash);
            });

    CROW_ROUTE(app, "/autorefresh")
    ([&]() {
        uint64_t page_no {0};
        bool refresh_page {true};
        return xmrblocks.index(page_no, refresh_page);
    });

    CROW_ROUTE(app, "/css/style.css")
    ([&]() {
        return xmreg::read("./templates/css/style.css");
    });

    // run the crow http server
    app.port(app_port).multithreaded().run();

    return 0;
}

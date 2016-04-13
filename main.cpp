
#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/page.h"

#include "ext/crow/crow.h"

#include <fstream>

using boost::filesystem::path;

using namespace std;


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


    uint16_t app_port = boost::lexical_cast<uint16_t>(*port_opt);


    path blockchain_path {"/home/mwo/.bitmonero/lmdb"};


    // change timezone to Universtal time zone
    char old_tz[128];
    const char *tz_org = getenv("TZ");

    if (tz_org)
    {
        strcpy(old_tz, tz_org);
    }

    // set new timezone
    std::string tz = "TZ=Coordinated Universal Time";
    putenv(const_cast<char *>(tz.c_str()));
    tzset(); // Initialize timezone data


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

    CROW_ROUTE(app, "/autorefresh")
    ([&]() {
        bool refresh_page {true};
        return xmrblocks.index(refresh_page);
    });

    CROW_ROUTE(app, "/css/style.css")
    ([&]() {
        return xmreg::read("./templates/css/style.css");
    });


    app.port(app_port).multithreaded().run();

    // set timezone to orginal value
    if (tz_org != 0)
    {
        setenv("TZ", old_tz, 1);
        tzset();
    }

    return 0;
}
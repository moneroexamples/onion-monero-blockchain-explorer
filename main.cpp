#define CROW_ENABLE_SSL

#include "ext/crow/crow.h"

#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/page.h"


#include "ext/member_checker.h"

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

    auto help_opt          = opts.get_option<bool>("help");
    auto testnet_opt       = opts.get_option<bool>("testnet");
    auto enable_pusher_opt = opts.get_option<bool>("enable-pusher");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return EXIT_SUCCESS;
    }

    bool testnet       {*testnet_opt};
    bool enable_pusher {*enable_pusher_opt};

    auto port_opt           = opts.get_option<string>("port");
    auto bc_path_opt        = opts.get_option<string>("bc-path");
    auto custom_db_path_opt = opts.get_option<string>("custom-db-path");
    auto deamon_url_opt     = opts.get_option<string>("deamon-url");
    auto ssl_crt_file_opt   = opts.get_option<string>("ssl-crt-file");
    auto ssl_key_file_opt   = opts.get_option<string>("ssl-key-file");


    //cast port number in string to uint16
    uint16_t app_port = boost::lexical_cast<uint16_t>(*port_opt);


    bool use_ssl {false};

    string ssl_crt_file;
    string ssl_key_file;

    // check if ssl enabled and files exist

    if (ssl_crt_file_opt and ssl_key_file_opt)
    {
        if (!boost::filesystem::exists(boost::filesystem::path(*ssl_crt_file_opt)))
        {
            cerr << "ssl_crt_file path: " << *ssl_crt_file_opt
                 << "does not exist!" << endl;

            return EXIT_FAILURE;
        }

        if (!boost::filesystem::exists(boost::filesystem::path(*ssl_key_file_opt)))
        {
            cerr << "ssl_key_file path: " << *ssl_key_file_opt
                 << "does not exist!" << endl;

            return EXIT_FAILURE;
        }

        ssl_crt_file = *ssl_crt_file_opt;
        ssl_key_file = *ssl_key_file_opt;

        use_ssl = true;
    }



    // get blockchain path
    path blockchain_path;

    if (!xmreg::get_blockchain_path(bc_path_opt, blockchain_path, testnet))
    {
        cerr << "Error getting blockchain path." << endl;
        return EXIT_FAILURE;
    }

    cout << blockchain_path << endl;

     // enable basic monero log output
    xmreg::enable_monero_log();

    // create instance of our MicroCore
    // and make pointer to the Blockchain
    xmreg::MicroCore mcore;
    cryptonote::Blockchain* core_storage;

    // initialize mcore and core_storage
    if (!xmreg::init_blockchain(blockchain_path.string(),
                               mcore, core_storage))
    {
        cerr << "Error accessing blockchain." << endl;
        return EXIT_FAILURE;
    }

    // check if we have path to lmdb2 (i.e., custom db)
    // and if it exists

    string custom_db_path_str;

    if (custom_db_path_opt)
    {
        if (boost::filesystem::exists(boost::filesystem::path(*custom_db_path_opt)))
        {
            custom_db_path_str = *custom_db_path_opt;
        }
        else
        {
            cerr << "Custom db path: " << *custom_db_path_opt
                 << "does not exist" << endl;

            return EXIT_FAILURE;
        }
    }
    else
    {
        // if not given assume it is located in ~./bitmonero/lmdb2 folder
        // or ~./bitmonero/testnet/lmdb2 for testnet network
        custom_db_path_str = blockchain_path.parent_path().string()
                             + string("/lmdb2");
    }

    custom_db_path_str = xmreg::remove_trailing_path_separator(custom_db_path_str);


    string deamon_url {*deamon_url_opt};

    if (testnet)
        deamon_url = "http:://127.0.0.1:28081";

    // create instance of page class which
    // contains logic for the website
    xmreg::page xmrblocks(&mcore,
                          core_storage,
                          deamon_url,
                          custom_db_path_str,
                          testnet,
                          enable_pusher);

    // crow instance
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([&](const crow::request& req) {
        return crow::response(xmrblocks.index2());
    });

    CROW_ROUTE(app, "/page/<uint>")
    ([&](size_t page_no) {
        return xmrblocks.index2(page_no);
    });

    CROW_ROUTE(app, "/block/<uint>")
    ([&](const crow::request& req, size_t block_height) {
        return crow::response(xmrblocks.show_block(block_height));
    });

    CROW_ROUTE(app, "/block/<string>")
    ([&](const crow::request& req, string block_hash) {
        return crow::response(xmrblocks.show_block(block_hash));
    });

    CROW_ROUTE(app, "/tx/<string>")
    ([&](const crow::request& req, string tx_hash) {
        return crow::response(xmrblocks.show_tx(tx_hash));
    });

    CROW_ROUTE(app, "/tx/<string>/<uint>")
    ([&](string tx_hash, uint with_ring_signatures) {
        return xmrblocks.show_tx(tx_hash, with_ring_signatures);
    });

    CROW_ROUTE(app, "/myoutputs").methods("GET"_method)
    ([&](const crow::request& req) {

        string tx_hash     = string(req.url_params.get("tx_hash"));
        string xmr_address = string(req.url_params.get("xmr_address"));
        string viewkey     = string(req.url_params.get("viewkey"));

        return xmrblocks.show_my_outputs(tx_hash, xmr_address, viewkey);
    });


    CROW_ROUTE(app, "/prove").methods("GET"_method)
    ([&](const crow::request& req) {

        string tx_hash     = string(req.url_params.get("txhash"));
        string tx_prv_key  = string(req.url_params.get("txprvkey"));
        string xmr_address = string(req.url_params.get("xmraddress"));

        return xmrblocks.show_prove(tx_hash, xmr_address, tx_prv_key);
    });

    CROW_ROUTE(app, "/rawtx")
    ([&](const crow::request& req) {
        return xmrblocks.show_rawtx();
    });

    CROW_ROUTE(app, "/checkandpush").methods("POST"_method)
    ([&](const crow::request& req) {

        map<std::string, std::string> post_body = xmreg::parse_crow_post_data(req.body);

        if (post_body.count("rawtxdata") == 0 || post_body.count("action") == 0)
        {
            return string("Raw tx data or action not provided");
        }

        string raw_tx_data = post_body["rawtxdata"];
        string action      = post_body["action"];

        if (action == "check")
            return xmrblocks.show_checkrawtx(raw_tx_data, action);
        else if (action == "push")
            return xmrblocks.show_pushrawtx(raw_tx_data, action);

    });

    CROW_ROUTE(app, "/rawkeyimgs")
    ([&](const crow::request& req) {
        return xmrblocks.show_rawkeyimgs();
    });

    CROW_ROUTE(app, "/checkrawkeyimgs").methods("POST"_method)
    ([&](const crow::request& req) {

        map<std::string, std::string> post_body = xmreg::parse_crow_post_data(req.body);

        if (post_body.count("rawkeyimgsdata") == 0)
        {
            return string("Raw key images data not given");
        }

        if (post_body.count("viewkey") == 0)
        {
            return string("Viewkey not provided. Cant decrypt key image file without it");
        }

        string raw_data = post_body["rawkeyimgsdata"];
        string viewkey  = post_body["viewkey"];

        return xmrblocks.show_checkrawkeyimgs(raw_data, viewkey);
    });


    CROW_ROUTE(app, "/rawoutputkeys")
    ([&](const crow::request& req) {
        return xmrblocks.show_rawoutputkeys();
    });

    CROW_ROUTE(app, "/checkrawoutputkeys").methods("POST"_method)
            ([&](const crow::request& req) {

                map<std::string, std::string> post_body = xmreg::parse_crow_post_data(req.body);

                if (post_body.count("rawoutputkeysdata") == 0)
                {
                    return string("Raw output keys data not given");
                }

                if (post_body.count("viewkey") == 0)
                {
                    return string("Viewkey not provided. Cant decrypt key image file without it");
                }

                string raw_data = post_body["rawoutputkeysdata"];
                string viewkey  = post_body["viewkey"];

                return xmrblocks.show_checkcheckrawoutput(raw_data, viewkey);
            });



    CROW_ROUTE(app, "/search").methods("GET"_method)
    ([&](const crow::request& req) {
        return xmrblocks.search(string(req.url_params.get("value")));
    });

    CROW_ROUTE(app, "/robots.txt")
    ([&]() {
        string text = "User-agent: *\n"
                      "Disallow: ";
        return text;
    });

    CROW_ROUTE(app, "/autorefresh")
    ([&]() {
        uint64_t page_no {0};
        bool refresh_page {true};
        return xmrblocks.index2(page_no, refresh_page);
    });

    // run the crow http server

    if (use_ssl)
    {
        cout << "Staring in ssl mode" << endl;
        app.port(app_port).ssl_file(ssl_crt_file, ssl_key_file).multithreaded().run();
    }
    else
    {
        cout << "Staring in non-ssl mode" << endl;
        app.port(app_port).multithreaded().run();
    }


    return EXIT_SUCCESS;
}

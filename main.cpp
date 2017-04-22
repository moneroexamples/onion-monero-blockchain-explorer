#define CROW_ENABLE_SSL

#include "ext/crow/crow.h"

#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/page.h"

#include <fstream>
#include <regex>

using boost::filesystem::path;

using namespace std;

int main(int ac, const char* av[]) {

    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    auto help_opt                      = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return EXIT_SUCCESS;
    }

    auto port_opt                      = opts.get_option<string>("port");
    auto bc_path_opt                   = opts.get_option<string>("bc-path");
    auto deamon_url_opt                = opts.get_option<string>("deamon-url");
    auto ssl_crt_file_opt              = opts.get_option<string>("ssl-crt-file");
    auto ssl_key_file_opt              = opts.get_option<string>("ssl-key-file");
    auto no_blocks_on_index_opt        = opts.get_option<string>("no-blocks-on-index");
    auto testnet_url                   = opts.get_option<string>("testnet-url");
    auto mainnet_url                   = opts.get_option<string>("mainnet-url");
    auto testnet_opt                   = opts.get_option<bool>("testnet");
    auto enable_key_image_checker_opt  = opts.get_option<bool>("enable-key-image-checker");
    auto enable_output_key_checker_opt = opts.get_option<bool>("enable-output-key-checker");
    auto enable_autorefresh_option_opt = opts.get_option<bool>("enable-autorefresh-option");
    auto enable_pusher_opt             = opts.get_option<bool>("enable-pusher");
    auto enable_mixin_details_opt      = opts.get_option<bool>("enable-mixin-details");
    auto enable_mempool_cache_opt      = opts.get_option<bool>("enable-mempool-cache");
    auto enable_tx_cache_opt           = opts.get_option<bool>("enable-tx-cache");
    auto enable_block_cache_opt        = opts.get_option<bool>("enable-block-cache");
    auto show_cache_times_opt          = opts.get_option<bool>("show-cache-times");

    bool testnet                      {*testnet_opt};
    bool enable_pusher                {*enable_pusher_opt};
    bool enable_key_image_checker     {*enable_key_image_checker_opt};
    bool enable_autorefresh_option    {*enable_autorefresh_option_opt};
    bool enable_output_key_checker    {*enable_output_key_checker_opt};
    bool enable_mixin_details         {*enable_mixin_details_opt};
    bool enable_mempool_cache         {*enable_mempool_cache_opt};
    bool enable_tx_cache              {*enable_tx_cache_opt};
    bool enable_block_cache           {*enable_block_cache_opt};
    bool show_cache_times             {*show_cache_times_opt};


    // set  monero log output level
    uint32_t log_level = 0;
    mlog_configure("", true);

    //cast port number in string to uint
    uint16_t app_port = boost::lexical_cast<uint16_t>(*port_opt);

    // cast no_blocks_on_index_opt to uint
    uint64_t no_blocks_on_index = boost::lexical_cast<uint64_t>(*no_blocks_on_index_opt);

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

    string deamon_url {*deamon_url_opt};

    if (testnet && deamon_url == "http:://127.0.0.1:18081")
        deamon_url = "http:://127.0.0.1:28081";

    // create instance of page class which
    // contains logic for the website
    xmreg::page xmrblocks(&mcore,
                          core_storage,
                          deamon_url,
                          testnet,
                          enable_pusher,
                          enable_key_image_checker,
                          enable_output_key_checker,
                          enable_autorefresh_option,
                          enable_mixin_details,
                          enable_mempool_cache,
                          enable_tx_cache,
                          enable_block_cache,
                          show_cache_times,
                          no_blocks_on_index,
                          *testnet_url,
                          *mainnet_url);

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
    ([&](string tx_hash, uint16_t with_ring_signatures) {
        return xmrblocks.show_tx(tx_hash, with_ring_signatures);
    });

    CROW_ROUTE(app, "/myoutputs").methods("POST"_method)
    ([&](const crow::request& req) {

        map<std::string, std::string> post_body
                = xmreg::parse_crow_post_data(req.body);

        if (post_body.count("xmr_address") == 0
            || post_body.count("viewkey") == 0
            || post_body.count("tx_hash") == 0)
        {
            return string("xmr address, viewkey or tx hash not provided");
        }

        string tx_hash     = post_body["tx_hash"];
        string xmr_address = post_body["xmr_address"];
        string viewkey     = post_body["viewkey"];

        // this will be only not empty when checking raw tx data
        // using tx pusher
        string raw_tx_data = post_body["raw_tx_data"];

        return xmrblocks.show_my_outputs(tx_hash, xmr_address,
                                         viewkey, raw_tx_data);
    });

    CROW_ROUTE(app, "/prove").methods("POST"_method)
        ([&](const crow::request& req) {

            map<std::string, std::string> post_body
                    = xmreg::parse_crow_post_data(req.body);

            if (post_body.count("xmraddress") == 0
                || post_body.count("txprvkey") == 0
                || post_body.count("txhash") == 0)
            {
                return string("xmr address, tx private key or "
                                      "tx hash not provided");
            }

            string tx_hash     = post_body["txhash"];;
            string tx_prv_key  = post_body["txprvkey"];;
            string xmr_address = post_body["xmraddress"];;

            return xmrblocks.show_prove(tx_hash, xmr_address, tx_prv_key);
    });

    if (enable_pusher)
    {
        CROW_ROUTE(app, "/rawtx")
        ([&](const crow::request& req) {
            return xmrblocks.show_rawtx();
        });

        CROW_ROUTE(app, "/checkandpush").methods("POST"_method)
        ([&](const crow::request& req) {

            map<std::string, std::string> post_body
                    = xmreg::parse_crow_post_data(req.body);

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
    }

    if (enable_key_image_checker)
    {
        CROW_ROUTE(app, "/rawkeyimgs")
        ([&](const crow::request& req) {
            return xmrblocks.show_rawkeyimgs();
        });

        CROW_ROUTE(app, "/checkrawkeyimgs").methods("POST"_method)
        ([&](const crow::request& req) {

            map<std::string, std::string> post_body
                    = xmreg::parse_crow_post_data(req.body);

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
    }


    if (enable_output_key_checker)
    {
        CROW_ROUTE(app, "/rawoutputkeys")
        ([&](const crow::request& req) {
            return xmrblocks.show_rawoutputkeys();
        });

        CROW_ROUTE(app, "/checkrawoutputkeys").methods("POST"_method)
        ([&](const crow::request& req) {

            map<std::string, std::string> post_body
                    = xmreg::parse_crow_post_data(req.body);

            if (post_body.count("rawoutputkeysdata") == 0)
            {
                return string("Raw output keys data not given");
            }

            if (post_body.count("viewkey") == 0)
            {
                return string("Viewkey not provided. Cant decrypt "
                                      "key image file without it");
            }

            string raw_data = post_body["rawoutputkeysdata"];
            string viewkey  = post_body["viewkey"];

            return xmrblocks.show_checkcheckrawoutput(raw_data, viewkey);
        });
    }


    CROW_ROUTE(app, "/search").methods("GET"_method)
    ([&](const crow::request& req) {
        return xmrblocks.search(string(req.url_params.get("value")));
    });

    CROW_ROUTE(app, "/mempool")
    ([&](const crow::request& req) {
        return xmrblocks.mempool(true);
    });

    CROW_ROUTE(app, "/robots.txt")
    ([&]() {
        string text = "User-agent: *\n"
                      "Disallow: ";
        return text;
    });

    CROW_ROUTE(app, "/api/transaction/<string>")
    ([&](const crow::request& req, string tx_hash) {

        crow::response r {xmrblocks.json_transaction(tx_hash).dump()};
        r.set_header("Content-Type", "application/json");

        return r;
    });

    CROW_ROUTE(app, "/api/block/<string>")
    ([&](const crow::request& req, string block_no_or_hash) {

        crow::response r {xmrblocks.json_block(block_no_or_hash).dump()};
        r.set_header("Content-Type", "application/json");

        return r;
    });


    CROW_ROUTE(app, "/api/transactions").methods("GET"_method)
    ([&](const crow::request& req) {

        string page  = regex_search(req.raw_url, regex {"page=\\d+"}) ?
                       req.url_params.get("page") : "0";

        string limit = regex_search(req.raw_url, regex {"limit=\\d+"}) ?
                       req.url_params.get("limit") : "25";

        crow::response r {xmrblocks.json_transactions(page, limit).dump()};
        r.set_header("Content-Type", "application/json");

        return r;
    });

    CROW_ROUTE(app, "/api/mempool")
    ([&](const crow::request& req) {

        crow::response r {xmrblocks.json_mempool().dump()};
        r.set_header("Content-Type", "application/json");

        return r;
    });

    CROW_ROUTE(app, "/api/search/<string>")
    ([&](const crow::request& req, string search_value) {

        crow::response r {xmrblocks.json_search(search_value).dump()};
        r.set_header("Content-Type", "application/json");

        return r;
    });

    CROW_ROUTE(app, "/api/outputs").methods("GET"_method)
    ([&](const crow::request& req) {

        string tx_hash = regex_search(req.raw_url, regex {"txhash=\\w+"}) ?
                         req.url_params.get("txhash") : "";

        string address  = regex_search(req.raw_url, regex {"address=\\w+"}) ?
                       req.url_params.get("address") : "";

        string viewkey = regex_search(req.raw_url, regex {"viewkey=\\w+"}) ?
                       req.url_params.get("viewkey") : "";

        bool tx_prove = regex_search(req.raw_url, regex {"txprove=[01]"}) ?
                        boost::lexical_cast<bool>(req.url_params.get("txprove")) :
                        false;

        crow::response r {xmrblocks.json_outputs(
                tx_hash, address, viewkey, tx_prove).dump()};

        r.set_header("Content-Type", "application/json");

        return r;
    });


    if (enable_autorefresh_option)
    {
        CROW_ROUTE(app, "/autorefresh")
        ([&]() {
            uint64_t page_no {0};
            bool refresh_page {true};
            return xmrblocks.index2(page_no, refresh_page);
        });
    }

    // run the crow http server

    if (use_ssl)
    {
        cout << "Staring in ssl mode" << endl;
        app.port(app_port).ssl_file(ssl_crt_file, ssl_key_file)
                .multithreaded().run();
    }
    else
    {
        cout << "Staring in non-ssl mode" << endl;
        app.port(app_port).multithreaded().run();
    }


    return EXIT_SUCCESS;
}

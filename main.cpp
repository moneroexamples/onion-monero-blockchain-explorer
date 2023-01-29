#define CROW_ENABLE_SSL
#define CROW_MAIN

#include "src/page.h"

#include "ext/crow_all.h"
#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"

#include <fstream>
#include <regex>

using boost::filesystem::path;
using xmreg::remove_bad_chars;

using namespace std;

namespace myxmr
{
struct htmlresponse: public crow::response
{
    htmlresponse(string&& _body)
            : crow::response {std::move(_body)}
    {
        add_header("Content-Type", "text/html; charset=utf-8");
    }
};

struct jsonresponse: public crow::response
{
    jsonresponse(const nlohmann::json& _body)
            : crow::response {_body.dump()}
    {
        add_header("Access-Control-Allow-Origin", "*");
        add_header("Access-Control-Allow-Headers", "Content-Type");
        add_header("Content-Type", "application/json");
    }
};
}

int
main(int ac, const char* av[])
{

    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    auto help_opt                      = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return EXIT_SUCCESS;
    }

    auto port_opt                      = opts.get_option<string>("port");
    auto bindaddr_opt                  = opts.get_option<string>("bindaddr");
    auto bc_path_opt                   = opts.get_option<string>("bc-path");
    auto daemon_url_opt                = opts.get_option<string>("daemon-url");
    auto ssl_crt_file_opt              = opts.get_option<string>("ssl-crt-file");
    auto ssl_key_file_opt              = opts.get_option<string>("ssl-key-file");
    auto no_blocks_on_index_opt        = opts.get_option<string>("no-blocks-on-index");
    auto testnet_url                   = opts.get_option<string>("testnet-url");
    auto stagenet_url                  = opts.get_option<string>("stagenet-url");
    auto mainnet_url                   = opts.get_option<string>("mainnet-url");
    auto mempool_info_timeout_opt      = opts.get_option<string>("mempool-info-timeout");
    auto mempool_refresh_time_opt      = opts.get_option<string>("mempool-refresh-time");
    auto daemon_login_opt              = opts.get_option<string>("daemon-login");
    auto testnet_opt                   = opts.get_option<bool>("testnet");
    auto stagenet_opt                  = opts.get_option<bool>("stagenet");
    auto enable_key_image_checker_opt  = opts.get_option<bool>("enable-key-image-checker");
    auto enable_output_key_checker_opt = opts.get_option<bool>("enable-output-key-checker");
    auto enable_autorefresh_option_opt = opts.get_option<bool>("enable-autorefresh-option");
    auto enable_pusher_opt             = opts.get_option<bool>("enable-pusher");
    auto enable_randomx_opt            = opts.get_option<bool>("enable-randomx");
    auto enable_mixin_details_opt      = opts.get_option<bool>("enable-mixin-details");
    auto enable_json_api_opt           = opts.get_option<bool>("enable-json-api");
    auto enable_as_hex_opt             = opts.get_option<bool>("enable-as-hex");
    auto enable_mixin_guess_opt        = opts.get_option<bool>("enable-mixin-guess");
    auto concurrency_opt               = opts.get_option<size_t>("concurrency");
    auto enable_emission_monitor_opt   = opts.get_option<bool>("enable-emission-monitor");


    bool testnet                      {*testnet_opt};
    bool stagenet                     {*stagenet_opt};

    if (testnet && stagenet)
    {
        cerr << "testnet and stagenet cannot be specified at the same time!" << endl;
        return EXIT_FAILURE;
    }

    const cryptonote::network_type nettype = testnet ?
        cryptonote::network_type::TESTNET : stagenet ?
        cryptonote::network_type::STAGENET : cryptonote::network_type::MAINNET;

    bool enable_pusher                {*enable_pusher_opt};
    bool enable_randomx               {*enable_randomx_opt};
    bool enable_key_image_checker     {*enable_key_image_checker_opt};
    bool enable_autorefresh_option    {*enable_autorefresh_option_opt};
    bool enable_output_key_checker    {*enable_output_key_checker_opt};
    bool enable_mixin_details         {*enable_mixin_details_opt};
    bool enable_mixin_guess           {*enable_mixin_guess_opt};
    bool enable_json_api              {*enable_json_api_opt};
    bool enable_as_hex                {*enable_as_hex_opt};
    bool enable_emission_monitor      {*enable_emission_monitor_opt};

    //temprorary disable randomx
    if (enable_randomx == true) {
        cout << "Support for randomx code is disabled due to issues with it"<< endl;
        enable_randomx = false;
    }

    // set  monero log output level
    uint32_t log_level = 0;
    mlog_configure("", true);

    (void) log_level;

    //cast port number in string to uint
    uint16_t app_port = boost::lexical_cast<uint16_t>(*port_opt);

    string bindaddr = *bindaddr_opt;

    // cast no_blocks_on_index_opt to uint
    uint64_t no_blocks_on_index = boost::lexical_cast<uint64_t>(*no_blocks_on_index_opt);

    bool use_ssl {false};

    string ssl_crt_file;
    string ssl_key_file;

    xmreg::rpccalls::login_opt daemon_rpc_login {};


    if (daemon_login_opt)
    {

       string user {};
       epee::wipeable_string pass {};

       string daemon_login = *daemon_login_opt;

       size_t colon_location = daemon_login.find_first_of(':');

       if (colon_location != std::string::npos)
       {
           // have colon for user:password
           user = daemon_login.substr(0, colon_location);
           pass  = daemon_login.substr(colon_location + 1);
       }
       else
       {
          user = *daemon_login_opt;
       }

       daemon_rpc_login = epee::net_utils::http::login {user, pass};

       //cout << "colon_location: " << colon_location << endl;
       // cout << "user: " << user << endl;
       // cout << "pass: " << std::string(pass.data(), pass.size()) << endl;
    }


    // check if ssl enabled and files exist

    if (ssl_crt_file_opt && ssl_key_file_opt)
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

    if (!xmreg::get_blockchain_path(bc_path_opt, blockchain_path, nettype))
    {
        cerr << "Error getting blockchain path." << endl;
        return EXIT_FAILURE;
    }

    // create instance of our MicroCore
    // and make pointer to the Blockchain
    xmreg::MicroCore mcore;
    cryptonote::Blockchain* core_storage;

    // initialize mcore and core_storage
    if (!xmreg::init_blockchain(blockchain_path.string(),
                               mcore, core_storage, nettype))
    {
        cerr << "Error accessing blockchain." << endl;
        return EXIT_FAILURE;
    }

    string daemon_url {*daemon_url_opt};

    if (testnet && daemon_url == "127.0.0.1:18081")
        daemon_url = "127.0.0.1:28081";
    if (stagenet && daemon_url == "127.0.0.1:18081")
        daemon_url = "127.0.0.1:38081";
        
    uint64_t mempool_info_timeout {5000};

    try
    {
        mempool_info_timeout = boost::lexical_cast<uint64_t>(
                *mempool_info_timeout_opt);
    }
    catch (boost::bad_lexical_cast &e)
    {
        cout << "Cant cast " << (*mempool_info_timeout_opt) 
             <<" into numbers. Using default values.\n";
    }

    uint64_t mempool_refresh_time {10};


    if (enable_emission_monitor == true)
    {
        // This starts new thread, which aim is
        // to calculate, store and monitor
        // current total Monero emission amount.

        // This thread stores the current emission
        // which it has caluclated in
        // <blockchain_path>/emission_amount.txt file,
        // e.g., ~/.bitmonero/lmdb/emission_amount.txt.
        // So instead of calcualting the emission
        // from scrach whenever the explorer is started,
        // the thread is initalized with the values
        // found in emission_amount.txt file.

        xmreg::CurrentBlockchainStatus::blockchain_path
                = blockchain_path;
        xmreg::CurrentBlockchainStatus::nettype
                = nettype;
        xmreg::CurrentBlockchainStatus::daemon_url
                = daemon_url;
        xmreg::CurrentBlockchainStatus::set_blockchain_variables(
                &mcore, core_storage);

        // launch the status monitoring thread so that it keeps track of blockchain
        // info, e.g., current height. Information from this thread is used
        // by tx searching threads that are launched for each user independently,
        // when they log back or create new account.
        xmreg::CurrentBlockchainStatus::start_monitor_blockchain_thread();
    }


    xmreg::MempoolStatus::blockchain_path
            = blockchain_path;
    xmreg::MempoolStatus::nettype
            = nettype;
    xmreg::MempoolStatus::daemon_url
            = daemon_url;
    xmreg::MempoolStatus::login
            = daemon_rpc_login;
    xmreg::MempoolStatus::set_blockchain_variables(
            &mcore, core_storage);

    xmreg::MempoolStatus::network_info initial_info;
    strcpy(initial_info.block_size_limit_str, "0.0");
    strcpy(initial_info.block_size_median_str, "0.0");
    xmreg::MempoolStatus::current_network_info = initial_info;

    try
    {
        mempool_refresh_time = boost::lexical_cast<uint64_t>(*mempool_refresh_time_opt);

    }
    catch (boost::bad_lexical_cast &e)
    {
        cout << "Cant cast " << (*mempool_refresh_time_opt)
             <<" into number. Using default value."
             << endl;
    }

    // launch the status monitoring thread so that it keeps track of blockchain
    // info, e.g., current height. Information from this thread is used
    // by tx searching threads that are launched for each user independently,
    // when they log back or create new account.
    xmreg::MempoolStatus::mempool_refresh_time = mempool_refresh_time;
    xmreg::MempoolStatus::start_mempool_status_thread();

    // create instance of page class which
    // contains logic for the website
    xmreg::page xmrblocks(&mcore,
                          core_storage,
                          daemon_url,
                          nettype,
                          enable_pusher,
                          enable_randomx,
                          enable_as_hex,
                          enable_key_image_checker,
                          enable_output_key_checker,
                          enable_autorefresh_option,
                          enable_mixin_details,
                          enable_mixin_guess,
                          no_blocks_on_index,
                          mempool_info_timeout,
                          *testnet_url,
                          *stagenet_url,
                          *mainnet_url,
                          daemon_rpc_login);

    // crow instance
    crow::SimpleApp app;

    // get domian url based on the request
    auto get_domain = [&use_ssl](crow::request const& req) {
        return (use_ssl ? "https://" : "http://")
               + req.get_header_value("Host");
    };

    CROW_ROUTE(app, "/")
    ([&]() {
        return myxmr::htmlresponse(xmrblocks.index2());
    });

    CROW_ROUTE(app, "/page/<uint>")
    ([&](size_t page_no) {
        return myxmr::htmlresponse(xmrblocks.index2(page_no));
    });

    CROW_ROUTE(app, "/block/<uint>")
    ([&](size_t block_height) {
        return myxmr::htmlresponse(xmrblocks.show_block(block_height));
    });
    
    CROW_ROUTE(app, "/randomx/<uint>")
    ([&](size_t block_height) {
        return myxmr::htmlresponse(xmrblocks.show_randomx(block_height));
    });

    CROW_ROUTE(app, "/block/<string>")
    ([&](string block_hash) {
        return myxmr::htmlresponse(
                xmrblocks.show_block(remove_bad_chars(block_hash)));
    });

    CROW_ROUTE(app, "/tx/<string>")
    ([&](string tx_hash) {
        return myxmr::htmlresponse(
                xmrblocks.show_tx(remove_bad_chars(tx_hash)));
    });
    if (enable_autorefresh_option)
    {
        CROW_ROUTE(app, "/tx/<string>/autorefresh")
        ([&](string tx_hash) {
            bool refresh_page {true};
            uint16_t with_ring_signatures {0};
            return myxmr::htmlresponse(
                xmrblocks.show_tx(remove_bad_chars(tx_hash), with_ring_signatures, refresh_page));
        });
    }

    if (enable_as_hex)
    {
        CROW_ROUTE(app, "/txhex/<string>")
        ([&](string tx_hash) {
            return crow::response(
                    xmrblocks.show_tx_hex(remove_bad_chars(tx_hash)));
        });

        CROW_ROUTE(app, "/ringmembershex/<string>")
        ([&](string tx_hash) {
            return crow::response(
                    xmrblocks.show_ringmembers_hex(remove_bad_chars(tx_hash)));
        });

        CROW_ROUTE(app, "/blockhex/<uint>")
        ([&](size_t block_height) {
            return crow::response(
                    xmrblocks.show_block_hex(block_height, false));
        });

        CROW_ROUTE(app, "/blockhexcomplete/<uint>")
        ([&](size_t block_height) {
            return crow::response(
                    xmrblocks.show_block_hex(block_height, true));
        });

//        CROW_ROUTE(app, "/ringmemberstxhex/<string>")
//        ([&](string tx_hash) {
//            return crow::response(
//              xmrblocks.show_ringmemberstx_hex(remove_bad_chars(tx_hash)));
//        });

        CROW_ROUTE(app, "/ringmemberstxhex/<string>")
        ([&](string tx_hash) {
            return myxmr::jsonresponse {
                xmrblocks.show_ringmemberstx_jsonhex(
                        remove_bad_chars(tx_hash))};
        });

    }

    CROW_ROUTE(app, "/tx/<string>/<uint>")
    ([&](string tx_hash, uint16_t with_ring_signatures)
     {
        return myxmr::htmlresponse(
                xmrblocks.show_tx(remove_bad_chars(tx_hash), 
                    with_ring_signatures));
    });
    if (enable_autorefresh_option)
    {
        CROW_ROUTE(app, "/tx/<string>/<uint>/autorefresh")
        ([&](string tx_hash, uint16_t with_ring_signature) {
            bool refresh_page {true};
            return myxmr::htmlresponse(
                xmrblocks.show_tx(remove_bad_chars(tx_hash), with_ring_signature, refresh_page));
        });
    }

    CROW_ROUTE(app, "/myoutputs").methods("POST"_method)
    ([&](const crow::request& req) -> myxmr::htmlresponse
     {

        map<std::string, std::string> post_body
                = xmreg::parse_crow_post_data(req.body);

        if (post_body.count("xmr_address") == 0
            || post_body.count("viewkey") == 0
            || post_body.count("tx_hash") == 0)
        {
            return string("xmr address, viewkey or tx hash not provided");
        }

        string tx_hash     = remove_bad_chars(post_body["tx_hash"]);
        string xmr_address = remove_bad_chars(post_body["xmr_address"]);
        string viewkey     = remove_bad_chars(post_body["viewkey"]);

        // this will be only not empty when checking raw tx data
        // using tx pusher
        string raw_tx_data = remove_bad_chars(post_body["raw_tx_data"]);

        string domain      =  get_domain(req);

        string response = xmrblocks.show_my_outputs(
                                         tx_hash, xmr_address,
                                         viewkey, raw_tx_data,
                                         domain);

        return myxmr::htmlresponse(std::move(response));
    });

    CROW_ROUTE(app, "/myoutputs/<string>/<string>/<string>")
    ([&](const crow::request& req, string tx_hash,
        string xmr_address, string viewkey)
     {

        string domain = get_domain(req);

        return myxmr::htmlresponse(xmrblocks.show_my_outputs(
                                         remove_bad_chars(tx_hash),
                                         remove_bad_chars(xmr_address),
                                         remove_bad_chars(viewkey),
                                         string {},
                                         domain));
    });

    CROW_ROUTE(app, "/prove").methods("POST"_method)
        ([&](const crow::request& req) -> myxmr::htmlresponse 
         {

            map<std::string, std::string> post_body
                    = xmreg::parse_crow_post_data(req.body);

            if (post_body.count("xmraddress") == 0
                || post_body.count("txprvkey") == 0
                || post_body.count("txhash") == 0)
            {
                return string("xmr address, tx private key or "
                                      "tx hash not provided");
            }

            string tx_hash     = remove_bad_chars(post_body["txhash"]);
            string tx_prv_key  = remove_bad_chars(post_body["txprvkey"]);
            string xmr_address = remove_bad_chars(post_body["xmraddress"]);

            // this will be only not empty when checking raw tx data
            // using tx pusher
            string raw_tx_data = remove_bad_chars(post_body["raw_tx_data"]);

            string domain      = get_domain(req);

            return myxmr::htmlresponse(xmrblocks.show_prove(tx_hash,
                                        xmr_address,
                                        tx_prv_key,
                                        raw_tx_data,
                                        domain));
    });


    CROW_ROUTE(app, "/prove/<string>/<string>/<string>")
    ([&](const crow::request& req, string tx_hash,
         string xmr_address, string tx_prv_key) 
     {

        string domain = get_domain(req);

        return myxmr::htmlresponse(xmrblocks.show_prove(
                                    remove_bad_chars(tx_hash),
                                    remove_bad_chars(xmr_address),
                                    remove_bad_chars(tx_prv_key),
                                    string {},
                                    domain));
    });

    if (enable_pusher)
    {
        CROW_ROUTE(app, "/rawtx")
        ([&]() {
            return myxmr::htmlresponse(xmrblocks.show_rawtx());
        });

        CROW_ROUTE(app, "/checkandpush").methods("POST"_method)
        ([&](const crow::request& req) -> myxmr::htmlresponse
         {

            map<std::string, std::string> post_body
                    = xmreg::parse_crow_post_data(req.body);

            if (post_body.count("rawtxdata") == 0 
                    || post_body.count("action") == 0)
            {
                return string("Raw tx data or action not provided");
            }

            string raw_tx_data = remove_bad_chars(post_body["rawtxdata"]);
            string action      = remove_bad_chars(post_body["action"]);

            if (action == "check")
                return myxmr::htmlresponse(
                        xmrblocks.show_checkrawtx(raw_tx_data, action));
            else if (action == "push")
                return myxmr::htmlresponse(
                        xmrblocks.show_pushrawtx(raw_tx_data, action));
            return string("Provided action is neither check nor push");

        });
    }

    if (enable_key_image_checker)
    {
        CROW_ROUTE(app, "/rawkeyimgs")
        ([&]() {
            return myxmr::htmlresponse(xmrblocks.show_rawkeyimgs());
        });

        CROW_ROUTE(app, "/checkrawkeyimgs").methods("POST"_method)
        ([&](const crow::request& req) -> myxmr::htmlresponse
         {

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

            string raw_data = remove_bad_chars(post_body["rawkeyimgsdata"]);
            string viewkey  = remove_bad_chars(post_body["viewkey"]);

            return myxmr::htmlresponse(
                    xmrblocks.show_checkrawkeyimgs(raw_data, viewkey));
        });
    }


    if (enable_output_key_checker)
    {
        CROW_ROUTE(app, "/rawoutputkeys")
        ([&]() {
            return myxmr::htmlresponse(xmrblocks.show_rawoutputkeys());
        });

        CROW_ROUTE(app, "/checkrawoutputkeys").methods("POST"_method)
        ([&](const crow::request& req) -> myxmr::htmlresponse
         {

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

            string raw_data = remove_bad_chars(post_body["rawoutputkeysdata"]);
            string viewkey  = remove_bad_chars(post_body["viewkey"]);

            return myxmr::htmlresponse(
                    xmrblocks.show_checkcheckrawoutput(raw_data, viewkey));
        });
    }


    CROW_ROUTE(app, "/search").methods("GET"_method)
    ([&](const crow::request& req) {
        return myxmr::htmlresponse(
                xmrblocks.search(
                    remove_bad_chars(
                        string(req.url_params.get("value")))));
    });

    CROW_ROUTE(app, "/mempool")
    ([&]() {
        return myxmr::htmlresponse(xmrblocks.mempool(true));
    });

    // alias to  "/mempool"
    CROW_ROUTE(app, "/txpool")
    ([&]() {
        return myxmr::htmlresponse(xmrblocks.mempool(true));
    });

//    CROW_ROUTE(app, "/altblocks")
//    ([&](const crow::request& req) {
//        return xmrblocks.altblocks();
//    });

    CROW_ROUTE(app, "/robots.txt")
    ([&]() {
        string text = "User-agent: *\n"
                      "Disallow: ";
        return text;
    });

    if (enable_json_api)
    {

        cout << "Enable JSON API\n";

        CROW_ROUTE(app, "/api/transaction/<string>")
        ([&](string tx_hash) {

            myxmr::jsonresponse r{xmrblocks.json_transaction(remove_bad_chars(tx_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/rawtransaction/<string>")
        ([&](string tx_hash) {

            myxmr::jsonresponse r{xmrblocks.json_rawtransaction(remove_bad_chars(tx_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/detailedtransaction/<string>")
        ([&](string tx_hash) {

            myxmr::jsonresponse r{xmrblocks.json_detailedtransaction(remove_bad_chars(tx_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/block/<string>")
        ([&](string block_no_or_hash) {

            myxmr::jsonresponse r{xmrblocks.json_block(remove_bad_chars(block_no_or_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/rawblock/<string>")
        ([&](string block_no_or_hash) {

            myxmr::jsonresponse r{xmrblocks.json_rawblock(remove_bad_chars(block_no_or_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/transactions").methods("GET"_method)
        ([&](const crow::request &req) {

            string page = regex_search(req.raw_url, regex {"page=\\d+"}) ?
                          req.url_params.get("page") : "0";

            string limit = regex_search(req.raw_url, regex {"limit=\\d+"}) ?
                           req.url_params.get("limit") : "25";

            myxmr::jsonresponse r{xmrblocks.json_transactions(
                    remove_bad_chars(page), remove_bad_chars(limit))};

            return r;
        });

        CROW_ROUTE(app, "/api/mempool").methods("GET"_method)
        ([&](const crow::request &req) {

            string page = regex_search(req.raw_url, regex {"page=\\d+"}) ?
                          req.url_params.get("page") : "0";

            // default value for limit is some large number, so that
            // a call to api/mempool without any limit return all
            // mempool txs
            string limit = regex_search(req.raw_url, regex {"limit=\\d+"}) ?
                           req.url_params.get("limit") : "100000000";

            myxmr::jsonresponse r{xmrblocks.json_mempool(
                    remove_bad_chars(page), remove_bad_chars(limit))};

            return r;
        });

        CROW_ROUTE(app, "/api/search/<string>")
        ([&](string search_value) {

            myxmr::jsonresponse r{xmrblocks.json_search(remove_bad_chars(search_value))};

            return r;
        });

        CROW_ROUTE(app, "/api/networkinfo")
        ([&]() {

            myxmr::jsonresponse r{xmrblocks.json_networkinfo()};

            return r;
        });

        CROW_ROUTE(app, "/api/emission")
        ([&]() {

            myxmr::jsonresponse r{xmrblocks.json_emission()};

            return r;
        });

        CROW_ROUTE(app, "/api/outputs").methods("GET"_method)
        ([&](const crow::request &req) {

            string tx_hash = regex_search(req.raw_url, regex {"txhash=\\w+"}) ?
                             req.url_params.get("txhash") : "";

            string address = regex_search(req.raw_url, regex {"address=\\w+"}) ?
                             req.url_params.get("address") : "";

            string viewkey = regex_search(req.raw_url, regex {"viewkey=\\w+"}) ?
                             req.url_params.get("viewkey") : "";

            bool tx_prove{false};

            try
            {
                tx_prove = regex_search(req.raw_url, regex {"txprove=[01]"}) ?
                           boost::lexical_cast<bool>(req.url_params.get("txprove")) :
                           false;
            }
            catch (const boost::bad_lexical_cast &e)
            {
                cerr << "Cant parse tx_prove as bool. Using default value" << endl;
            }

            myxmr::jsonresponse r{xmrblocks.json_outputs(
                    remove_bad_chars(tx_hash),
                    remove_bad_chars(address),
                    remove_bad_chars(viewkey),
                    tx_prove)};

            return r;
        });

        CROW_ROUTE(app, "/api/outputsblocks").methods("GET"_method)
        ([&](const crow::request &req) {

            string limit = regex_search(req.raw_url, regex {"limit=\\d+"}) ?
                           req.url_params.get("limit") : "3";

            string address = regex_search(req.raw_url, regex {"address=\\w+"}) ?
                             req.url_params.get("address") : "";

            string viewkey = regex_search(req.raw_url, regex {"viewkey=\\w+"}) ?
                             req.url_params.get("viewkey") : "";

            bool in_mempool_aswell {false};

            try
            {
                in_mempool_aswell = regex_search(
                        req.raw_url, regex {"mempool=[01]"}) ?
                           boost::lexical_cast<bool>(
                                   req.url_params.get("mempool")) :
                           false;
            }
            catch (const boost::bad_lexical_cast &e)
            {
                cerr << "Cant parse tx_prove as bool. Using default value" 
                     << endl;
            }

            myxmr::jsonresponse r{xmrblocks.json_outputsblocks(
                    remove_bad_chars(limit),
                    remove_bad_chars(address),
                    remove_bad_chars(viewkey),
                    in_mempool_aswell)};

            return r;
        });

        CROW_ROUTE(app, "/api/version")
        ([&]() {

            myxmr::jsonresponse r{xmrblocks.json_version()};

            return r;
        });

    } // if (enable_json_api)

    if (enable_autorefresh_option)
    {
        CROW_ROUTE(app, "/autorefresh")
        ([&]() {
            uint64_t page_no {0};
            bool refresh_page {true};
            return myxmr::htmlresponse(xmrblocks.index2(page_no, refresh_page));
        });
    }

    // run the crow http server

    if (use_ssl)
    {
        cout << "Staring in ssl mode" << endl;
        app.bindaddr(bindaddr).port(app_port).ssl_file(
                ssl_crt_file, ssl_key_file)
                .multithreaded().run();
    }
    else
    {
        cout << "Staring in non-ssl mode" << endl;
        if (*concurrency_opt == 0)
        {
            app.bindaddr(bindaddr).port(app_port).multithreaded().run();
        }
        else
        {
            app.bindaddr(bindaddr).port(app_port)
                .concurrency(*concurrency_opt).run();
        }
    }

    if (enable_emission_monitor == true)
    {
        // finish Emission monitoring thread in a cotrolled manner.

        cout << "Waiting for emission monitoring thread to finish." << endl;

        xmreg::CurrentBlockchainStatus::m_thread.interrupt();
        xmreg::CurrentBlockchainStatus::m_thread.join();

        cout << "Emission monitoring thread finished." << endl;
    }

    // finish mempool thread

    cout << "Waiting for mempool monitoring thread to finish." << endl;

    xmreg::MempoolStatus::m_thread.interrupt();
    xmreg::MempoolStatus::m_thread.join();

    cout << "Mempool monitoring thread finished." << endl;

    cout << "The explorer is terminating." << endl;

    return EXIT_SUCCESS;
}

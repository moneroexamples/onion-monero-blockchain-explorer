# Onion Monero Blockchain Explorer

Curently available Monero blockchain explorer websites have several limitations which are of special importance to privacy-oriented users:

 - they use JavaScript,
 - have images which might be used for [cookieless tracking](http://lucb1e.com/rp/cookielesscookies/),
 - track users activates through google analytics,
 - are closed sourced,
 - are not available as hidden services,
 - provide only basic search capabilities,
 - can't identify users outputs based on provided Monero address and viewkey, or private tx key,
 - do not support Monero testnet, i.e., they dont work with RingCT transactions


In this example, these limitations are addressed by development of
an Onion Monero Blockchain Explorer. The example not only shows how to use Monero C++ libraries, but also demonstrates how to use:

 - [crow](https://github.com/ipkn/crow) - C++ micro web framework
 - [lmdb++](https://github.com/bendiken/lmdbxx) - C++ wrapper for the LMDB
 - [mstch](https://github.com/no1msd/mstch) - C++ {{mustache}} templates
 - [json](https://github.com/nlohmann/json) - JSON for Modern C++
 - [date](https://github.com/HowardHinnant/date) - C++ date and time library
 - [fmt](https://github.com/fmtlib/fmt) - Small, safe and fast string formatting library

## Address

Tor users (main Monero network):

 - [http://xmrblocksvckbwvx.onion](http://xmrblocksvckbwvx.onion)
 
Tor users (testnet Monero network):
 
 - [http://xmrtestbbzl275vy.onion](http://xmrtestbbzl275vy.onion)
 
i2p users (main Monero network):

 - [http://monerotools.i2p](http://monerotools.i2p)

Non tor users, can use its clearnet version (thanks to [Gingeropolous](https://github.com/Gingeropolous)):

 - [http://explore.MoneroWorld.com](http://explore.moneroworld.com)
 
 
## Onion Monero Blockchain Explorer features

The key features of the Onion Monero Blockchain Explorer are:

 - available as a hidden service,
 - no javascript, no cookies, no web analytics trackers, no images,
 - open sourced,
 - made fully in C++,
 - the only explorer showing encrypted payments ID,
 - the only explorer with the ability to search by encrypted payments ID, tx public
 and private keys, stealth addresses, input key images, block timestamps 
 (UTC time, e.g., 2016-11-23 14:03:05)
 - the only explorer showing ring signatures,
 - the only explorer showing transaction extra field,
 - the only explorer showing public components of Monero addresses,
 - the only explorer that can show which outputs and mixins belong to the given Monero address and viewkey,
 - the only explorer that can be used to prove that you send Monero to someone,
 - the only explorer showing detailed information about mixins, such as, mixins'
 age, timescale, mixin of mixins,
 - the only explorer showing number of amount output indices,
 - the only explorer supporting Monero testnet network and RingCT,
 - the only explorer providing tx checker and pusher for online pushing of transactions,
 - the only explorer allowing to inspect encrypted key images file and output files.

## Prerequisite

Everything here was done and tested using Monero 0.9.4 on Ubuntu 16.04 x86_64.

Instruction for Monero 0.10 compilation and Monero headers and libraries setup are
as shown here:
 - [Compile Monero 0.9 on Ubuntu 16.04 x64](https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04)
 - [lmdbcpp-monero](https://github.com/moneroexamples/lmdbcpp-monero.git)

## C++ code

```c++
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
```



## Example screenshot

![Onion Monero Blockchain Explorer](https://raw.githubusercontent.com/moneroexamples/onion-monero-blockchain-explorer/master/screenshot/screenshot.jpg)


## Compile and run the explorer

##### Monero headers and libraries setup

The Onion Explorer uses Monero C++ libraries and headers. 
 Instructions how to download source files and compile Monero,
 setup header and library files are presented here:

- https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04 (Ubuntu 16.04)
- https://github.com/moneroexamples/compile-monero-09-on-arch-linux (Arch Linux)


##### Custom lmdb database (optional)

Most unique search abilities of the Onion Explorer are achieved through using
a [custom lmdb database](https://github.com/moneroexamples/lmdbcpp-monero.git) constructed based on the Monero blockchain.
The reason for the custom database is that Monero's own lmdb database has limited
search abilities. For example, its not possible to search for a tx having a
 given key image, except by performing an exhaustive search on the blockchain which is very time consuming.

Instruction how to compile the `lmdbcpp-monero` are provided here:

 - https://github.com/moneroexamples/lmdbcpp-monero.git

The custom database is rather big, 12GB now, and it must be running alongside Monero deamon
 so that it keeps updating itself with new information from new blocks as they are added
  to the blockchain.

For these reasons, its use is optional. However, without it, some searches wont be possible,
e.g., searching for key images, output and tx public keys, encrypted payments id.

##### Compile and run the explorer
Once the Monero is compiled and setup, the explorer can be downloaded and compiled
as follows:

```bash
# download the source code
git clone https://github.com/moneroexamples/onion-monero-blockchain-explorer.git

# enter the downloaded sourced code folder
cd onion-monero-blockchain-explorer

# make a build folder and enter it
mkdir build && cd build

# create the makefile
cmake ..

# compile
make
```

When compilation finishes executable `xmrblocks` should be created.

To run it:
```
./xmrblocks
```

Example output:

```bash
[mwo@arch onion-monero-blockchain-explorer]$ ./xmrblocks
2016-May-28 10:04:49.160280 Blockchain initialized. last block: 1056761, d0.h0.m12.s47 time ago, current difficulty: 1517857750
(2016-05-28 02:04:49) [INFO    ] Crow/0.1 server is running, local port 8081
```

Go to your browser: http://127.0.0.1:8081

## Enable SSL (https)

By default, the explorer does not use ssl. But It has such a functionality. 

As an example, you can generate your own ssl certificates as follows:

```bash
cd /tmp # example folder
openssl genrsa -out server.key 1024
openssl req -new -key server.key -out server.csr
openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt
```

Having the `crt` and `key` files, run `xmrblocks` in the following way:

```bash
./xmrblocks --ssl-crt-file=/tmp/server.crt --ssl-key-file=/tmp/server.key 
```

Note: Because we generated our own certificate, modern browsers will complain
about it as they cant verify the signatures against any third party. So probably
for any practical use need to have properly issued ssl certificates. 

## Other examples

Other examples can be found on  [github](https://github.com/moneroexamples?tab=repositories).
Please know that some of the examples/repositories are not
finished and may not work as intended.

## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

Some Monero are also welcome:
```
48daf1rG3hE1Txapcsxh6WXNe9MLNKtu7W7tKTivtSoVLHErYzvdcpea2nSTgGkz66RFP4GKVAsTV14v6G3oddBTHfxP6tU
```

# Onion Monero Blockchain Explorer

Monero blockchain explorer websites exist in the clearnet. Although useful,
their limitations are that they use JavaScript, have images
([might be used for coockless tracking](http://lucb1e.com/rp/cookielesscookies/)),
track users activates through google analytics, are not open sourced, and are not
 available as hidden services. These things are of importance
 for privacy-oriented users.

 In this example, these limitations are addressed by development of
 an Onion Monero Blockchain Explorer. It is build in C++,
 and it not only shows how to use Monero C++ libraries, but also demonstrates how to
 use:

  - [crow](https://github.com/ipkn/crow) - C++ micro web framework
  - [lmdb++](https://github.com/bendiken/lmdbxx) - C++ wrapper for the LMDB
  - [mstch](https://github.com/no1msd/mstch) - C++ {{mustache}} templates
  - [rapidjson](https://github.com/miloyip/rapidjson) - C++ JSON parser/generator


## Onion Monero Blockchain Explorer features

 - no javascript, no web analytics trackers, no images
 - open source
 - made fully in C++
 - the only explorer showing encrypted payments ID
 - the only explorer with the ability to search by encrypted payments ID, tx public
 keys, outputs public keys, input key images
 - the only explorer showing ring signatures


## Prerequisite

Everything here was done and tested using Monero 0.9.4 on
Xubuntu 16.04 x86_64.

Instruction for Monero 0.9 compilation and Monero headers and libraries setup are
as shown here:
 - [Compile Monero 0.9 on Ubuntu 16.04 x64](https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04)
 - [lmdbcpp-monero](https://github.com/moneroexamples/lmdbcpp-monero.git)

## C++ code

```c++
int main(int ac, const char* av[]) {

    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    auto help_opt = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return EXIT_SUCCESS;
    }

    auto port_opt           = opts.get_option<string>("port");
    auto bc_path_opt        = opts.get_option<string>("bc-path");
    auto custom_db_path_opt = opts.get_option<string>("custom-db-path");
    auto deamon_url_opt     = opts.get_option<string>("deamon-url");

    //cast port number in string to uint16
    uint16_t app_port = boost::lexical_cast<uint16_t>(*port_opt);

    // get blockchain path
    path blockchain_path;

    if (!xmreg::get_blockchain_path(bc_path_opt, blockchain_path))
    {
        cerr << "Error getting blockchain path." << endl;
        return EXIT_FAILURE;
    }

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

    // create instance of page class which
    // contains logic for the website
    xmreg::page xmrblocks(&mcore, core_storage, *deamon_url_opt);

    // crow instance
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([&]() {
        return xmrblocks.index2();
    });

    CROW_ROUTE(app, "/page/<uint>")
    ([&](size_t page_no) {
        return xmrblocks.index2(page_no);
    });

    CROW_ROUTE(app, "/block/<uint>")
    ([&](size_t block_height) {
        return xmrblocks.show_block(block_height);
    });

    CROW_ROUTE(app, "/block/<string>")
    ([&](string block_hash) {
        return xmrblocks.show_block(block_hash);
    });

    CROW_ROUTE(app, "/tx/<string>")
    ([&](string tx_hash) {
        return xmrblocks.show_tx(tx_hash);
    });

    CROW_ROUTE(app, "/tx/<string>/<uint>")
    ([&](string tx_hash, uint with_ring_signatures) {
        return xmrblocks.show_tx(tx_hash, with_ring_signatures);
    });


    CROW_ROUTE(app, "/search").methods("GET"_method)
    ([&](const crow::request& req) {
        return xmrblocks.search(string(req.url_params.get("value")));
    });

    CROW_ROUTE(app, "/autorefresh")
    ([&]() {
        uint64_t page_no {0};
        bool refresh_page {true};
        return xmrblocks.index2(page_no, refresh_page);
    });

    // run the crow http server
    app.port(app_port).multithreaded().run();

    return EXIT_SUCCESS;
}
```



## Example screenshot

![Onion Monero Blockchain Explorer](https://raw.githubusercontent.com/moneroexamples/onion-monero-blockchain-explorer/master/screenshot/screenshot.jpg | width=400)




## Compile this example
The dependencies are same as those for Monero, so I assume Monero compiles
correctly. If so then to download and compile this example, the following
steps can be executed:

```bash
# download the source code
https://github.com/moneroexamples/onion-monero-blockchain-explorer.git

# enter the downloaded sourced code folder
cd onion-monero-blockchain-explorer

# create the makefile
cmake .

# compile
make
```


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





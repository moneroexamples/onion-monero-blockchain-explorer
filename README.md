# Onion Monero Blockchain Explorer

Curently available Monero blockchain explorer websites have several limitations which are of special importance to privacy-oriented users:

 - they use JavaScript,
 - have images which might be used for [cookieless tracking](http://lucb1e.com/rp/cookielesscookies/),
 - track users activates through google analytics,
 - are closed sourced,
 - are not available as hidden services,
 - provide only basic search capabilities,
 - can't identify users outputs based on provided Monero address and viewkey.


In this example, these limitations are addressed by development of
an Onion Monero Blockchain Explorer. The example not only shows how to use Monero C++ libraries, but also demonstrates how to use:

 - [crow](https://github.com/ipkn/crow) - C++ micro web framework
 - [lmdb++](https://github.com/bendiken/lmdbxx) - C++ wrapper for the LMDB
 - [mstch](https://github.com/no1msd/mstch) - C++ {{mustache}} templates
 - [rapidjson](https://github.com/miloyip/rapidjson) - C++ JSON parser/generator


## Onion address

Tor users:

 - [http://xmrblocksvckbwvx.onion](http://xmrblocksvckbwvx.onion)

Non tor users, can try tor proxy, e.g.,

 - [http://xmrblocksvckbwvx.onion.to](http://xmrblocksvckbwvx.onion.to)

## Still under development

The Onion Monero Blockchain Explorer is still under development and testing.
So not everything can work as expected and the explorer can be offline from time
to time, when changes are being made to it.

## Onion Monero Blockchain Explorer features

The key features of the Onion Monero Blockchain Explorer are

 - available as a hidden service,
 - no javascript, no web analytics trackers, no images,
 - open sourced,
 - made fully in C++,
 - the only explorer showing encrypted payments ID,
 - the only explorer with the ability to search by encrypted payments ID, tx public
 keys, outputs public keys, input key images, output global indices,
 output amount index and its amount,
 - the only explorer showing ring signatures,
 - the only explorer that can show which outputs belong to the given Monero address and viewkey,
 - the only explorer showing detailed information about mixins, such as, mixins'
 age, timescale, mixin of mixins,
 - the only explorer showing amount output indices.

## Prerequisite

Everything here was done and tested using Monero 0.9.4 on Ubuntu 16.04 x86_64.

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

    CROW_ROUTE(app, "/myoutputs").methods("GET"_method)
    ([&](const crow::request& req) {

        string tx_hash     = string(req.url_params.get("tx_hash"));
        string xmr_address = string(req.url_params.get("xmr_address"));
        string viewkey     = string(req.url_params.get("viewkey"));

        return xmrblocks.show_my_outputs(tx_hash, xmr_address, viewkey);
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

![Onion Monero Blockchain Explorer](https://raw.githubusercontent.com/moneroexamples/onion-monero-blockchain-explorer/master/screenshot/screenshot.jpg)


## Compile this example
The dependencies are same as those for Monero, so I assume Monero compiles
correctly. If so then to download and compile this example, the following
steps can be executed:

```bash
# download the source code
git clone https://github.com/moneroexamples/onion-monero-blockchain-explorer.git

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

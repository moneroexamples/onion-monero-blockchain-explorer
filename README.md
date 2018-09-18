# Onion Monero Blockchain Explorer

Currently available Monero blockchain explorers have several limitations which are of
special importance to privacy-oriented users:

 - they use JavaScript,
 - have images which might be used for [cookieless tracking](http://lucb1e.com/rp/cookielesscookies/),
 - track users activates through google analytics,
 - are closed sourced,
 - are not available as hidden services,
 - do not support Monero testnet nor stagenet networks,
 - have limited JSON API.


In this example, these limitations are addressed by development of
an Onion Monero Blockchain Explorer. The example not only shows how to use
Monero C++ libraries, but also demonstrates how to use:

 - [crow](https://github.com/ipkn/crow) - C++ micro web framework
 - [mstch](https://github.com/no1msd/mstch) - C++ {{mustache}} templates
 - [json](https://github.com/nlohmann/json) - JSON for Modern C++
 - [fmt](https://github.com/fmtlib/fmt) - Small, safe and fast string formatting library

## Explorer hosts

Tor users:

 - [http://dvwae436pd7nt4bc.onion](http://dvwae436pd7nt4bc.onion) (Front-end templates are [maintained by @suhz](https://github.com/suhz/onion-monero-blockchain-explorer/tree/moneroexplorer.com/src/templates)).

Clearnet versions:
 - [http://172.104.45.209:8081/](http://172.104.45.209:8081/) - devel version, javascript enabled.
 - [https://labor.serveo.net/](https://labor.serveo.net/) - temprorary link (slow), bleading edge version.
 - [https://xmrchain.net/](https://xmrchain.net/) - https enabled, most popular and very stable.
 - [https://MoneroExplorer.com/](https://moneroexplorer.com/) - nice looking one, https enabled.
 - [https://monerohash.com/explorer/](https://monerohash.com/explorer/) - nice looking one, https enabled.
 - [http://explore.MoneroWorld.com](http://explore.moneroworld.com) - same as the second one.
 - [https://moneroexplorer.pro/](https://moneroexplorer.pro/) - nice looking one, https enabled.
 - [http://monerochain.com/](http://monerochain.com/) - JSON API based, multiple nodes.   
 - [https://blox.minexmr.com/](https://blox.minexmr.com/) - - https enabled.

Testnet version:

 - [http://nimis.serveo.net/](http://nimis.serveo.net/) - bleeding edge version (down currently).
 - [https://testnet.xmrchain.com/](https://testnet.xmrchain.com/) - https enabled.
 - [https://explorer.monero-otc.com/](https://explorer.monero-otc.com/) - https enabled.

Stagenet version:

 - [http://162.210.173.150:8083/](http://162.210.173.150:8083/) - recent version.

i2p users (main Monero network):

 - [http://7o4gezpkye6ekibhgpkg7v626ze4idsirapufzrefkdysa6zxhha.b32.i2p/](http://7o4gezpkye6ekibhgpkg7v626ze4idsirapufzrefkdysa6zxhha.b32.i2p/)

Alternative block explorers:

- [http://moneroblocks.info](http://moneroblocks.info/)
- [https://monerobase.com](https://monerobase.com/)
- [https://monerovision.com](https://monerovision.com)
- [http://chainradar.com](http://chainradar.com/xmr/blocks)


## Onion Monero Blockchain Explorer features

The key features of the Onion Monero Blockchain Explorer are:

 - no cookies, no web analytics trackers, no images,
 - by default no JavaScript, but can be enabled for client side decoding and proving transactions,
 - open sourced,
 - made fully in C++,
 - showing encrypted payments ID,
 - showing ring signatures,
 - showing transaction extra field,
 - showing public components of Monero addresses,
 - decoding which outputs and mixins belong to the given Monero address and viewkey,
 - can prove that you send Monero to someone,
 - detailed information about ring members, such as, their age, timescale and their ring sizes,
 - showing number of amount output indices,
 - support Monero testnet and stagnet networks,
 - tx checker and pusher for online pushing of transactions,
 - estimate possible spendings based on address and viewkey,
 - can provide total amount of all miner fees,
 - decoding encrypted payment id,
 - decoding outputs and proving txs sent to sub-address.


## Development branch

Current development branch:

 - https://github.com/moneroexamples/onion-monero-blockchain-explorer/tree/devel



## Compilation on Ubuntu 16.04/18.04

##### Compile latest Monero development version

Download and compile recent Monero into your home folder:

```bash
# first install monero dependecines
sudo apt update

sudo apt install git build-essential cmake libboost-all-dev miniupnpc libunbound-dev graphviz doxygen libunwind8-dev pkg-config libssl-dev libcurl4-openssl-dev libgtest-dev libreadline-dev libzmq3-dev libsodium-dev libpcsclite-dev

# go to home folder
cd ~

git clone --recursive https://github.com/monero-project/monero

cd monero/


USE_SINGLE_BUILDDIR=1 make
```

##### Compile and run the explorer

Once the Monero is compiles, the explorer can be downloaded and compiled
as follows:

```bash
# go to home folder if still in ~/monero
cd ~

# download the source code
git clone https://github.com/moneroexamples/onion-monero-blockchain-explorer.git

# enter the downloaded sourced code folder
cd onion-monero-blockchain-explorer

# make a build folder and enter it
mkdir build && cd build

# create the makefile
cmake ..

# altearnatively can use: cmake -DMONERO_DIR=/path/to/monero_folder ..
# if monero is not in ~/monero
#
# also can build with ASAN (sanitizers), for example
# cmake -DSANITIZE_ADDRESS=On ..

# compile
make
```


To run it:
```
./xmrblocks
```

By default it will look for blockchain in its default location i.e., `~/.bitmonero/lmdb`.
You can use `-b` option if its in different location.

For example:

```bash
./xmrblocks -b /home/mwo/non-defult-monero-location/lmdb/
```

Example output:

```bash
[mwo@arch onion-monero-blockchain-explorer]$ ./xmrblocks
2016-May-28 10:04:49.160280 Blockchain initialized. last block: 1056761, d0.h0.m12.s47 time ago, current difficulty: 1517857750
(2016-05-28 02:04:49) [INFO    ] Crow/0.1 server is running, local port 8081
```

Go to your browser: http://127.0.0.1:8081

## The explorer's command line options

```
xmrblocks, Onion Monero Blockchain Explorer:
  -h [ --help ] [=arg(=1)] (=0)         produce help message
  -t [ --testnet ] [=arg(=1)] (=0)      use testnet blockchain
  -s [ --stagenet ] [=arg(=1)] (=0)     use stagenet blockchain
  --enable-pusher [=arg(=1)] (=0)       enable signed transaction pusher
  --enable-mixin-details [=arg(=1)] (=0)
                                        enable mixin details for key images,
                                        e.g., timescale, mixin of mixins, in tx
                                        context
  --enable-key-image-checker [=arg(=1)] (=0)
                                        enable key images file checker
  --enable-output-key-checker [=arg(=1)] (=0)
                                        enable outputs key file checker
  --enable-json-api [=arg(=1)] (=1)     enable JSON REST api
  --enable-tx-cache [=arg(=1)] (=0)     enable caching of transaction details
  --show-cache-times [=arg(=1)] (=0)    show times of getting data from cache
                                        vs no cache
  --enable-block-cache [=arg(=1)] (=0)  enable caching of block details
  --enable-js [=arg(=1)] (=0)           enable checking outputs and proving txs
                                        using JavaScript on client side
  --enable-autorefresh-option [=arg(=1)] (=0)
                                        enable users to have the index page on
                                        autorefresh
  --enable-emission-monitor [=arg(=1)] (=0)
                                        enable Monero total emission monitoring
                                        thread
  -p [ --port ] arg (=8081)             default explorer port
  --testnet-url arg                     you can specify testnet url, if you run
                                        it on mainnet or stagenet. link will
                                        show on front page to testnet explorer
  --stagenet-url arg                    you can specify stagenet url, if you
                                        run it on mainnet or testnet. link will
                                        show on front page to stagenet explorer
  --mainnet-url arg                     you can specify mainnet url, if you run
                                        it on testnet or stagenet. link will
                                        show on front page to mainnet explorer
  --no-blocks-on-index arg (=10)        number of last blocks to be shown on
                                        index page
  --mempool-info-timeout arg (=5000)    maximum time, in milliseconds, to wait
                                        for mempool data for the front page
  --mempool-refresh-time arg (=5)       time, in seconds, for each refresh of
                                        mempool state
  -b [ --bc-path ] arg                  path to lmdb folder of the blockchain,
                                        e.g., ~/.bitmonero/lmdb
  --ssl-crt-file arg                    path to crt file for ssl (https)
                                        functionality
  --ssl-key-file arg                    path to key file for ssl (https)
                                        functionality
  -d [ --deamon-url ] arg (=http:://127.0.0.1:18081)
                                        Monero deamon url
```

Example usage, defined as bash aliases.

```bash
# for mainnet explorer
alias xmrblocksmainnet='~/onion-monero-blockchain-explorer/build/xmrblocks    --port 8081 --testnet-url "http://139.162.32.245:8082" --enable-pusher --enable-emission-monitor'

# for testnet explorer
alias xmrblockstestnet='~/onion-monero-blockchain-explorer/build/xmrblocks -t --port 8082 --mainnet-url "http://139.162.32.245:8081" --enable-pusher --enable-emission-monitor'
```

These are aliases similar to those used for http://139.162.32.245:8081/ and http://139.162.32.245:8082/, respectively.

## Enable Monero emission

Obtaining current Monero emission amount is not straight forward. Thus, by default it is
disabled. To enable it use `--enable-emission-monitor` flag, e.g.,


```bash
xmrblocks --enable-emission-monitor
```

This flag will enable emission monitoring thread. When started, the thread
 will initially scan the entire blockchain, and calculate the cumulative emission based on each block.
Since it is a separate thread, the explorer will work as usual during this time.
Every 10000 blocks, the thread will save current emission in a file, by default,
 in `~/.bitmonero/lmdb/emission_amount.txt`. For testnet or stagenet networks,
 it is `~/.bitmonero/testnet/lmdb/emission_amount.txt` or `~/.bitmonero/stagenet/lmdb/emission_amount.txt`. This file is used so that we don't
 need to rescan entire blockchain whenever the explorer is restarted. When the
 explorer restarts, the thread will first check if `~/.bitmonero/lmdb/emission_amount.txt`
 is present, read its values, and continue from there if possible. Subsequently, only the initial
 use of the tread is time consuming. Once the thread scans the entire blockchain, it updates
 the emission amount using new blocks as they come. Since the explorer writes this file, there can
 be only one instance of it running for mainnet, testnet and stagenet. Thus, for example, you cant have
 two explorers for mainnet
 running at the same time, as they will be trying to write and read the same file at the same time,
 leading to unexpected results. Off course having one instance for mainnet and one instance for testnet
 is fine, as they write to different files.

 When the emission monitor is enabled, information about current emission of coinbase and fees is
 displayed on the front page, e.g., :

```
Monero emission (fees) is 14485540.430 (52545.373) as of 1313448 block
```

The values given, can be checked using Monero daemon's  `print_coinbase_tx_sum` command.
For example, for the above example: `print_coinbase_tx_sum 0 1313449`.

To disable the monitor, simply restart the explorer without `--enable-emission-monitor` flag.

## Enable JavaScript for decoding proving transactions

By default, decoding and proving tx's outputs are done on the server side. To do this on the client side
(private view and tx keys are not send to the server) JavaScript-based decoding can be enabled:

```
xmrblocks --enable-js
```

## Enable SSL (https)

By default, the explorer does not use ssl. But it has such a functionality.

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

## JSON API

The explorer has JSON api. For the API, it uses conventions defined by [JSend](https://labs.omniti.com/labs/jsend).
By default the api is disabled. To enable it, use `--enable-json-api` flag, e.g.,

```
./xmrblocks --enable-json-api
```

#### api/transaction/<tx_hash>

```bash
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/transaction/6093260dbe79fd6277694d14789dc8718f1bd54457df8bab338c2efa3bb0f03d"
```

Partial results shown:

```json
{
  "data": {
    "block_height": 1268252,
    "coinbase": false,
    "confirmations": 1,
    "current_height": 1268253,
    "extra": "01be23e277aed6b5f41f66b05244bf994c13108347366ec678ae16657f0fc3a22b",
    "inputs": [
      {
        "amount": 0,
        "key_image": "67838fd0ffd79f13e735830d3ec60412aed59e53e1f997feb6f73d088b949611",
        "mixins": [
          {
            "block_no": 1238623,
            "public_key": "0a5b853c55303c10e1326acfb085b9e246e088b1ccac7e37f7a810d46a28a914"
          },
          {
            "block_no": 1246942,
            "public_key": "527cf86f5abbfb006c970f7c6eb40493786d4751306f8985c6a43f98a88c0dff"
          }
        ]
      }
    ],
    "mixin": 9,
    "outputs": [
      {
        "amount": 0,
        "public_key": "525779873776e4a42f517fd79b72e7c31c3ba03e730fc32287f6414fb702c1d7"
      },
      {
        "amount": 0,
        "public_key": "e25f00fceb77af841d780b68647618812695b4ca6ebe338faba6e077f758ac30"
      }
    ],
    "payment_id": "",
    "payment_id8": "",
    "rct_type": 1,
    "timestamp": 1489753456,
    "timestamp_utc": "2017-03-17 12:24:16",
    "tx_fee": 12517785574,
    "tx_hash": "6093260dbe79fd6277694d14789dc8718f1bd54457df8bab338c2efa3bb0f03d",
    "tx_size": 13323,
    "tx_version": 2,
    "xmr_inputs": 0,
    "xmr_outputs": 0
  },
  "status": "success"
}
```

#### api/transactions

Transactions in last 25 blocks


```bash
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/transactions"
```

Partial results shown:

```json
{
  "data": {
    "blocks": [
      {
        "age": "33:16:49:53",
        "height": 1268252,
        "size": 105390000000000000,
        "timestamp": 1489753456,
        "timestamp_utc": "2017-03-17 12:24:16",
        "txs": [
          {
            "coinbase": true,
            "mixin": 0,
            "outputs": 8491554678365,
            "rct_type": 0,
            "tx_fee": 0,
            "tx_hash": "7c4286f64544568265bb5418df84ae69afaa3567749210e46f8340c247f4803f",
            "tx_size": 151000000000000,
            "tx_version": 2
          },
          {
            "coinbase": false,
            "mixin": 5,
            "outputs": 0,
            "rct_type": 2,
            "tx_fee": 17882516700,
            "tx_hash": "2bfbccb918ee5f050808dd040ce03943b7315b81788e9cdee59cf86b557ba48c",
            "tx_size": 19586000000000000,
            "tx_version": 2
          }
        ]
      }
    ],
    "limit": 25,
    "page": 0
  },
  "status": "success"
}
```

#### api/transactions?page=<page_no>&limit=<tx_per_page>


```bash
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/transactions?page=2&limit=10"
```

Result analogical to the one above.

#### api/block/<block_number|block_hash>


```bash
curl  -w "\n" -X GET "http://139.162.32.245:8081/api/block/1293257"
```

Partial results shown:

```json
{
  "data": {
    "block_height": 1293257,
    "block_reward": 0,
    "current_height": 1293264,
    "hash": "9ef6bb8f9b8bd253fc6390e5c2cdc45c8ee99fad16447437108bf301fe6bd6e1",
    "size": 141244,
    "timestamp": 1492761974,
    "timestamp_utc": "2017-04-21 08:06:14",
    "txs": [
      {
        "coinbase": true,
        "extra": "018ae9560eb85d5ebd22d3beaed55c21d469eab430c5e3cac61b3fe2f5ad156770020800000001a9030800",
        "mixin": 0,
        "payment_id": "",
        "payment_id8": "",
        "rct_type": 0,
        "tx_fee": 0,
        "tx_hash": "3ff71b65bec34c9261e01a856e6a03594cf0472acf6b77db3f17ebd18eaa30bf",
        "tx_size": 95,
        "tx_version": 2,
        "xmr_inputs": 0,
        "xmr_outputs": 8025365394426
      }
    ]
  },
  "status": "success"
}
```

#### api/mempool

Return all txs in the mempool.

```bash
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/mempool"
```

Partial results shown:

```json
{
  "data": {
    "limit": 100000000,
    "page": 0,
    "total_page_no": 0,
    "txs": [
      {
        "coinbase": false,
        "extra": "022100325f677d96f94155a4840a84d8e0c905f7a4697a25744633bcb438feb1e51fb2012eda81bf552c53c2168f4130dbe0265c3a7898f3a7eee7c1fed955a778167b5d",
        "mixin": 3,
        "payment_id": "325f677d96f94155a4840a84d8e0c905f7a4697a25744633bcb438feb1e51fb2",
        "payment_id8": "",
        "rct_type": 2,
        "timestamp": 1494470894,
        "timestamp_utc": "2017-05-11 02:48:14",
        "tx_fee": 15894840000,
        "tx_hash": "9f3374f8ac67febaab153eab297937a3d0d2c706601e496bf5028146da0c9aef",
        "tx_size": 13291,
        "tx_version": 2,
        "xmr_inputs": 0,
        "xmr_outputs": 0
      }
    ],
    "txs_no": 7
  },
  "status": "success"
}
```

Limit of 100000000 is just default value above to ensure that all mempool txs are fetched
if no specific limit given.

#### api/mempool?limit=<no_of_top_txs>

Return number of newest mempool txs, e.g., only 10.

```bash
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/mempool?limit=10"
```

Result analogical to the one above.

#### api/search/<block_number|tx_hash|block_hash>

```bash
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/search/1293669"
```

Partial results shown:

```json
{
  "data": {
    "block_height": 1293669,
    "current_height": 1293670,
    "hash": "5d55b8fabf85b0b4c959d66ad509eb92ddfe5c2b0e84e1760abcb090195c1913",
    "size": 118026,
    "timestamp": 1492815321,
    "timestamp_utc": "2017-04-21 22:55:21",
    "title": "block",
    "txs": [
      {
        "coinbase": true,
        "extra": "01cb7fda09033a5fa06dc601b9295ef3790397cf3c645e958e34cf7ab699d2f5230208000000027f030200",
        "mixin": 0,
        "payment_id": "",
        "payment_id8": "",
        "rct_type": 0,
        "tx_fee": 0,
        "tx_hash": "479ba432f5c88736b438dd4446a11a13046a752d469f7828151f5c5b86be4e9a",
        "tx_size": 95,
        "tx_version": 2,
        "xmr_inputs": 0,
        "xmr_outputs": 7992697599717
      }
    ]
  },
  "status": "success"
}
```


#### api/outputs?txhash=<tx_hash>&address=<address>&viewkey=<viewkey>&txprove=<0|1>

For `txprove=0` we check which outputs belong to given address and corresponding viewkey.
For `txprove=1` we use to prove to the recipient that we sent them founds.
For this, we use recipient's address and our tx private key as a viewkey value,
 i.e., `viewkey=<tx_private_key>`

Checking outputs:

```bash
# we use here official Monero project's donation address as an example
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/outputs?txhash=17049bc5f2d9fbca1ce8dae443bbbbed2fc02f1ee003ffdd0571996905faa831&address=44AFFq5kSiGBoZ4NMDwYtN18obc8AemS33DBLWs3H7otXft3XjrpDtQGv7SqSsaBYBb98uNbr2VBBEt7f2wfn3RVGQBEP3A&viewkey=f359631075708155cc3d92a32b75a7d02a5dcf27756707b47a2b31b21c389501&txprove=0"
```

```json
{
  "data": {
    "address": "42f18fc61586554095b0799b5c4b6f00cdeb26a93b20540d366932c6001617b75db35109fbba7d5f275fef4b9c49e0cc1c84b219ec6ff652fda54f89f7f63c88",
    "outputs": [
      {
        "amount": 34980000000000,
        "match": true,
        "output_idx": 0,
        "output_pubkey": "35d7200229e725c2bce0da3a2f20ef0720d242ecf88bfcb71eff2025c2501fdb"
      },
      {
        "amount": 0,
        "match": false,
        "output_idx": 1,
        "output_pubkey": "44efccab9f9b42e83c12da7988785d6c4eb3ec6e7aa2ae1234e2f0f7cb9ed6dd"
      }
    ],
    "tx_hash": "17049bc5f2d9fbca1ce8dae443bbbbed2fc02f1ee003ffdd0571996905faa831",
    "tx_prove": false,
    "viewkey": "f359631075708155cc3d92a32b75a7d02a5dcf27756707b47a2b31b21c389501"
  },
  "status": "success"
}
```

Proving transfer:

We use recipient's address (i.e. not our address from which we sent xmr to recipient).
For the viewkey, we use `tx_private_key` (although the GET variable is still called `viewkey`) that we obtained by sending this txs.

```bash
# this is for testnet transaction
curl  -w "\n" -X GET "http://127.0.0.1:8082/api/outputs?txhash=94782a8c0aa8d8768afa0c040ef0544b63eb5148ca971a024ac402cad313d3b3&address=9wUf8UcPUtb2huK7RphBw5PFCyKosKxqtGxbcKBDnzTCPrdNfJjLjtuht87zhTgsffCB21qmjxjj18Pw7cBnRctcKHrUB7N&viewkey=e94b5bfc599d2f741d6f07e3ab2a83f915e96fb374dfb2cd3dbe730e34ecb40b&txprove=1"
```

```json
{
  "data": {
    "address": "71bef5945b70bc0a31dbbe6cd0bd5884fe694bbfd18fff5f68f709438554fb88a51b1291e378e2f46a0155108782c242cc1be78af229242c36d4f4d1c4f72da2",
    "outputs": [
      {
        "amount": 1000000000000,
        "match": true,
        "output_idx": 0,
        "output_pubkey": "c1bf4dd020b5f0ab70bd672d2f9e800ea7b8ab108b080825c1d6cfc0b7f7ee00"
      },
      {
        "amount": 0,
        "match": false,
        "output_idx": 1,
        "output_pubkey": "8c61fae6ada2a103565dfdd307c7145b2479ddb1dab1eaadfa6c34db65d189d5"
      }
    ],
    "tx_hash": "94782a8c0aa8d8768afa0c040ef0544b63eb5148ca971a024ac402cad313d3b3",
    "tx_prove": true,
    "viewkey": "e94b5bfc599d2f741d6f07e3ab2a83f915e96fb374dfb2cd3dbe730e34ecb40b"
  },
  "status": "success"
}
```


Result analogical to the one above.

#### api/networkinfo

```bash
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/networkinfo"
```

```json
{
  "data": {
    "alt_blocks_count": 0,
    "block_size_limit": 600000,
    "cumulative_difficulty": 2091549555696348,
    "difficulty": 7941560081,
    "fee_per_kb": 303970000,
    "grey_peerlist_size": 4991,
    "hash_rate": 66179667,
    "height": 1310423,
    "incoming_connections_count": 0,
    "outgoing_connections_count": 5,
    "start_time": 1494822692,
    "status": "OK",
    "target": 120,
    "target_height": 0,
    "testnet": false,
    "top_block_hash": "76f9e85d62415312758bc09e0b9b48fd2b005231ad1eee435a8081e551203f82",
    "tx_count": 1219048,
    "tx_pool_size": 2,
    "white_peerlist_size": 1000
  },
  "status": "success"
}
```

#### api/outputsblocks

Search for our outputs in last few blocks (up to 5 blocks), using provided address and viewkey.


```bash
# testnet address
curl  -w "\n" -X GET http://127.0.0.1:8081/api/outputsblocks?address=9sDyNU82ih1gdhDgrqHbEcfSDFASjFgxL9B9v5f1AytFUrYsVEj7bD9Pyx5Sw2qLk8HgGdFM8qj5DNecqGhm24Ce6QwEGDi&viewkey=807079280293998634d66e745562edaaca45c0a75c8290603578b54e9397e90a&limit=5&mempool=1
```

Example result:

```json
{
{
  "data": {
    "address": "0182d5be0f708cecf2b6f9889738bde5c930fad846d5b530e021afd1ae7e24a687ad50af3a5d38896655669079ad0163b4a369f6c852cc816dace5fc7792b72f",
    "height": 960526,
    "limit": "5",
    "mempool": true,
    "outputs": [
      {
        "amount": 33000000000000,
        "block_no": 0,
        "in_mempool": true,
        "output_idx": 1,
        "output_pubkey": "2417b24fc99b2cbd9459278b532b37f15eab6b09bbfc44f9d17e15cd25d5b44f",
        "payment_id": "",
        "tx_hash": "9233708004c51d15f44e86ac1a3b99582ed2bede4aaac6e2dd71424a9147b06f"
      },
      {
        "amount": 2000000000000,
        "block_no": 960525,
        "in_mempool": false,
        "output_idx": 0,
        "output_pubkey": "9984101f5471dda461f091962f1f970b122d4469077aed6b978a910dc3ed4576",
        "payment_id": "0000000000000055",
        "tx_hash": "37825d0feb2e96cd10fa9ec0b990ac2e97d2648c0f23e4f7d68d2298996acefd"
      },
      {
        "amount": 96947454120000,
        "block_no": 960525,
        "in_mempool": false,
        "output_idx": 1,
        "output_pubkey": "e4bded8e2a9ec4d41682a34d0a37596ec62742b28e74b897fcc00a47fcaa8629",
        "payment_id": "0000000000000000000000000000000000000000000000000000000000001234",
        "tx_hash": "4fad5f2bdb6dbd7efc2ce7efa3dd20edbd2a91640ce35e54c6887f0ee5a1a679"
      }
    ],
    "viewkey": "807079280293998634d66e745562edaaca45c0a75c8290603578b54e9397e90a"
  },
  "status": "success"
}
```

#### api/emission

```bash
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/emission"
```

```json
{
  "data": {
    "blk_no": 1313969,
    "coinbase": 14489473877253413000,
    "fee": 52601974988641130
  },
  "status": "success"
}
```

Emission only works when the emission monitoring thread is enabled.

#### api/version

```bash
curl  -w "\n" -X GET "http://127.0.0.1:8081/api/version"
```

```json
{
  "data": {
    "api": 65536,
    "blockchain_height": 1357031,
    "git_branch_name": "update_to_current_monero",
    "last_git_commit_date": "2017-07-25",
    "last_git_commit_hash": "a549f25",
    "monero_version_full": "0.10.3.1-ab594cfe"
  },
  "status": "success"
}
```

api number is store as `uint32_t`. In this case `65536` represents
major version 1 and minor version 0.
In JavaScript to get these numbers, one can do as follows:

```javascript
var api_major = response.data.api >> 16;
var api_minor = response.data.api & 0xffff;
```

#### api/rawblock/<block_number|block_hash>

Return raw json block data, as represented in Monero.

```bash
curl  -w "\n" -X GET "http://139.162.32.245:8081/api/rawblock/1293257"
```

Example result not shown.

#### api/rawtransaction/<tx_hash>

Return raw json tx data, as represented in Monero.

```bash
curl  -w "\n" -X GET "http://139.162.32.245:8081/api/rawtransaction/6093260dbe79fd6277694d14789dc8718f1bd54457df8bab338c2efa3bb0f03d"
```

Example result not shown.

## Other monero examples

Other examples can be found on  [github](https://github.com/moneroexamples?tab=repositories).
Please know that some of the examples/repositories are not
finished and may not work as intended.

## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

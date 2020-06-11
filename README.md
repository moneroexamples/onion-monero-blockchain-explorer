# Equilibria Blockchain Explorer

## Onion Monero Blockchain Explorer features

The key features of the Onion Monero Blockchain Explorer are:

 - open sourced,
 - made fully in C++,
 - showing encrypted payments ID,
 - showing ring signatures,
 - showing transaction extra field,
 - showing public components of Equilibria addresses,
 - decoding which outputs and mixins belong to the given Monero address and viewkey,
 - detailed information about ring members, such as, their age, timescale and their ring sizes,
 - showing number of amount output indices,
 - support Equilibria testnet and stagnet networks,
 - can provide total amount of all miner fees,
 - check oracle nodes


## Compilation on Ubuntu 16.04/18.04

##### Compile latest Equilibria version

Download and compile recent Equilibria into your home folder:

```bash
# first install monero dependecines
sudo apt update

sudo apt install git build-essential cmake libboost-all-dev miniupnpc libunbound-dev graphviz doxygen libunwind8-dev pkg-config libssl-dev libcurl4-openssl-dev libgtest-dev libreadline-dev libzmq3-dev libsodium-dev libpcsclite-dev

# go to home folder
cd ~

git clone --recursive https://github.com/EquilibriaCC/equilibria

cd equilibria/

# checkout last monero version
git checkout -b last_release v5.0.3

make
```

##### Compile and run the explorer

Once the Equilibria is compiled, the explorer can be downloaded and compiled
as follows:

```bash
# go to home folder if still in ~/monero
cd ~

# download the source code
git clone https://github.com/Equilibria/Explorer

# enter the downloaded sourced code folder
cd Explorer

# make a build folder and enter it
mkdir build && cd build

# create the makefile
cmake ..

make
```


To run it:
```
./e_exp
```

Equilibria Blockchain Explorer:
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
                                        e.g., ~/.equilibria/lmdb
  --ssl-crt-file arg                    path to crt file for ssl (https)
                                        functionality
  --ssl-key-file arg                    path to key file for ssl (https)
                                        functionality
  -d [ --deamon-url ] arg (=http:://127.0.0.1:9231)
                                        Monero deamon url
```

## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.


## Originally From 

https://github.com/moneroexamples/onion-monero-blockchain-explorer

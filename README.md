# Onion Monero Blockchain Explorer

Curently available Monero blockchain explorer websites have several limitations which are of special importance to privacy-oriented users:

 - they use JavaScript,
 - have images which might be used for [cookieless tracking](http://lucb1e.com/rp/cookielesscookies/),
 - track users activates through google analytics,
 - are closed sourced,
 - are not available as hidden services,
 - provide only basic search capabilities,
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

Clearnet versions:
 - [https://xmrchain.net/](https://xmrchain.net/) - https enabled and most up to date.
 - [http://explore.MoneroWorld.com](http://explore.moneroworld.com) - same as above.
 - [http://blox.supportxmr.com/](http://blox.supportxmr.com/)
 - [https://explorer.xmr.my/](https://explorer.xmr.my/)
 
Clearnet testnet Monero version - - https enabled:
  
 - [https://testnet.xmrchain.com/](https://testnet.xmrchain.com/)

Tor users - down for now:
 
 - [http://3ccmeg4dunrl7h3i.onion/](http://3ccmeg4dunrl7h3i.onion/)

i2p users (main Monero network) - down for now:

 - [http://monerotools.i2p](http://monerotools.i2p)

 
## Onion Monero Blockchain Explorer features

The key features of the Onion Monero Blockchain Explorer are:

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

Instruction for Monero 0.10.1 compilation and Monero headers and libraries setup are
as shown here:
 - [Compile Monero 0.9 on Ubuntu 16.04 x64](https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04)
 - [lmdbcpp-monero](https://github.com/moneroexamples/lmdbcpp-monero.git) (optional)

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
to the blockchain. Also, by default it is 10 blocks behind the current blockchain height
to minimize indexing/saving orphaned blocks.

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

## The explorer's command line options

```
./xmrblocks -h
xmrblocks, start Onion Monero Blockchain Explorer:
  -h [ --help ] [=arg(=1)] (=0)         produce help message
  -t [ --testnet ] [=arg(=1)] (=0)      use testnet blockchain
  --enable-pusher [=arg(=1)] (=0)       enable pushing signed tx
  -p [ --port ] arg (=8081)             default port
  -b [ --bc-path ] arg                  path to lmdb blockchain
  --ssl-crt-file arg                    A path to crt file for ssl (https) 
                                        functionality
  --ssl-key-file arg                    A path to key file for ssl (https) 
                                        functionality
  -c [ --custom-db-path ] arg           path to the custom lmdb database used 
                                        for searching things
  -d [ --deamon-url ] arg (=http:://127.0.0.1:18081)
                                        monero address string
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

## Enable transaction pusher

By default, the tx pusher is disabled. The pushing will not work, but tx checking and inspecting will. 

To enable pushing the txs, use flag `--enable-pusher`, e.g.:

```bash
./xmrblocks --enable-pusher
```
 
Note: There has been a number of issues with compatibility of tx's binary data between different Monero versions
and operating systems. Unless you are using latest development version of Monero and the explorer has been compiled
against the lastest version, pushing and checking unsigined and signed tx data
 might not work due to incompatibilities in binary data.

## Example screenshot

![Onion Monero Blockchain Explorer](https://raw.githubusercontent.com/moneroexamples/onion-monero-blockchain-explorer/master/screenshot/screenshot_01.jpg)


## Other monero examples

Other examples can be found on  [github](https://github.com/moneroexamples?tab=repositories).
Please know that some of the examples/repositories are not
finished and may not work as intended.

## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

Some Monero are also welcome:
```
48daf1rG3hE1Txapcsxh6WXNe9MLNKtu7W7tKTivtSoVLHErYzvdcpea2nSTgGkz66RFP4GKVAsTV14v6G3oddBTHfxP6tU
```

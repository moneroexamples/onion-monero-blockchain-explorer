# Onion Monero Blockchain Explorer

Currently available Monero blockchain explorer websites have several limitations which are of special importance to privacy-oriented users:

 - they use JavaScript,
 - have images which might be used for [cookieless tracking](http://lucb1e.com/rp/cookielesscookies/),
 - track users activates through google analytics,
 - are closed sourced,
 - are not available as hidden services,
 - provide only basic search capabilities,
 - do not support Monero testnet

In this example, these limitations are addressed by development of
an Onion Monero Blockchain Explorer. The example not only shows how to use Monero C++ libraries,
 but also demonstrates how to use:

 - [crow](https://github.com/ipkn/crow) - C++ micro web framework
 - [mstch](https://github.com/no1msd/mstch) - C++ {{mustache}} templates
 - [json](https://github.com/nlohmann/json) - JSON for Modern C++
 - [date](https://github.com/HowardHinnant/date) - C++ date and time library
 - [fmt](https://github.com/fmtlib/fmt) - Small, safe and fast string formatting library

## Addresses

Tor users:
 
 - [http://dvwae436pd7nt4bc.onion](http://dvwae436pd7nt4bc.onion)

Clearnet versions:
 - [http://139.162.32.245:8081/](http://139.162.32.245:8081/) - bleeding edge, no https.
 - [https://xmrchain.net/](https://xmrchain.net/) - https enabled.
 - [https://monerohash.com/explorer/](https://monerohash.com/explorer/) - nice looking one, https enabled. 
 - [http://blox.supportxmr.com/](http://blox.supportxmr.com/) - no https.
 - [https://explorer.xmr.my/](https://explorer.xmr.my/) - https enabled.
  
Clearnet testnet Monero version:

 - [http://139.162.32.245:8082/](http://139.162.32.245:8082/) - bleeding edge, no https.
 - [https://testnet.xmrchain.com/](https://testnet.xmrchain.com/) - https enabled.
   
i2p users (main Monero network) - down for now:

 - [http://monerotools.i2p](http://monerotools.i2p)

   
Clearnet testnet priority nodes:

 - 23.228.193.90 - /opt/monero/monerod --testnet --add-priority-node 23.228.193.90
 - 62.210.104.109 - /opt/monero/monerod --testnet --add-priority-node 62.210.104.109  

Monero tor nodes:

 - 7kome2dwgre4wll6.onion - `/opt/monero/monero-wallet-cli --daemon-host 7kome2dwgre4wll6.onion` 
 - o6nvntbo3qsn36dm.onion - `/opt/monero/monero-wallet-cli --daemon-host o6nvntbo3qsn36dm.onion`
  
## Onion Monero Blockchain Explorer features

The key features of the Onion Monero Blockchain Explorer are:

 - no javascript, no cookies, no web analytics trackers, no images,
 - open sourced,
 - made fully in C++,
 - the only explorer showing encrypted payments ID,
 - the only explorer showing ring signatures,
 - the only explorer showing transaction extra field,
 - the only explorer showing public components of Monero addresses,
 - the only explorer that can show which outputs and mixins belong to the given Monero address and viewkey,
 - the only explorer that can be used to prove that you send Monero to someone,
 - the only explorer showing detailed information about mixins, such as, mixins'
 age, timescale, mixin of mixins,
 - the only explorer showing number of amount output indices,
 - the only explorer supporting Monero testnet network,
 - the only explorer providing tx checker and pusher for online pushing of transactions,
 - the only explorer allowing to inspect encrypted key images file and output files.
 - the only explorer able to estimate possible spendings based on address and viewkey.

## Prerequisite

Instruction for Monero 0.10.3 compilation and Monero headers and libraries setup are
as shown here:
 - [Compile Monero on Ubuntu 16.04 x64](https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04)
 - [lmdbcpp-monero](https://github.com/moneroexamples/lmdbcpp-monero.git) (optional)

## Compile and run the explorer

##### Monero headers and libraries setup

The Onion Explorer uses Monero C++ libraries and headers. 
 Instructions how to download source files and compile Monero,
 setup header and library files are presented here:

- https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04 (Ubuntu 16.04)
- https://github.com/moneroexamples/compile-monero-09-on-arch-linux (Arch Linux)

##### Compile and run 
Once the Monero is compiled and setup, the explorer can be downloaded and compiled
as follows:

```bash
# download the source code
git clone https://github.com/moneroexamples/onion-monero-blockchain-explorer.git

# enter the downloaded sourced code folder
cd onion-monero-blockchain-explorer

# make ~/Downloads forlder if you dont have it
# time zone library that explorer is using, puts there
# its database of time zone offsets

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
  -h [ --help ] [=arg(=1)] (=0)         produce help message
  -t [ --testnet ] [=arg(=1)] (=0)      use testnet blockchain
  --enable-pusher [=arg(=1)] (=0)       enable pushing signed tx
  --enable-key-image-checker [=arg(=1)] (=0)
                                        enable key images file checker
  --enable-output-key-checker [=arg(=1)] (=0)
                                        enable outputs key file checker
  --enable-autorefresh-option [=arg(=1)] (=0)
                                        enable users to have the index page on 
                                        autorefresh
  -p [ --port ] arg (=8081)             default port
  -b [ --bc-path ] arg                  path to lmdb blockchain
  --ssl-crt-file arg                    A path to crt file for ssl (https) 
                                        functionality
  --ssl-key-file arg                    A path to key file for ssl (https) 
                                        functionality
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


## Other Monero examples

Other examples can be found on  [github](https://github.com/moneroexamples?tab=repositories).
Please know that some of the examples/repositories are not
finished and may not work as intended.

## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.


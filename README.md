# Onion Monero Blockchain Explorer

Two Monero blockchain explorer exist in the clearnet. Although useful,
their limitations are that they use JavaScript, have images
([might be used for coockless tracking](http://lucb1e.com/rp/cookielesscookies/)),
track users activates through google analytics, are not open sourced, and are not
 available as hidden services. These things are of importance
 for privacy-oriented users.

 In this example, these limitations are addressed. Specifically,
 an Onion Monero Blockchain Explorer is developed. It is build in C++,
 and it not only shows how to use Monero C++ libraries, but also demonstrates how to
 use:

  - [crow](https://github.com/ipkn/crow) - C++ micro web framework
  - [lmdb++](https://github.com/bendiken/lmdbxx) - C++ wrapper for the LMDB
  - [mstch](https://github.com/no1msd/mstch) - C++ {{mustache}} templates
  - [rapidjson](https://github.com/miloyip/rapidjson) - C++ JSON parser/generator



## Onion Monero Blockchain Explorer features

 - no javascript, no web analytics trackers, no images, i.e., no user tracking
 - open source which allows everyone to check its source code, fork it, contribute
 - made fully in C++ allowing for seamless integration with Monero
 - does not use RPC calls, except to get mempool data, which improves its performance










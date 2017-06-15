//
// Created by mwo on 6/11/15.
//

#include "CmdLineOptions.h"


namespace xmreg
{
    /**
     * Take the acc and *avv[] from the main() and check and parse
     * all the options given
     */
    CmdLineOptions::CmdLineOptions(int acc, const char *avv[]) {

        positional_options_description p;

        p.add("txhash", -1);

        options_description desc(
                "xmrblocks, Onion Monero Blockchain Explorer");

        desc.add_options()
                ("help,h", value<bool>()->default_value(false)->implicit_value(true),
                 "produce help message")
                ("testnet,t", value<bool>()->default_value(false)->implicit_value(true),
                 "use testnet blockchain")
                ("enable-pusher", value<bool>()->default_value(false)->implicit_value(true),
                 "enable signed transaction pusher")
                ("enable-mixin-details", value<bool>()->default_value(false)->implicit_value(true),
                 "enable mixin details for key images, e.g., timescale, mixin of mixins, in tx context")
                ("enable-key-image-checker", value<bool>()->default_value(false)->implicit_value(true),
                 "enable key images file checker")
                ("enable-output-key-checker", value<bool>()->default_value(false)->implicit_value(true),
                 "enable outputs key file checker")
                ("enable-json-api", value<bool>()->default_value(true),
                 "enable JSON REST api")
                ("enable-tx-cache", value<bool>()->default_value(false)->implicit_value(true),
                 "enable caching of transaction details")
                ("show-cache-times", value<bool>()->default_value(false)->implicit_value(true),
                 "show times of getting data from cache vs no cache")
                ("enable-block-cache", value<bool>()->default_value(false)->implicit_value(true),
                 "enable caching of block details")
                ("enable-autorefresh-option", value<bool>()->default_value(false)->implicit_value(true),
                 "enable users to have the index page on autorefresh")
                ("enable-emission-monitor", value<bool>()->default_value(false)->implicit_value(true),
                 "enable Monero total emission monitoring thread")
                ("port,p", value<string>()->default_value("8081"),
                 "default explorer port")
                ("testnet-url", value<string>()->default_value(""),
                 "you can specify testnet url, if you run it on mainnet. link will show on front page to testnet explorer")
                ("mainnet-url", value<string>()->default_value(""),
                 "you can specify mainnet url, if you run it on testnet. link will show on front page to mainnet explorer")
                ("no-blocks-on-index", value<string>()->default_value("10"),
                 "number of last blocks to be shown on index page")
                ("mempool-info-timeout", value<string>()->default_value("5000"),
                 "maximum time, in milliseconds, to wait for mempool data for the front page")
                ("mempool-refresh-time", value<string>()->default_value("10"),
                 "time, in seconds, for each refresh of mempool state")
                ("bc-path,b", value<string>(),
                 "path to lmdb folder of the blockchain, e.g., ~/.bitmonero/lmdb")
                ("ssl-crt-file", value<string>(),
                 "path to crt file for ssl (https) functionality")
                ("ssl-key-file", value<string>(),
                 "path to key file for ssl (https) functionality")
                ("deamon-url,d", value<string>()->default_value("http:://127.0.0.1:18081"),
                 "Monero deamon url");


        store(command_line_parser(acc, avv)
                          .options(desc)
                          .positional(p)
                          .run(), vm);

        notify(vm);

        if (vm.count("help"))
        {
            if (vm["help"].as<bool>())
                cout << desc << "\n";
        }
    }

    /**
     * Return the value of the argument passed to the program
     * in wrapped around boost::optional
     */
    template<typename T>
    boost::optional<T>
    CmdLineOptions::get_option(const string & opt_name) const
    {

        if (!vm.count(opt_name))
        {
            return boost::none;
        }

        return vm[opt_name].as<T>();
    }


    // explicit instantiations of get_option template function
    template  boost::optional<string>
    CmdLineOptions::get_option<string>(const string & opt_name) const;

    template  boost::optional<bool>
            CmdLineOptions::get_option<bool>(const string & opt_name) const;

    template  boost::optional<size_t>
            CmdLineOptions::get_option<size_t>(const string & opt_name) const;

}

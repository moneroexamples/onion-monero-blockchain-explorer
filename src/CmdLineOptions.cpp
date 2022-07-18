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
                ("stagenet,s", value<bool>()->default_value(false)->implicit_value(true),
                 "use stagenet blockchain")
                ("enable-pusher", value<bool>()->default_value(false)->implicit_value(true),
                 "enable signed transaction pusher")
                ("enable-randomx", value<bool>()->default_value(false)->implicit_value(true),
                 "enable generation of randomx code")
                ("enable-mixin-details", value<bool>()->default_value(false)->implicit_value(true),
                 "enable mixin details for key images, e.g., timescale, mixin of mixins, in tx context")
                ("enable-key-image-checker", value<bool>()->default_value(false)->implicit_value(true),
                 "enable key images file checker")
                ("enable-output-key-checker", value<bool>()->default_value(false)->implicit_value(true),
                 "enable outputs key file checker")
                ("enable-json-api", value<bool>()->default_value(false)->implicit_value(true),
                 "enable JSON REST api")
                ("enable-as-hex", value<bool>()->default_value(false)->implicit_value(true),
                 "enable links to provide hex represtations of a tx and a block")
                ("enable-autorefresh-option", value<bool>()->default_value(false)->implicit_value(true),
                 "enable users to have the index page on autorefresh")
                ("enable-emission-monitor", value<bool>()->default_value(false)->implicit_value(true),
                 "enable Monero total emission monitoring thread")
                ("port,p", value<string>()->default_value("8081"),
                 "default explorer port")
                ("bindaddr,x", value<string>()->default_value("0.0.0.0"),
                 "default bind address for the explorer")
                ("testnet-url", value<string>()->default_value(""),
                 "you can specify testnet url, if you run it on mainnet or stagenet. link will show on front page to testnet explorer")
                ("stagenet-url", value<string>()->default_value(""),
                 "you can specify stagenet url, if you run it on mainnet or testnet. link will show on front page to stagenet explorer")
                ("mainnet-url", value<string>()->default_value(""),
                 "you can specify mainnet url, if you run it on testnet or stagenet. link will show on front page to mainnet explorer")
                ("no-blocks-on-index", value<string>()->default_value("10"),
                 "number of last blocks to be shown on index page")
                ("mempool-info-timeout", value<string>()->default_value("5000"),
                 "maximum time, in milliseconds, to wait for mempool data for the front page")
                ("mempool-refresh-time", value<string>()->default_value("5"),
                 "time, in seconds, for each refresh of mempool state")
                ("concurrency,c", value<size_t>()->default_value(0),
                 "number of threads handling http queries. Default is 0 which means it is based you on the cpu")
                ("bc-path,b", value<string>(),
                 "path to lmdb folder of the blockchain, e.g., ~/.bitmonero/lmdb")
                ("ssl-crt-file", value<string>(),
                 "path to crt file for ssl (https) functionality")
                ("ssl-key-file", value<string>(),
                 "path to key file for ssl (https) functionality")
                ("daemon-login", value<string>(),
                 "Specify username[:password] for daemon RPC client")
                ("daemon-url,d", value<string>()->default_value("127.0.0.1:18081"),
                 "Monero daemon url")
                ("enable-mixin-guess", value<bool>()->default_value(false)->implicit_value(true),
                 "enable guessing real outputs in key images based on viewkey");


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

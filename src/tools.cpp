//
// Created by marcin on 5/11/15.
//

#include "tools.h"



namespace xmreg
{

    /**
     * Parse key string, e.g., a viewkey in a string
     * into crypto::secret_key or crypto::public_key
     * depending on the template argument.
     */
    template <typename T>
    bool
    parse_str_secret_key(const string& key_str, T& secret_key)
    {

        // hash and keys have same structure, so to parse string of
        // a key, e.g., a view key, we can first parse it into the hash
        // object using parse_hash256 function, and then copy the reslting
        // hash data into secret key.
        crypto::hash hash_;

        if(!parse_hash256(key_str, hash_))
        {
            cerr << "Cant parse a key (e.g. viewkey): " << key_str << endl;
            return false;
        }

        // crypto::hash and crypto::secret_key have basicly same
        // structure. They both keep they key/hash as c-style char array
        // of fixed size. Thus we can just copy data from hash
        // to key
        copy(begin(hash_.data), end(hash_.data), secret_key.data);

        return true;
    }

    // explicit instantiations of get template function
    template bool parse_str_secret_key<crypto::secret_key>(const string& key_str, crypto::secret_key& secret_key);
    template bool parse_str_secret_key<crypto::public_key>(const string& key_str, crypto::public_key& secret_key);
    template bool parse_str_secret_key<crypto::hash>(const string& key_str, crypto::hash& secret_key);

    /**
     * Get transaction tx using given tx hash. Hash is represent as string here,
     * so before we can tap into the blockchain, we need to pare it into
     * crypto::hash object.
     */
    bool
    get_tx_pub_key_from_str_hash(Blockchain& core_storage, const string& hash_str, transaction& tx)
    {
        crypto::hash tx_hash;
        parse_hash256(hash_str, tx_hash);

        try
        {
            // get transaction with given hash
            tx = core_storage.get_db().get_tx(tx_hash);
        }
        catch (const TX_DNE& e)
        {
            cerr << e.what() << endl;
            return false;
        }

        return true;
    }

    /**
     * Parse monero address in a string form into
     * cryptonote::account_public_address object
     */
    bool
    parse_str_address(const string& address_str,
                      account_public_address& address,
                      bool testnet)
    {

        if (!get_account_address_from_str(address, testnet, address_str))
        {
            cerr << "Error getting address: " << address_str << endl;
            return false;
        }

        return true;
    }


    /**
     * Return string representation of monero address
     */
    string
    print_address(const account_public_address& address, bool testnet)
    {
        return "<" + get_account_address_as_str(testnet, address) + ">";
    }

    string
    print_sig (const signature& sig)
    {
        stringstream ss;

        ss << "c: <" << epee::string_tools::pod_to_hex(sig.c) << "> "
           << "r: <" << epee::string_tools::pod_to_hex(sig.r) << ">";

        return ss.str();
    }

    /**
     * Check if a character is a path seprator
     */
    inline bool
    is_separator(char c)
    {
        // default linux path separator
        const char separator = PATH_SEPARARTOR;

        return c == separator;
    }



    /**
     * Remove trailinig path separator.
     */
    string
    remove_trailing_path_separator(const string& in_path)
    {
        string new_string = in_path;
        if (!new_string.empty() && is_separator(new_string[new_string.size() - 1]))
            new_string.erase(new_string.size() - 1);
        return new_string;
    }

    bf::path
    remove_trailing_path_separator(const bf::path& in_path)
    {
        string path_str = in_path.native();
        return bf::path(remove_trailing_path_separator(path_str));
    }


    string
    timestamp_to_str(time_t timestamp, const char* format)
    {

        const int TIME_LENGTH = 60;

        char str_buff[TIME_LENGTH];

        tm *tm_ptr;
        tm_ptr = localtime(&timestamp);

        size_t len;

        len = std::strftime(str_buff, TIME_LENGTH, format, tm_ptr);

        return string(str_buff, len);
    }


    ostream&
    operator<< (ostream& os, const account_public_address& addr)
    {
        os << get_account_address_as_str(false, addr);
        return os;
    }


    /*
     * Generate key_image of foran ith output
     */
    bool
    generate_key_image(const crypto::key_derivation& derivation,
                       const std::size_t i,
                       const crypto::secret_key& sec_key,
                       const crypto::public_key& pub_key,
                       crypto::key_image& key_img)
    {

        cryptonote::keypair in_ephemeral;

        if (!crypto::derive_public_key(derivation, i,
                                       pub_key,
                                       in_ephemeral.pub))
        {
            cerr << "Error generating publick key " << pub_key << endl;
            return false;
        }

        try
        {

            crypto::derive_secret_key(derivation, i,
                                      sec_key,
                                      in_ephemeral.sec);
        }
        catch(const std::exception& e)
        {
            cerr << "Error generate secret image: " << e.what() << endl;
            return false;
        }


        try
        {
            crypto::generate_key_image(in_ephemeral.pub,
                                       in_ephemeral.sec,
                                       key_img);
        }
        catch(const std::exception& e)
        {
            cerr << "Error generate key image: " << e.what() << endl;
            return false;
        }

        return true;
    }


    string
    get_default_lmdb_folder()
    {
        // default path to monero folder
        // on linux this is /home/<username>/.bitmonero
        string default_monero_dir = tools::get_default_data_dir();

        // the default folder of the lmdb blockchain database
        // is therefore as follows
        return default_monero_dir + string("/lmdb");
    }


    /*
     * Ge blockchain exception from command line option
     *
     * If not given, provide default path
     */
    bool
    get_blockchain_path(const boost::optional<string>& bc_path, bf::path& blockchain_path )
    {
        // the default folder of the lmdb blockchain database
        string default_lmdb_dir   = xmreg::get_default_lmdb_folder();

        blockchain_path = bc_path
                          ? bf::path(*bc_path)
                          : bf::path(default_lmdb_dir);



        if (!bf::is_directory(blockchain_path))
        {
            cerr << "Given path \"" << blockchain_path   << "\" "
                 << "is not a folder or does not exist" << " "
                 << endl;

            return false;
        }

        blockchain_path = xmreg::remove_trailing_path_separator(blockchain_path);

        return true;
    }


    /**
     * Rough estimate of block height from the time provided
     *
     */
    uint64_t
    estimate_bc_height(const string& date, const char* format)
    {
        const pt::ptime MONERO_START {gt::date(2014,04,18)};
        const uint64_t MONERO_BLOCK_TIME {60}; // seconds

        dateparser parser {format};

        if (!parser(date))
        {
           throw runtime_error(string("Date format is incorrect: ") + date);
        }

        pt::ptime requested_date = parser.pt;

        if (requested_date < MONERO_START)
        {
            return 0;
        }

        pt::time_duration td = requested_date - MONERO_START;

        return static_cast<uint64_t>(td.total_seconds()) / MONERO_BLOCK_TIME;
    }


    array<size_t, 5>
    timestamp_difference(uint64_t t1, uint64_t t2)
    {

        uint64_t timestamp_diff = t1 - t2;

        // calculate difference of timestamps from current block to the mixin one
        if (t2 > t1)
        {
            timestamp_diff = t2 - t1;
        }

        uint64_t time_diff_years = timestamp_diff / 31536000;

        timestamp_diff -=  time_diff_years * 31536000;

        uint64_t time_diff_days = timestamp_diff / 86400;

        timestamp_diff -=  time_diff_days * 86400;

        uint64_t time_diff_hours = timestamp_diff / 3600;

        timestamp_diff -=  time_diff_hours * 3600;

        uint64_t time_diff_minutes = timestamp_diff / 60;

        timestamp_diff -=  time_diff_minutes * 60;

        uint64_t time_diff_seconds = timestamp_diff ;

        return array<size_t, 5> {time_diff_years, time_diff_days,
                                 time_diff_hours, time_diff_minutes,
                                 time_diff_seconds};

    };


}

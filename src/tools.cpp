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
    auto a_time_point = chrono::system_clock::from_time_t(timestamp);

    try
    {
        auto utc          = date::to_utc_time(chrono::system_clock::from_time_t(timestamp));
        auto sys_time     = date::to_sys_time(utc);

        return date::format(format, date::floor<chrono::seconds>(sys_time));
    }
    catch (std::runtime_error& e)
    {
        cerr << "xmreg::timestamp_to_str: " << e.what() << endl;
        cerr << "Seems cant convert to UTC timezone using date libary. "
                "So just use local timezone." <<endl;

        return timestamp_to_str_local(timestamp, format);
    }
}


string
timestamp_to_str_local(time_t timestamp, const char* format)
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
get_default_lmdb_folder(bool testnet)
{
    // default path to monero folder
    // on linux this is /home/<username>/.bitmonero
    string default_monero_dir = tools::get_default_data_dir();

    if (testnet)
        default_monero_dir += "/testnet";


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
get_blockchain_path(const boost::optional<string>& bc_path,
                    bf::path& blockchain_path,
                    bool testnet)
{
    // the default folder of the lmdb blockchain database
    string default_lmdb_dir   = xmreg::get_default_lmdb_folder(testnet);

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


uint64_t
sum_money_in_outputs(const transaction& tx)
{
    uint64_t sum_xmr {0};

    for (const tx_out& txout: tx.vout)
    {
        sum_xmr += txout.amount;
    }

    return sum_xmr;
}

pair<uint64_t, uint64_t>
sum_money_in_outputs(const string& json_str)
{
    pair<uint64_t, uint64_t> sum_xmr {0, 0};

    cout << json_str << endl;


    json j;
    try
    {
       j = json::parse( json_str);
    }
    catch (std::invalid_argument& e)
    {
        cerr << "sum_money_in_outputs: " << e.what() << endl;
    }

    cout << j.dump() << endl;

    return sum_xmr;
};



uint64_t
sum_money_in_inputs(const transaction& tx)
{
    uint64_t sum_xmr {0};

    size_t input_no = tx.vin.size();

    for (size_t i = 0; i < input_no; ++i)
    {

        if(tx.vin[i].type() != typeid(cryptonote::txin_to_key))
        {
            continue;
        }

        // get tx input key
        const cryptonote::txin_to_key& tx_in_to_key
                = boost::get<cryptonote::txin_to_key>(tx.vin[i]);

        sum_xmr += tx_in_to_key.amount;
    }

    return sum_xmr;
}

pair<uint64_t, uint64_t>
sum_money_in_inputs(const string& json_str)
{
    pair<uint64_t, uint64_t> sum_xmr {0, 0};

    cout << json_str << endl;

    json j;
    try
    {
        j = json::parse( json_str);
    }
    catch (std::invalid_argument& e)
    {
        cerr << "sum_money_in_outputs: " << e.what() << endl;
    }

    cout << j.dump() << endl;

    return sum_xmr;
};

array<uint64_t, 2>
sum_money_in_tx(const transaction& tx)
{
    array<uint64_t, 2> sum_xmr;

    sum_xmr[0] = sum_money_in_inputs(tx);
    sum_xmr[1] = sum_money_in_outputs(tx);

    return sum_xmr;
};


array<uint64_t, 2>
sum_money_in_txs(const vector<transaction>& txs)
{
    array<uint64_t, 2> sum_xmr {0,0};

    for (const transaction& tx: txs)
    {
        sum_xmr[0] += sum_money_in_inputs(tx);
        sum_xmr[1] += sum_money_in_outputs(tx);
    }

    return sum_xmr;
};


uint64_t
sum_fees_in_txs(const vector<transaction>& txs)
{
    uint64_t fees_sum {0};

    for (const transaction& tx: txs)
    {
        fees_sum += get_tx_fee(tx);
    }

    return fees_sum;
}



vector<pair<txout_to_key, uint64_t>>
get_ouputs(const transaction& tx)
{
    vector<pair<txout_to_key, uint64_t>> outputs;

    for (const tx_out& txout: tx.vout)
    {
        if (txout.target.type() != typeid(txout_to_key))
        {
            continue;
        }

        // get tx input key
        const txout_to_key& txout_key
                = boost::get<cryptonote::txout_to_key>(txout.target);

        outputs.push_back(make_pair(txout_key, txout.amount));
    }

    return outputs;

};

vector<tuple<txout_to_key, uint64_t, uint64_t>>
get_ouputs_tuple(const transaction& tx)
{
    vector<tuple<txout_to_key, uint64_t, uint64_t>> outputs;

    for (uint64_t n = 0; n < tx.vout.size(); ++n)
    {

        if (tx.vout[n].target.type() != typeid(txout_to_key))
        {
            continue;
        }

        // get tx input key
        const txout_to_key& txout_key
                = boost::get<cryptonote::txout_to_key>(tx.vout[n].target);

        outputs.push_back(make_tuple(txout_key, tx.vout[n].amount, n));
    }

    return outputs;
};

uint64_t
get_mixin_no(const transaction& tx)
{
    uint64_t mixin_no {0};

    size_t input_no = tx.vin.size();

    for (size_t i = 0; i < input_no; ++i)
    {

        if(tx.vin[i].type() != typeid(cryptonote::txin_to_key))
        {
            continue;
        }

        // get tx input key
        const txin_to_key& tx_in_to_key
                = boost::get<cryptonote::txin_to_key>(tx.vin[i]);

        mixin_no = tx_in_to_key.key_offsets.size();

        // look for first mixin number.
        // all inputs in a single transaction have same number
        if (mixin_no > 0)
        {
            break;
        }
    }

    return mixin_no;
}
vector<uint64_t>
get_mixin_no(const string& json_str)
{
    vector<uint64_t> mixin_no;

    return mixin_no;
}

vector<uint64_t>
get_mixin_no_in_txs(const vector<transaction>& txs)
{
    vector<uint64_t> mixin_no;

    for (const transaction& tx: txs)
    {
        mixin_no.push_back(get_mixin_no(tx));
    }

    return mixin_no;
}


vector<txin_to_key>
get_key_images(const transaction& tx)
{
    vector<txin_to_key> key_images;

    size_t input_no = tx.vin.size();

    for (size_t i = 0; i < input_no; ++i)
    {

        if(tx.vin[i].type() != typeid(txin_to_key))
        {
            continue;
        }

        // get tx input key
        const txin_to_key& tx_in_to_key
                = boost::get<cryptonote::txin_to_key>(tx.vin[i]);

        key_images.push_back(tx_in_to_key);
    }

    return key_images;
}


bool
get_payment_id(const vector<uint8_t>& extra,
               crypto::hash& payment_id,
               crypto::hash8& payment_id8)
{
    payment_id = null_hash;
    payment_id8 = null_hash8;

    std::vector<tx_extra_field> tx_extra_fields;

    if(!parse_tx_extra(extra, tx_extra_fields))
    {
        return false;
    }

    tx_extra_nonce extra_nonce;

    if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
    {
        // first check for encrypted id and then for normal one
        if(get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
        {
            return true;
        }
        else if (get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
        {
            return true;
        }
    }

    return false;
}


bool
get_payment_id(const transaction& tx,
               crypto::hash& payment_id,
               crypto::hash8& payment_id8)
{
    return get_payment_id(tx.extra, payment_id, payment_id8);
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


string
read(string filename)
{
   if (!bf::exists(bf::path(filename)))
   {
       cerr << "File does not exist: " << filename << endl;
       return string();
   }

    std::ifstream t(filename);
    return string(std::istreambuf_iterator<char>(t),
                  std::istreambuf_iterator<char>());
}

pair<string, double>
timestamps_time_scale(const vector<uint64_t>& timestamps,
                      uint64_t timeN,
                      uint64_t resolution,
                      uint64_t time0)
{
    string empty_time =  string(resolution, '_');

    size_t time_axis_length = empty_time.size();

    uint64_t interval_length = timeN-time0;

    double scale = double(interval_length) / double(time_axis_length);

    for (const auto& timestamp: timestamps)
    {

        if (timestamp < time0 || timestamp > timeN)
        {
            cout << "Out of range" << endl;
            continue;
        }

        uint64_t timestamp_place = double(timestamp-time0)
                         / double(interval_length)*(time_axis_length - 1);

        empty_time[timestamp_place + 1] = '*';
    }

    return make_pair(empty_time, scale);
}

// useful reference to get epoch time in correct timezon
// http://www.boost.org/doc/libs/1_41_0/doc/html/date_time/examples.html#date_time.examples.seconds_since_epoch
time_t
ptime_to_time_t(const pt::ptime& in_ptime)
{
    static pt::ptime epoch(gt::date(1970, 1, 1));
    pt::time_duration::sec_type no_seconds = (in_ptime - epoch).total_seconds();
    return time_t(no_seconds);
}

bool
decode_ringct(const rct::rctSig& rv,
              const crypto::public_key pub,
              const crypto::secret_key &sec,
              unsigned int i,
              rct::key & mask,
              uint64_t & amount)
{
    crypto::key_derivation derivation;

    bool r = crypto::generate_key_derivation(pub, sec, derivation);

    if (!r)
    {
        cerr <<"Failed to generate key derivation to decode rct output " << i << endl;
        return false;
    }

    crypto::secret_key scalar1;

    crypto::derivation_to_scalar(derivation, i, scalar1);

    try
    {
        switch (rv.type)
        {
            case rct::RCTTypeSimple:
                amount = rct::decodeRctSimple(rv,
                                              rct::sk2rct(scalar1),
                                              i,
                                              mask);
                break;
            case rct::RCTTypeFull:
                amount = rct::decodeRct(rv,
                                        rct::sk2rct(scalar1),
                                        i,
                                        mask);
                break;
            default:
                cerr << "Unsupported rct type: " << rv.type << endl;
                return false;
        }
    }
    catch (const std::exception &e)
    {
        cerr << "Failed to decode input " << i << endl;
        return false;
    }

    return true;
}

bool
url_decode(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == '%')
        {
            if (i + 3 <= in.size())
            {
                int value = 0;
                std::istringstream is(in.substr(i + 1, 2));
                if (is >> std::hex >> value)
                {
                    out += static_cast<char>(value);
                    i += 2;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        else if (in[i] == '+')
        {
            out += ' ';
        }
        else
        {
            out += in[i];
        }
    }
    return true;
}

map<std::string, std::string>
parse_crow_post_data(const string& req_body)
{
    map<std::string, std::string> body;

    vector<string> vec;
    string tmp;
    bool result = url_decode(req_body, tmp);
    if (result)
    {
        boost::algorithm::split(vec, tmp, [](char x) {return x == '&'; });
        for(auto &it : vec)
        {
            auto pos = it.find("=");
            if (pos != string::npos)
            {
                body[it.substr(0, pos)] = it.substr(pos + 1);
            }
            else
            {
                break;
            }
        }
    }
    return body;
}

bool
get_dummy_account_keys(account_keys& dummy_keys, bool testnet)
{
    secret_key adress_prv_viewkey;
    secret_key adress_prv_spendkey;

    account_public_address dummy_address;

    if (!get_account_address_from_str(dummy_address,
                                 testnet,
                                 "4BAyX63gVQgDqKS1wmqNVHdcCNjq1jooLYCXsKEY9w7VdGh45oZbPLvN7y8oVg2zmnhECkRBXpREWb97KtfAcT6p1UNXm9K"))
    {
        return false;
    }


    if (!epee::string_tools::hex_to_pod("f238be69411631f35b76c5a9148b3b7e8327eb41bfd0b396e090aeba40235d01", adress_prv_viewkey))
    {
        return false;
    }

    if (!epee::string_tools::hex_to_pod("5db8e1d2c505f888e54aca15b1a365c8814d7deebc1a246690db3bf71324950d", adress_prv_spendkey))
    {
        return false;
    }


    dummy_keys = account_keys {
            dummy_address,
            adress_prv_spendkey,
            adress_prv_viewkey
    };

    return true;
}



// from wallet2::decrypt
string
decrypt(const std::string &ciphertext,
        const crypto::secret_key &skey,
        bool authenticated)
{

    const size_t prefix_size = sizeof(chacha8_iv)
                               + (authenticated ? sizeof(crypto::signature) : 0);
    if (ciphertext.size() < prefix_size)
    {
        cerr <<  "Unexpected ciphertext size" << endl;
        return {};
    }

    crypto::chacha8_key key;
    crypto::generate_chacha8_key(&skey, sizeof(skey), key);

    const crypto::chacha8_iv &iv = *(const crypto::chacha8_iv*)&ciphertext[0];

    std::string plaintext;

    plaintext.resize(ciphertext.size() - prefix_size);

    if (authenticated)
    {
        crypto::hash hash;
        crypto::cn_fast_hash(ciphertext.data(), ciphertext.size() - sizeof(signature), hash);
        crypto::public_key pkey;
        crypto::secret_key_to_public_key(skey, pkey);

        const crypto::signature &signature =
                *(const crypto::signature*)&ciphertext[ciphertext.size()
                                                       - sizeof(crypto::signature)];

        if (!crypto::check_signature(hash, pkey, signature))
        {
            cerr << "Failed to authenticate criphertext" << endl;
            return {};
        }

    }

    crypto::chacha8(ciphertext.data() + sizeof(iv),
                    ciphertext.size() - prefix_size,
                    key, iv, &plaintext[0]);

    return plaintext;
}

// based on
// crypto::public_key wallet2::get_tx_pub_key_from_received_outs(const tools::wallet2::transfer_details &td) const
public_key
get_tx_pub_key_from_received_outs(const transaction &tx)
{
    std::vector<tx_extra_field> tx_extra_fields;

    if(!parse_tx_extra(tx.extra, tx_extra_fields))
    {
        // Extra may only be partially parsed, it's OK if tx_extra_fields contains public key
    }

    // Due to a previous bug, there might be more than one tx pubkey in extra, one being
    // the result of a previously discarded signature.
    // For speed, since scanning for outputs is a slow process, we check whether extra
    // contains more than one pubkey. If not, the first one is returned. If yes, they're
    // checked for whether they yield at least one output
    tx_extra_pub_key pub_key_field;

    if (!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, 0))
    {
        return null_pkey;
    }

    public_key tx_pub_key = pub_key_field.pub_key;

    bool two_found = find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, 1);

    if (!two_found)
    {
        // easy case, just one found
        return tx_pub_key;
    }
    else
    {
        // just return second one if there are two.
        // this does not require private view key, as
        // its not needed for my use case.
        return pub_key_field.pub_key;
    }

    return null_pkey;
}

date::sys_seconds
parse(const std::string& str, string format)
{
    std::istringstream in(str);
    date::sys_seconds tp;
    in >> date::parse(format, tp);
    if (in.fail())
    {
        in.clear();
        in.str(str);
        in >> date::parse(format, tp);
    }
    return tp;
}

/**
* Check if given output (specified by output_index)
* belongs is ours based
* on our private view key and public spend key
*/
bool
is_output_ours(const size_t& output_index,
               const transaction& tx,
               const public_key& pub_tx_key,
               const secret_key& private_view_key,
               const public_key& public_spend_key)
{
    // public transaction key is combined with our viewkey
    // to create, so called, derived key.
    key_derivation derivation;

    if (!generate_key_derivation(pub_tx_key, private_view_key, derivation))
    {
        cerr << "Cant get dervied key for: "  << "\n"
             << "pub_tx_key: " << pub_tx_key  << " and "
             << "prv_view_key" << private_view_key << endl;

        return false;
    }


    // get the tx output public key
    // that normally would be generated for us,
    // if someone had sent us some xmr.
    public_key pubkey;

    derive_public_key(derivation,
                      output_index,
                      public_spend_key,
                      pubkey);

    //cout << "\n" << tx.vout.size() << " " << output_index << endl;

    // get tx output public key
    const txout_to_key tx_out_to_key
            = boost::get<txout_to_key>(tx.vout[output_index].target);


    if (tx_out_to_key.key == pubkey)
    {
        return true;
    }

    return false;
}


bool
get_real_output_for_key_image(const key_image& ki,
                              const transaction& tx,
                              const secret_key& private_view_key,
                              const public_key& public_spend_key,
                              uint64_t output_idx,
                              public_key output_pub_key)
{



    return false;
}


bool
make_tx_from_json(const string& json_str, transaction& tx)
{

    json j;

    cout << json_str << endl;

    const char * raw = R"V0G0N(
{
  "version": 2,
  "unlock_time": 0,
  "vin": [ {
      "key": {
        "amount": 0,
        "key_offsets": [ 37042, 15496, 14068, 3115, 1556
        ],
        "k_image": "b47a493cded4e1416a8b189ceb549eb49cf7a0c7981e45b8e24a0fb292a1401c"
      }
    }
  ],
  "vout": [ {
      "amount": 0,
      "target": {
        "key": "5a93738154ab6b9bac75755ee08d843abaf1a4be0d6ac1f140292045bd79c04b"
      }
    }, {
      "amount": 0,
      "target": {
        "key": "310691eda5bf73cd66d87e956b9997932038f04be5ed8e83e299d104a0210b0f"
      }
    }
  ],
  "extra": [ 1, 204, 81, 140, 246, 217, 169, 194, 118, 216, 8, 57, 124, 64, 250, 53, 57, 18, 178, 241, 110, 48, 80, 57, 228, 63, 151, 222, 101, 129, 6, 237, 208
  ],
  "rct_signatures": {
    "type": 1,
    "txnFee": 32494198425,
    "ecdhInfo": [ {
        "mask": "27d5f4a686c2eb50d474fdc573571d9d10a30ecfbdf1bec46c832cf105320d0b",
        "amount": "af4bf293f4414a5572bb729249b5dbd973c33a804ca14f5e7b2e0ec3ddd7850c"
      }, {
        "mask": "8d17e36348486a83192b2790dd08008358e9b01948cdabaf3b0371517e330307",
        "amount": "c3fec30c14563c39309760ec51e4dc69c1864845e3bc4bab60a5dac69a57c509"
      }],
    "outPk": [ "b31757b8fde16dc6befa8b33206e358889dbe72702454063beef1bb2c95ee5a2", "2e97892f3e851780a307963536df2509f6e0d87fe2c1af74f7b911869f3cfc72"]
  },
  "rctsig_prunable": {
    "rangeSigs": [ {
        "asig": "acc009be15e00e621326935791421725382e232185a688b01d056c47d724dbd29edc40e94297207f5fd258a10ac837cac961bd8384e982d25dfa01e991698f2553b3a9ff8705d053e2f7b0dc70606d8bc0ecdd5ead82f34cc62d3a77d1427baa9be419a48c5bd209c02ab48f7084199cb4c441298c9ccf0b38106ffb88d03b5923afc33a1be9f019a22116acb05e65a185939e69a5015456d168ef54a11f2b69afc0ff3c20c158f87b2a59af62288120be1a21e271ef994957b7e9b0ffe4b7a0cbf3be718681aae240ae6a5d37ae1dcfcdbd7a7f7c03bfd7b5a11bc2fca12a1371a6bcda5b100060ad5d553f3f2f9a1cbf175fff2f0a33a8354c56e6f99a452187087e5fbb58f436c147e640554ddea46b1323973dba18f4ae63f2c9c3eecd7467a9b1acbc60f8404224823aaa0d57f750b61450d0d6d43b4cdb41acf9f3f64f4fd372eeaa7dfe0165802126c7b6fafd0951f35ad82f6913a3356d9bb8cf0dbd9f5fd138d10e921614106939458190d73c7654b432a7946359a45171ad2355afd245c23024a81bc5ac60073312b1f79e4a8817c9867d3e5730193f36b8d0d118ea93cd21ff4329039e6b33a46f79b9f177dba8aacebdead3ef8e131256f545d9df5a44d3ea6c5f6008f226a8cd39d343eed019143c89b808d7bf531f54c5416ef7d3c060be0e81cedca2fccb79d5fb6f67bd953f8576327814b7d602cc1b1c146b999a70ef49851aa4cc533bc352a7233103198b92a4260e77cdeb268c8bd9c4ab471b3692114db0dc5d161f056553c432a8ce9d4864bf36954a851829db9e6a7fd76cac766eae83c4d2051ee4caca201ebcc04c35cffff3e599837ea64978b7a87358a550efb4652619bb2e9b837fb55f1c32175394f7b5ce4d54ef8057da94cf8c789941ef4bd759a0528b22c0d90ca3025e25fe5f3653d2227b9a15381c2f6429b5f610cc08353faacfcff843619e814c8f3ba9f17b0c68dd589a56a94604b04ea7eeeae8a74a1be18c1a1dd6cbd9cf3c08d81b359cc1ce0513bb75e12f18b8ce55eefd3c5940caf706b10b3d0810c63d5e1f9dfae7cb0458b2b28497f21cc7eeacf0237c35402d8f8329c50b8f90e6af2a57aa2296e9f806362348e53293d65daed650f794da7e5c8b68f40e234a06e927ae741ac9f9e0f8a96f5d9ad50009534e355f71b2815580f96142a0f5bc1615cce9325f5040c3bb3c8796413b3e04fbe1853daca9583fc8ab8127c707fcbe44133d9851d3ab43888e5b66534479b02d518e666bd601e934dd26117dc0678f808b0fa75337bb059e67fe527a582eb0b827132dc788499e25165271bda8d08f27d8a8989abc09a1af87b485ba4181d5a5897f8ed09a7e28cc195989edc4e861f08716dd5bbd4870b52ce09c0ed75725cfbc0780dfb1f7ce94552f1a6c3a4aac360becc4f69daf60d9c7e221ec4c520d3b880687798e42835695f120fcb0d7bdb698ce98bcec9f3d6cdd4e33f296ffa5d50ff5945a6c59e21ed153b4032a94ce34995e4bf7fb75e7e7bc60a132cbc303674e460a2479e5a1dedbce1e8efda5fbee12565e16126ec16165659630ee3177de9de92352d7e86a87730ed89118f500f244860da66dd85c515d1dcedec48a8f2f6185b99f67389c48a59ba41691a0b6eb1a0952fde2d604e896f347585142c7a61b63bec82d20d464e2ef4710dd84750bada4018f448a225e3f269467478db0f04a3cd28bd9cb617884596ea78197bafa0f473548e2a606206de08b63a0a65c7515552efee55c4d1812dcbaedfbc7a037e8f82074ecfae9e6637bc4795bd6490a04a336e18b3c6e31d57acea7cb867cb265125f1f68c92f7d43c73433b9aef942843f1b92fa18a4921bca0f4a685a80fc860eb282412547e18670026c6255f8ad1334daf7c791675b18708d328b5b98bde03d9edd2ab7b19bd8dce40e5f5ad3da1b2bcf57c00001d14d4cb39827173cd94fa9bf75414193382295a4426eb324c442ed490eeb91a02561d16393180fe79b6be1d8d3d199a0655be325312765e768901a716552403695c9812808a676ab2e9ef401fa1759e9a9cc63dcd5daa93037b9e8bac7956bd28c7312bf7e4f6a6e43b8e7a0c877edb90138fdeeeab59face12e4b8f5b1fb9c47c0c331cbd9eea7609d4961816c2dc81b41c03ffaa055d306df7e70c43406641449ec7a76a684564263690b015d94210611fe7721e9450844d2a202a075f3ac0de265bb9a592a725eb970456ee4349e2b0220df7c1e6841411a62b3230a8268edb24d6ad812436338a245c2ea17d3b89374488e287c6c321e7f512342ef21e0902912cc1c8cf2f1b3706fbf51ab14076dd548abe29e287ab7965b82e1a70a170e8176df42e908b108d610e68207c3b83eed12dc4a18cf0536dce2f2461c5972e06e8d21b3c36cd82a7377eae0215276d3b9b3b20ccf08655004bc4e102820099f96b898d04648846f43dae03828232905d19b3069b8afb463da87f9b2bbb942e7e7a1ea5efbcb4f10cfa0616b43bdb6a7fe23836c0c9b75b1efa79f4822eba524c98c884b5fea5a5d5f96651d0c7a04fea0375c1c09c8108f93bac0b4b51a691929e62388f0d371e01f34b721720c2b5be37343df28f9cf53ee6ebb7553ec448392dc85c211bb4b4f9e69bf8b6817690cd658974b1b3c869d666f8cb9550e8d11502df3c6af303444c5951c2f0f2cbe8bf05ea7604bf0b4fb85b629ccb85b6479d79b4fc878c67444c508416e4d7e72f8a131d13be4d7f4646e73c8ab4b2f6c4c0725dc0d7354edd23bc55875993b14eef91c11f99c9260355c2194a5d8a84bc17979ce048fd81f308115137aed2407754ebe0d34314be133d2b1a2bd7a41826d1b1d4c1392a4ad6dfce715cc27e79a75ee69fe852de963385a6dbdb7dff3d0d63179af15f35340f90137181ec7f1bd16ba216f63c2d08859ca8acf8665be69aef8226ab89d03a13d41f6f447c41772862dc1e7415210106283780dd441253bfad37a1713ec3235ad777ccbc54ee5068a2783eaf075b088d60fa232f864ca45d0a80395a3e78d6b05ae299c6b65a1f813b5db8e5ebdb08c50670ffeb0855675710a5e1b8b73271cc6cc22db2056b8593d0a49d2adb3303862fe2493d95d3d8708f87b02f3f249536bcec83c0bbf2658b3f4d7c4974010c437f57e9dcc88c64dfad1adb03cb6fe607206aec89b8ae0a1827bef91d135805eed3a1852eb3c926afe3df81d5dd67bb1dfe156e035317444c32542f61d55f0b71fa64bfdb802240ea0994114cb05e82bb7e86786ea9476ac709ab98b7a1fa0288a284b5b5f596acdaef60e12015bea3d809a99f943af387a5ad43c112444e052dbb1a8351dafec798779897be89662382ee4d4c0411300c8a9e416430330e0085fafa3366a7264ab262db17b06c5731389dda0a5d582dfb6c2a7baaa5806a051c585757692cb24d2b83cdc0d4a4edc59adce73aaa3cd522269fa207423a3e03683adb7950894c538156554f5c552b9c3562f0a73f587a8a75c7f0dec5ae440f001a445c83664189b5e4af5967158ebd4bb3b8fae53239c8b950d6851d1494030b117f1c9e95caa8c5f385f7b3fa5b48487797393df98054758113b2abdfe00fe6bf8ba7b74dd8dac68bb3fb20af905ec33130cf50156804b20f2d13b930b0084bd9bdc9014ae2cc75412b1f95b4eab609eddfe28166fbdb948571f2b36ae6009400b02cd5ae096197f10351b84e74e9e783c1f2a6f0c136f9e38a4fe002c20500773d46c469a3ca934d81bccbff8232cf0d4b400cb80305be5bfd4ab9357b0be22603d883e37ad6c918cc906bfcff37193a1701d964e035003591f4020064060ea5d74b45dff2d65d817b509ad1fa516b34599a6946fb7a13248f987070610863e3209a18ae85edaeee829d6f598a8fbdc14b64a6e8ac99a55e9f539303dc0123bd26e503ec964d8e92c248709eac666820945121d7e029b63f98015e872901046c841977ad87e455b6d4dc3be74c830a29fe0843de098815d893ec909c5207bb23e0afac0072b0900401fc4593c7fe3461225213a5e2c77cbb3e2232c96e0bdf82b6ad94479e50407fa5ef5d2ae1ccbf617177ee922f9125e21544e1631d02e55db8d1bde0dc0e53bea599103ec94ddaa8eb56d2fe4d9863eb25343c71290d16fdf89561a1043d251fe6828cef446551cb39c079355128569f6365084e8e0ba7ad290a86f1ce5f36c7767533b6490e56862aa5718cfec7525ba9b8c3418d05ddec6533058f0e1d03e46d82294a49b0de9f41dbbd40ca2a3f45b604d0c46a04d2211dd71cbedb93735c7f0685bea5217599777f5e53fa69ff4d06ca8b43ac0a7635975e64ae9dda05f26c26f43f2e92799fe3facec8fe93c7c600761cf1cc07dffe3007e8fc51bf591b1453524d2affbee22cbbc3a11a138a2f0e6039f5c50bedc8d08c447899a2a2a0d3c740db1976f796be2bb9bccd4dbfee8f5b4c52870188d430d07a63bf558ff0071c0c8e2b16042498fdc17cf11c5008055592ec420a9dc43d315f8c664aebb1d89b3a67decc3cdc40ef10ed47789eb151d216ff8001fd756b475a858651921173b2dda02ed89beb2ac0dc2750bdceac23d8795cc008b9d67c4b4861c08d52d5dd948bed01fa486cf55d10d85b7a388dd4ad693c530bb19f2c0efcdd024399d3c42be80e14efef270cb6edfa41f273883fca233ebe0e89969064ec3cb68ca42308e89b85fb3c0f96d0bcf3641ba66e0b06337caccf08679ad58385909ce9c2d2d52ee676663a2fdf563066d7d45841af3913b655850be6f5f082c57d136a9a70b2d33af7514ebab5221641383e5b4e3922aa745d33007fe10d62d44df0c0593706af16698861e0acc4b248ad15b4a84f1b6b59e3ba052105bcad7f386886a67ecbd9b0b2172a4a4271fc3a4ceb241f5568eb8e6fee01cfc59bd4d27a91c055ee759f81aeb66e92a9560f79cc32a8685b8146c322af0c9d101a487d14dc5c3473b15aa76aeebd2aef99af79ad3434a1c19f367db4060babcb9cbfc2f6f31a07cd471c7e875f400fe0ab66ec880f83163efb2032fbb00828caaa35bc613213f53a7fb66a305862acdf90e49d80e0f1bb52708affe9b20dfe2c6bbfccd5d299d16296e379b72cfbb3263f27b13e0f179118e517abe2170fcf117b23d75d00b41e84a2c28d471c2aa3c73df1b17018061656df58aa24210c3cdc3db5f2f8d8ffb7d43dca55a6d087286494bf53f617e844f09e48cd04f20b29039f00a8508dd89eeda75c886a218f2a483798ecfc006e16edd63dd449c40f3e925d52924216dd9a33f7e1ca907712464d2ce30470d618015f433ca5cd3a01f7e91e53c0e8edf66b1ea8fcf24819d80e37236525af7b755ee6f5825b612902e32e0bece34cf66418b26e3925783409902724dc93661c572d4a0ec325a79308f51c6700018ba72c0c6ca90278ff4fb86e36123602994addb7e44d96febb5703196ae8a1578d3092b432259bd9305dc4f4e58af7c6e6cb933324f7fce6a080059d3214700a5389346add7c665c7524f80e69adb197c72182c8024a2ac6193f07e4bf94889c8bc07b6bcfc98b326a934701a1dfd25953cff7720b33e1be653801f1559443e502961c653edef2d5d82d6c5809a9d688e551a534fecbac84663802e25f17360315f43a62be241c1731794cf69b9659ca231ab11236471f9c8aee065edfa2077513fac214deddab6b79f81faba28ccc22ef750cfd192d5201fa92033226c28133f074f3597b243797c34457c771a5ed084e4a5866a2b520c82e4b0c346a808d1984861202075b6d435f7ebded222aec3e0adf3325c6a8bc63ba8301",
        "Ci": "e27a3f17dac638feba7a0b2d99f6a3a80f0cd6ce73fe6e4ae0d5ae9d465b953b060fe0111ce6ce2039cbe9b1af970a5f5427b230bb5fa8374b9fed1370abe5dda5f6cec25a996a083450e0b9db529a672b64b38173fa72aaab2553b45019fee88fd0a925a7183276482aa509a623fbea3c73c1796d0bf32e7972fcbd6e6cc505313bdd1b1e6d2b6836f6f8d33272fb0bd2aa4ae6bcc72beb4052ac43b61feb46fd68b85c36ea55a0d4b6fb3d43402fef960c4fd03e35b92d60d985bb2dea9e0c610aa9ea418ef21b749184f93c00470df8c001c5eab3e6d124111a081592cd052f4227d4192bef25b1695f161322d9ec5e91967b7a7a327dd1b5b72c18dc21639ffc46450e7afa589546bd42370d032460b94f4ed669d676e0ba1fdecf689d09caf60787eb9854759ee5340c4bac0211819295b8f7e7d53dd51ffb539908ec3ae283014cf17530f58000909d23468452b09d6c4bee3f66a03ae97f9e4bc2d2d098ca27268997f66acc4f44bec5134682fef3da18fbf566765f5184982fb4243130db8d607a674f51a66cf6f08e54dd8f14b2cd742712aba1ce1e8bae078e75fa56ebc5b225ceed4b3f03eb38c575d9f74514e66fd1eef3bc39cb0de5f65d5dafc653c912834f39a1d74ee8b9af6bcda9b938e607b83e58a157dc7a032ea1b4d1aeb7144f0d7dfd9e35e0a92a4c646bf5cddf449e8b2a9b7fd624e0ab23046a006314ec18fb884afaa6f88858f9bc5d28c8328f85bf514b9febf61e154d6f9b6f6033db98d4a50a36a972f191f64407b1c737d537427b1079f0d02061a0e5d75448ab78db03c7f59f097253c3456af8a8d19b4d7706a7c50dd9458a97e450b1589b2cb59d19fa64e6283d9bf2031463b83abb6294d6dcc03230b7c9f2fe5621a44f7afe26827e377d884f26c38e4bb652aa2f87936d99e19320abf3aa779a0595c5ee21ae2f1d0cb13befb5730a8ab3bf9c2b3aa12b27cee2ea1af63e3a8a30cf1e51384a5414a5785281d369d5be2b65a654b038c73694cf504d4f7f3b594b397f6709df0cd16e83b75a09af3d19232203cb2182396ac9370ba2c8187df4b8508af2debbc5df93587a3121ce3966a5d7827dd36829c80f77ec9a41e609b76a48d5b0cb6e22ac9b6801368b1917180e391dcbc084ad6f63ffd7ccb346cf8cc162e87b11e580b412ca793085bcc2af4480633bf8a5ef56642465c57d58d36315466ab2a7e8725c1e8783d1590ab46770ea7f4000fa868b36dfebe519613dc77042d8bd090bf29a08ed502a1e197352ff818acee8d577c4b53e03efa6591970502e28429cb72036a374102104ce9b981a35e1def5320e2e44ae88ac23b0a477ec86ce893cb4daa7eb7b4b2e9a3f5f8c1719bc2b2aff43bef2e67ac08e1147986486ae1223fa651e2e1e45898d73cb36914ae01a654b5160551a3527d7776fe7edeacd7e4067471e6c9979f9c2a75a10a470f227a11c4aacb5d72d5b4157cde7ab7e118dbded13610cc0c380c755653697afb4865c3d2c71ebc633d961bac18498a6282c5474892016d1f0be10e711c7f05b159f1a49efdd8eb77ca4aaf84748313950929aec48616344cb19b7aa9c5e460d99a187922d367fbb6c5bf271d74584e0b9a041c6b9d6e1dd7510cb7a4627f8a755dbccbf9b9b58c3aeed823d056f8ce910728b47e8eabd451ea288bd2fb8dc11bb711237c1942bd8afacaa45d1d2bfe4014e3064b47a8d94a80b0f3021fb971a3a848c5e916c1cc7e2624e402bef32f7ae0c17f75ed07901249f76a059bab307b7ae44072819d25f52a41783dd9d1a1a3359a42098dfcfcf80e55847ed99d87500beaee45f9dd20196b77a2253f9002bd0a2de7bdcc7b378c130ce5dcaf778d706275a344c6e7042783c227337b3ad69ce4f399769202080566ea1b1cb5e79bd6544963b175367358a7a64c419d74938c8aa5d3df202dc163af4339363d1f35f8d4760a7b9c90ad7c262b5a2606493d4634ea950f1ad239fe8d7d2f0ed53b92701ba188e812551a322839e877cfc4157eb0ec0119c0c407329398138520a45f8abf62103a955744f82ec1935f02fc1139a37e898a5b4ebf7c3cf8a2511d8bca65094a67eefc90d50becca0d3f898c628075995fcd61196aa3161a2c43f1f4aebce12b8ae3a0c4d82c587354a3756de73b9d6fe632a8124790077eaa1dc61dc1e8616837cce6162b269ce21e563eb70ec7d9b32e951d2f946c0ad92ea03d011b0754150dc94c6f9749212d456b490c4838db5004ecb8c87335d1ee91c4f585e95ac4608f4f1401f15914f201bccd7d92bb25838f90f67361ffe353309d60be3ec55d4c45e5c0cadec33d8dadc6241f85a251ff2d29b0aa87285fcaa8dc8195ed1d8a0e30b4c415a9aa608fee202c6339fa534d13f88650cc65459cab8da492527e04836ceb0fcd6ed2d59e94a23f42f8d94606dfe00fd2e2c7e1af484934165804e585b535c7e09c2e59f11a0bd46f097b5c57012c7c2bbea915f2a12305bd6d0a91bdd9c92c231be07308fee0ec7ddb4d66030c80ae527cabadd27a109f1caa1ebec4f3f5c354ee894055476110acadc33007c466ac18d211b957b7e1881046393274c35dbf8ba9769073bc7d7ba01033a6af3ae8c33756464c801266c8fbf984b63808c49978414920184b6bc5c262579adc02888d63ab970057f3f6769cdd003dbd39db96e4ead235e6f2d0c83f83d7a45c2779302273e83aa1a4cd437e704eacfc342f5dfbdc9f740a8f7e9f91b7c6d7001abe0d75a5a51cedae9641710ed8ad34960460effbcf2a8ee4dddccd4592f911125bb41ca70167e249e9e652f350bc3de5568e0290e7e52a4de62aa9e73cb4e1498a84d527d8d293d1b501e374abffadb68e0b8f75da3294c2faa8c6dfe"
      }, {
        "asig": "1f4324e2b8e221040fb1d016e4bd0a1d4a8f5921a8cc6de135d458cbc0dbef5d77592e69b121c8387ddd7860efb89a036f923e15b5a62e9163472422d86f3daf78b53368ea4625621b3462427fc9f6222c0b658c685c1a084d6f9863856b2e9149c6715e83d8b970024bd4e3d2b03144b41d3f8210fc8e07f5131a02be19dddf6fa5421c4ec59093086c5f7d6baf9039ff57fcf76beebf7eae68a0e95f82108f0d158ce5f6f92f309335265c30a9d764cbff304d6e4ec05b7a49c84ba4b2efa7041975b19874f7e1f13bca0caacc28eda509845e70f7b8c1a00bc462090da8af1f34caa5d60058b997e25ee2b0cb4cca4d3d07ce60924bdc6cd85b7759e75604aee9286229438cba9dfed9d5e1cad34c3f6164ee7909f85f370a923a257e92df4478557580988abd2ab76e6026d03eaac09c2f998012c34297712c8a6972e2d90fb3a9cbf4118c67b880c5233d88475c95b0f4bf672bcbcc93d3c2e724590eb1442cf3681e3d6c83535256600e864dc675075054c361c886f2a4f4867d2f03804baf8b3ad59fbdf9ba3cdb8e06c9f46c51680aa85eb5a1c87c146b952e992b9c522fa7957c9e0e0e861e25a10dfc3aa651374c00336b735b58e52e591407c63c6dad25a4879669239d9ad14d81da828197a810f893784464439836450c94e99b54937813db9cf25c970921fe07c3bd036d6bdae6adc22f6840c0f0508710cebb5378d86ee1279445b39144da85da9ca04aba29be243c35e202333b82ad7b871412695adf4c4b68290ef88171080c6fbd177f55c0ca1b4b0b8d06e5f7c4bf72cefd995dbf31a6a3d9f682f88aa16b95e28deabb68219133e968bc1fa74d39149b1d22c7edcd7520b460cc087687e96cd77ed5f410c7e11367c1b2779ace76878558b467916bd13c776e8ed274f681b8a6025d801e4e1e03dc2a6b2ebdb585e46d8a06d59d019a118012009175e4e8f2b3b04e018b0df2d0b3f12493cc44e1bb6415c5d78933d1ae4887feabc95c0ffc6398c2d82e827a460cf0f1340581ff79a7f2553c88ad64f88ae771124d8f64b8461598b27643f5e9deb9f4028af2c47424b8b46273ab4f426262c32d5e49a3a37ebafb0941a3ab526212824c295dc026620dd35be8ea6113356260ffca08d8f28a9ca30d600f80b1f85a88d8c97dcc6b79ded8ed4a1bb9e254fd9d479d0d08deb502b7e84bf6af6400808dd413c79455ce6204b907980ae7074aa1c57fac4699c5275b5a7eea822c60be68f6fe1aa132dc23cf4f9899ca2fdba3365497b7fe3df5dc251aa59a6d82303d4cfe94faa4d240466a67d3292e59a2a05034a359f57d70ebb18bffe65ccd6001def176ee9879f4e23fb06b79c57e61ee22a9804f2b9c25ef88968677eb98682a45a78281f0ecdf2c55de5e180cc9ae5875a7bdaa8e00bef6d1baa9f43ced43f8f826abf64ac8b26652b2d38d668536fc7498c6f23042f8f1a0a746079e4f740db142e915ee064f621bac5b9e24b7ac9904e71041c3f0d319204bd3195768c0c4abf2068b3aca64f27bdf1fd3065586fd69d17169f6f7b91769ce6c70e96fe25349ac679ced0785fb7239ccc36a4addd6f82a19a4e49d284c7cab13a42e03e5932607bdecc8bdc23e0382ff01afddd03808474eaf0270eec1cba7decaa9c0a1c16d588f2e884b5090e1fc815528b15cdf062e3b474b51af210e2db9bcbcb60d4145a01b3f8ba726c85835910341a96eff0a575a4793a651480fafea929e98b182115c25bfb4326b4f6d6ef4631b3e673559177895719ca78a6f9ca80865986063c33fd94283fabc5d804b9d38ca625b424fab6d7f43993f6eeee94cd9096f33fdc00df7dbb44f44ecb44edb80b675d869b16edfc075b1f203dc19f3fb8590be0488b5c441bed3fec4e738b5f6d3e077a3c62839bce765ef5f46f5037d93fa96419e5ce920f0c069b3d5352aa7536dbda3511eec4a493a5868f77ee87a0fb62421ebfee158d0ab41cdb03ae38399f53ada47a058c30168b6eacbe3a97726abbdd244052b2e25b9f24f163adcbd4bf556551017af8b465b83f908bf3a668f11ad6b6354d006ebf4c494f13f81350c8b572dc289bc770b2da46c594813a2c5bc61a096b152ebf7fd2ccc196d4842cbb467e44bd626ac7b338befa0ebf70b4d45ea3e5ccc33f80976fa522bd01466cfc4b1e5f558e9817fd8007ba4ff69eea0aead393742f292eadb7b25af3b36be3f6a9808ef8648c00a1b47b3845af8deb74e821abeee5037d0dd170ee9077c37b46bfe41361a31e874249e0292fbbc0c9b4e1d3184baa9c9bed643484a19c8179190ae86b9e77b8e726d9d6ccea13c9eec1e8585b6f99fe0281801a42740b29d263dfcf503ad2831fff9cf2e7bf2a3479120f71992ae591291bfbfbf07945d7af48a60b3ffbc2b7f147926f9c9c692f48a55194f9fb92b7127e28447abcb822ef3f78f46a583af2da7273b350c95963f4e6c56001b12f5aa17b75f4d9b0a2ecb1c5ff154de8282d5d17179c9b152263814ffaf4258ec7cadb4fab2fec001f9924da42e836bd5ed0c989c0882e779c54cb0ae7b6cf85c900cff5a726b5170615721d304669c40b6ef28b0846f12ec8782e8e741396361f8fdaa35acc7f750d2fbe0b437333f72cd605d9fe5113b47c8ac734c5a2d7a2f291556764610eff9602297ea0368a8b1f168b74b890cb3713ee28f026e2e68fd5614ceca9c63212d4edeefb2f2d76f48cc9e6dfc25edec63b8dd6fce2b7a6fcc33bb847f3dc61c2549adee2278011f8572d5fbaf365f02b9df5a38f8a12c6c5869928ea5217ad4ca54fbd98111cc5e40708b3181f72f4f35d90a4a6cb6a8fdf3d0b30c430be196b51765de3bf6a302ab5a9047f9d8d4d4a501f433f6601509aa2e01dafd0a41bb0421892a2acd8bbc9661fad374a65bbaa01845c76e8693618a2a572b2e0b878fbba9ce205e0b76524d11d5dafa1c68822017e09282214d9d56b7307a18094853feac6bbbbd50c6604e9af5fa1f90d421643beea7198c0a9fb1d69914b50c0bf42b91197e37cec7d96df59ce508bf89a34672e335ac538ec692cc7c355808ffd08abd229e62585c7cf51ba6cbbcb5237b4e8f30e50c5614438c390c42db0a2ef72286adbb18054eca1eca4e3e313e2591e09785175761d6b9881562b1960e934dbb9119dac0098b42deffbc1eafb63fff9a5b01de6c668fa33708e1635d04f6cbf6be7031d43fe02780a35d3af5ce413bcd4c13e40a67e8d44082b9367d08c23e6657120a579479273bdf176a27eeab9dda9d452120ef7f0b8dc164692a0857172e3c9edc290c79002332471147e076e9ac0a67a77e1518a090e46a3ea10d5b77789407f2cc0b08e4d0e6d759d73939fa0760a097f0f70dae27fb8fe8900f509c064be3060d06be3e82628ba84070b91e9e9b981506faf3d848289c9302090bf4a974fd9d10e66ee2675e1b7edea373eb592087a0667cabc03215d6bd1607bfde217739fb180c3aa2ee0feb9820bc34916f264d287b20174fc42796c394000c17f892ddd493f36f55af39605339cc349bb6a58ef25a7242459c935c84d00c207f3583cc1c356f1c4998881b5e0e54ae6fad2a927f79ae98bbc84bbb26d6016d4a9d01646b573bee5ad0619db4b321fa42b42b9f8f01e631978aea3e27e40a40869315f90b313da6414d59a6f567b928d16c50eb85977eacd19d568c18400e6381ac5edf4e1b6fbd620b2e38640be8ab52a80e53bf692ba5d4e0edf4e59c09abd6f5e78f077766bd33debf2e0616a40241143604ef21114e8af58ceb66ba089372aac7538d11fbf6628efb4fa3e283465e9019eab78e7e348b37e48b6d8c06061bf4ab9489021c41c6b43cb91f4f8274756de477e27789f3d2c2f7060a710dc3e6686c8ba2c7193805711d6026dee69987f86ada81101d6fb94433a026ef051c7772523f6f5fbdf0cad1f72f5910465f59539682d4e0d54b8422e2dbbae30cb444cc72fae8dbebde69a9592ccefd520d33229e1f68544cddf9a60ed8e0300c892f4af8504600492ddaaebf67f5f4b8145a6019ef13656fb2999eaac13017092c46f75797a96e63cacf40c0969fd58b7f8bff6368060678180afe3e23a53e0a13bf70a42649039113f2759ef426621dd8328ea9acf974b8624ba0729337a70c344bd7fccb3025745badd8d611ac93b0385a0c5f0a0ba2fefe20c7fe4e153f00af54fd4f6d26e43c7c9e3c3c8ebe8631729817d0e3e0b9c1bb6dc1067383ef0de5b403c0d01f32555c145f613a7bb1016a1a98a1cdffa30f752c38ac3950310fff366d8768eef59adfb9d1e43729016b9c9bc0e1bd3c49f404fa600901591b04311912b181d1d1e6d493112c6bc23e4d94ad476e44d25bef2ca0beceafbb49052321818a7e497517ae8f29a9e35c18d871d2f3505a4fe8f06f692b34fb564a047e42f298147fe0b2bb3247b4b2b1ef3196fa8d9332119199cad6d37920900c05bae838cc458320dde16c4f914ab7ba40ab680b3469b310c0330d8b19aabab60852a05b0bed69734b30e6fa084ee542070844cebc30db015af6fb483d5a3362022fbad17e48e42358ed20ad0fefe5d4f9b2a826dcde299fb652415b617e9837092d9366be93eeefbe042aed10a6fd851eb78793c875608a48831f6ac2ae8b6c0e06aeab6e1263cb2e81a9223c6777452cd3495f7a445a0cc2d9a3fa257dca5e0fc7881b031a3ba6f742e85f368ff6768bb19ab528724b6035a9c96af85de00c0e5d5b7cf71051c372dfe84819163f591e663673891f176a6e4d8fc2b37ef02b0c8b5f766875828c418166297c73371f435026d9861ff9138e24648273d5a587015e77d51dd55edb1233d2e3b6b544773c615fb3b62fd74686811cefd760452e041918308b17f09e9b05206e92fbdedd5a2519716b646760c5726d98227b109208f08b0e711055bfc639c26249ba26c456d17e704db4261bcf4be1f7f210d2350b64f714c690883348a6ca5d959d0b39675c0a0ca64e4ac15f2816d3dc5476e10d3c71c3151760235d7b298bdffe0e3256ad8d0a29b7777ce09c1b171bdeca79077b88954934f22ef21d57d5f3c9567dd2666b1802c80253f81d16e0aef0ba1d0ba3b2320987a273d70261ed88f77d59dc59dd9e6d01b7027bdca82ca4f28d32003b211b803ee985257d2941f9fb396418a647d7565b5faab08a939f5e02a7d909b7d522993b1d81da49fd88415fccdd7f3e43dd3b7cadcf269ead7617eaa60f0230d786bf0030c559de72bda1e9ed784c47df8fb7977c2923e1fd500d44696f0909c5a519138ed1eb35d430e6f83585f8723880995e6417aaad9a5ca412af150447ae93ee199f43695475bb7d32c905619ae71bca82e456333ab2427d7d55370ed873a28688197201a8f1a219a21cc749d08ef5553dbe15973f5d543100b8fb080b530bea8297c7daba6b14eb3cd8ce4b23ec32a265f6275b5f597b90b9d01106359e3e835fe859e5c6aa635d97cb96e8ddc20de7bf94a1f3fbe9f223c135f500fa0802d1f506231d2d28cd1c420cbba76f80f2af696195c61a07d5fa6a791b04d5cacbd969290577d79523679b82661eefa380bd3dabba86d66e937861265303fc7b979b78fd505e6b5b65339de6c98031c5577c622cb6c15a4720e50f1fea0391e4c86310fc13db193812b7c607ea79cae076f3b167292ba817666f8faa0804fcd4a38df8d2ae647117c9fefacfd87b22364d413775eacc841a602891033900479589eb78ff5ffcff7243dabb5770aec00229d7f4a5ae1e198bf9733a969a0ca92f195048dc97a5f0858384ae0f1f0ef6135a9a3e846f3b87a3dea804cdeb05",
        "Ci": "80abb0cf4a52612554155e27c7d4c52aee54eadd1708c70dda1c7c2611bb17c5872693c51b48d5362f31fab6b762c5a2a09759cbadd8e9b8869eaf9252b5e7bcae816146fb30478aff8a31a895c6f140fd5d1da0f2343916cd4b2a693beba6eeab3f02c65c28a4ab510813ca6fb734997050f2792e0fe3dd41cd36ba2d10b282dd4c7edd372e950d1dc4ad38a63bdb7c5e5761504ee9f81355cb6b2232857bba0614a3669633ecd6d6cd29543caf2b6eaf7f38e57c4abe879cf3a2f26a9f720ec5b26cd327b4a64854f9103a097d79f60721a814d2353aa8fd91c1d1d701becc9c59e772c4c960e8adce12958ac01870618130e9d74f3850d2ecad7ae649296d82f72d8672dd6a1f4306e78ca8def583ff0d234e1fe975f12aea8a16b52ae45b78a6c3ca3c0d3e072a93bd3b41f7e946f7ed149710ef7f7cf6d1debd7f63920f9f4a72c046bc7c2220ca3d9a63289ffa2e7d942c26780c6285bd3150d8a067bc789ec7aeaad7f075eb3070f86d2a0f1f506bd3d1c428199a3d2365794901b8eb8ac77598d593948063590d6154f71484cab5d6b172cb4b672fd12ebf937ead5404c48c082d52a9f9fcc59ea4f9f897b285e005a8734c72f6083f667cd82f449e2e84dcf7b4a4859fc184baf7d21287aa389cace42c59b94b0adc54f62a8078dc21ab8015c3af3689c6754de8ea71474af4c99aa9f4d0f60ef06481a8e72f5e26551d6aa8f9bce3d693dc36eb516f596e4369ed2900013087d6bff55999a41b7eca13b7acc087ddcdb6d9ddc6178240edb14832337afb432624f9b277500e08c9b599002945e8c02c9870d4fcbdb73068f546d060f0a838a6677abc08a67055728c08762d7b20c736f9be583676b854615ba6c0de133b4ddacc5e726a6589155bc1a3d5895921f6fc5ab65a30d9f0f7475f9ecbc82c812158126b26990f4b386e936919206297381eb541c01a54283303d1457eab21f49efe6d699c255765b2f9cd1b815c37f95d68556238277c69eb67d42f1e9d74fbe20cea80881c01842ab6d89a52e34f34827f6e68332c900f8c9303c4881e03815163848000650564e3f0cafa92f84f34fd3c5b8d6be9aad4df40bbbc6ad5afa512e684be0c9aba9c7c82190144990aa81739dc41956d5efa01b0e7ad3f286ae65c474c90608db8eb749bb486984fb326e2a805e908d3a2fd66b93d2965ecb9f350fca0823b7f4b3db2c0b84fa10905970b659227bb832a0ae049489445ec53ffb6da92c6fa7e030ff88900caa3b4ce9e74fdb111e98d27346168e337ef5d6e3d34c21f904118ac7a0ec58c942ab15322faa65973fef4731edbab85b8b90f60cf6e6c0d56834cb34dccfe3c100e43391c8471f359cebb6520ddfb18e91fd78b46d6413bf3d02dc94e35fecd976c8beaf915adaa357ec948caa58a6eb6eb648c293f3badab4016ab3ef3a468ceb5ebb1417ff24093bbf521b9a87d23588287771df077bca790e054b943f85b275b8927a8b8dca54d2fa82b37156c81e74894de68896b2f156494b6fdae431ea4f7faa68bbeaf7e823e7ecb8c69a7d4f879b8349e0975136356dac33022bc59af2cefc657864b2bc7b052d6924eb0357a70293cdce21a7d7ae28b9afe590802ae1753cf9b58eba4254405b6e8417dfb024328502555315b6a3fe947499e630126c09ba8957384f2a476bdcb045527097e65a9223345b35245ccfdb8fd01e3192825b1766199ae832ebd8ba72aae35dcb0fba98a85154afb5c9037f0aa6d19b211668aab98a4470600535acc089de49e063efd07e3838bc11270f03a18ddb8f8789ade6713e83ec199f74c0a93d2787cec45394ca8a7426e8ee0b870a013940d849afe3026285357d73873640697b4bc711d439d0958d96303f2529a5a1861f8f45c15e272172ca338f596ab8676467a44e6e3a2121db96fb85db1514edc85ce3428bceb5ed209afa902b82e6d84b4cd1cf9a09e2c834c773e374894ef900b24852d272cda65d7ed5e2e8678d2443dda036f1ebb5e5702de0994273f2c12deb9a7d2de74339077a9b9df2afe9a879d0ae77786bca6efa7be55aa8ca8f38b0c3236124a865751f897b55bdab902b39a7b49cc36baeb1988010ede5f88493e427bce95d36998eef1c1efe7979e4d838ff57055a2dd8ad3f2174e68e35e5fdd953cf5ee9bc6ea640809699d53f51fe2bb025a1b4f382764340152c9a28da3e96f6f7fee7517bb3efde1a1012e214710be1b71038a91025134d2fd18341ada293bc44f53a15998e3d84086b5f6a86bbe1a294787107ff60b187f3a708f089c3a09a8cfdc7b3be919de2da70ab0108582b0315ccca86758305768ce7d525d9283eaa3895f7a7834668f7359d0b328aebd8a35daaac89b20d8128ef9426d83e2c03226b979211f0b7afd1e139fbfc25e01559fb1fdcdab678bfd3c2371c2b8683fdb433b21ccc988e6df069b217e562a7c165de6a278384e4905e86770dc4f4d16bea6d61ef35f26d7fd9a1365d2eb5a3b32f7bccb712967a8b8c444487455e2cd02c5c28edf508e91a79779c1d202c7b9513731b3dc714a0ed6cb95fe8865a699ba8e91c9197713d23f0913f678408820341bd6364a605b92df00bc15fd8ab8d55f39036c8fe3f9d9bf7c5b343bc99adbdc7034f8a2a4d2f398dbc07fd8ef8209ddc1434b3a6de75cd03f86cadaac817d45a9ecae05484b721ada66ffb2259843563ff3a90e1ce74f855f57b7bcefc0f24cfbf4099e2b4af9e9c9f405d023503652f72d3fd7604774d9b7ca1b34f47720e40d42efd54f348c77564b4e49ef26e33fa0e9b81d30f09ac3750de3b85028878dc8a5a82cd6c5f4e98dca7edea5c360d73210ef50c5d2c4d0554b5e6d1cd673f6063e2e113f3d5803ae95d868ea750214"
      }]
  }
}
)V0G0N";

    try
    {
        j = json::parse(raw);
    }
    catch (std::invalid_argument& e)
    {
        cerr << "make_tx_from_json: cant parse json string: " << e.what() << endl;
        return false;
    }

    // get version and unlock time from json
    tx.version     = j["version"].get<size_t>();
    tx.unlock_time = j["version"].get<uint64_t>();

    // next get extra data
    for (json& extra_val: j["extra"])
        tx.extra.push_back(extra_val.get<uint8_t>());


    // now populate output data from json
    vector<tx_out>& tx_outputs = tx.vout;

    for (json& vo: j["vout"])
    {
        uint64_t amount = vo["amount"].get<uint64_t>();

        public_key out_pub_key;

        if (!epee::string_tools::hex_to_pod(vo["target"]["key"], out_pub_key))
        {
            cerr << "Faild to parse public_key of an output from json" << endl;
            return false;
        }

        txout_target_v target {txout_to_key {out_pub_key}};

        tx_outputs.push_back(tx_out {amount, target});
    }

    // now populate input data from json
    vector<txin_v>& tx_inputs = tx.vin;

    for (json& vi: j["vin"])
    {
        uint64_t amount = vi["key"]["amount"].get<uint64_t>();

        key_image in_key_image;

        if (!epee::string_tools::hex_to_pod(vi["key"]["k_image"], in_key_image))
        {
            cerr << "Faild to parse key_image of an input from json" << endl;
            return false;
        }

        vector<uint64_t> key_offsets;

        for (json& ko: vi["key"]["key_offsets"])
        {
            key_offsets.push_back(ko.get<uint64_t>());
        }

        txin_v _txin_v {txin_to_key {amount, key_offsets, in_key_image}};

        tx_inputs.push_back(_txin_v);
    }

    // now add rct_signatures from json to tx if exist

    if (j.find("rct_signatures") != j.end())
    {
        rct::rctSig& rct_signatures = tx.rct_signatures;

        vector<rct::ecdhTuple>& ecdhInfo = rct_signatures.ecdhInfo;

        for (json& ecdhI: j["rct_signatures"]["ecdhInfo"])
        {
            rct::ecdhTuple a_tuple;

            cout << "ecdhI[\"amount\"]: " << ecdhI["amount"] << endl;

            if (!epee::string_tools::hex_to_pod(ecdhI["amount"], a_tuple.amount))
            {
                cerr << "Faild to parse ecdhInfo of an amount from json" << endl;
                return false;
            }

            //cout << epee::string_tools::pod_to_hex(a_tuple.amount) << endl;

            if (!epee::string_tools::hex_to_pod(ecdhI["mask"], a_tuple.mask))
            {
                cerr << "Faild to parse ecdhInfo of an mask from json" << endl;
                return false;
            }

            ecdhInfo.push_back(a_tuple);
        }

        vector<rct::ctkey>& outPk = rct_signatures.outPk;

        for (json& pk: j["rct_signatures"]["outPk"])
        {
            rct::key a_key;

            if (!epee::string_tools::hex_to_pod(pk, a_key))
            {
                cerr << "Faild to parse rct::key of an outPk from json" << endl;
                return false;
            }

            // dont have second value, i.e. mask, in the json
            // so I just put whatever, same key for exmaple
            outPk.push_back(rct::ctkey{a_key, a_key});
        }

        rct_signatures.txnFee = j["rct_signatures"]["txnFee"].get<uint64_t>();
        rct_signatures.type   = j["rct_signatures"]["type"].get<uint8_t>();

    } //  if (j.find("rct_signatures") != j.end())


    if (j.find("rctsig_prunable") != j.end())
    {
        rct::rctSigPrunable &rctsig_prunable = tx.rct_signatures.p;

        vector<rct::rangeSig>& range_sigs = rctsig_prunable.rangeSigs;

//        for (json& range_s: j["rctsig_prunable"]["rangeSigs"])
//        {
//            rct::asnlSig asig;
//
//            if (!epee::string_tools::hex_to_pod<rct::asnlSig>(range_s["asig"], asig))
//            {
//                cerr << "Faild to parse asig of an asnlSig from json" << endl;
//                return false;
//            }
//
//            rct::key64 Ci;
//
//            if (!epee::string_tools::hex_to_pod<rct::key64>(range_s["Ci"], Ci))
//            {
//                cerr << "Faild to parse Ci of an asnlSig from json" << endl;
//                return false;
//            }
//
//            range_sigs.emplace_back(asig, Ci);
//        }

    } // j.find("rctsig_prunable") != j.end()


    cout << j.dump(4) << endl;

    cout << obj_to_json_str(tx) << endl;

    return true;
}

}


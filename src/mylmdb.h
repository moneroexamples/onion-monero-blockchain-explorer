//
// Created by mwo on 27/04/16.
//
#ifndef XMRLMDBCPP_MYLMDB_H
#define XMRLMDBCPP_MYLMDB_H

#include "../ext/lmdb++.h"

#include <iostream>
#include <memory>

namespace xmreg
{

    using epee::string_tools::pod_to_hex;

    using namespace std;


    class MyLMDB
    {


        static const uint64_t DEFAULT_MAPSIZE = 20UL * 1024UL * 1024UL * 1024UL; /* 10 GiB */
        static const uint64_t DEFAULT_NO_DBs  = 5;


        string m_db_path;

        uint64_t m_mapsize;
        uint64_t m_no_dbs;

        lmdb::env m_env;


    public:
        MyLMDB(string _path,
               uint64_t _mapsize = DEFAULT_MAPSIZE,
               uint64_t _no_dbs = DEFAULT_NO_DBs)
                : m_db_path {_path},
                  m_mapsize {_mapsize},
                  m_no_dbs {_no_dbs},
                  m_env {nullptr}
        {
            create_and_open_env();
        }

        bool
        create_and_open_env()
        {
            try
            {   m_env = lmdb::env::create();
                m_env.set_mapsize(m_mapsize);
                m_env.set_max_dbs(m_no_dbs);
                m_env.open(m_db_path.c_str(), MDB_CREATE, 0664);
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }


        bool
        write_key_images(const transaction& tx)
        {
            crypto::hash tx_hash = get_transaction_hash(tx);

            string tx_hash_str = pod_to_hex(tx_hash);

            vector<cryptonote::txin_to_key> key_images
                    = xmreg::get_key_images(tx);

            lmdb::txn wtxn {nullptr};
            lmdb::dbi wdbi {0};

            unsigned int flags = MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED;

            try
            {
                wtxn = lmdb::txn::begin(m_env);
                wdbi  = lmdb::dbi::open(wtxn, "key_images", flags);
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            for (const cryptonote::txin_to_key& key_image: key_images)
            {
                string key_img_str = pod_to_hex(key_image.k_image);

                lmdb::val key_img_val {key_img_str};
                lmdb::val tx_hash_val {tx_hash_str};

                wdbi.put(wtxn, key_img_val, tx_hash_val);
            }

            try
            {
                wtxn.commit();
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        write_output_public_keys(const transaction& tx)
        {
            crypto::hash tx_hash = get_transaction_hash(tx);

            string tx_hash_str = pod_to_hex(tx_hash);

            vector<pair<cryptonote::txout_to_key, uint64_t>> outputs
                    = xmreg::get_ouputs(tx);

            lmdb::txn wtxn {nullptr};
            lmdb::dbi wdbi {0};

            unsigned int flags = MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED;

            try
            {
                wtxn = lmdb::txn::begin(m_env);
                wdbi = lmdb::dbi::open(wtxn, "output_public_keys", flags);
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            for (const pair<cryptonote::txout_to_key, uint64_t>& output: outputs)
            {
                string public_key_str = pod_to_hex(output.first.key);

                lmdb::val public_key_val {public_key_str};
                lmdb::val tx_hash_val    {tx_hash_str};

                wdbi.put(wtxn, public_key_val, tx_hash_val);
            }

            try
            {
                wtxn.commit();
            }
            catch (lmdb::error& e )
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        write_tx_public_key(const transaction& tx)
        {
            crypto::hash tx_hash = get_transaction_hash(tx);

            string tx_hash_str = pod_to_hex(tx_hash);

            unsigned int flags = MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED;

            public_key pk = get_tx_pub_key_from_extra(tx);

            string pk_str = pod_to_hex(pk);

            try
            {
                lmdb::txn wtxn = lmdb::txn::begin(m_env);
                lmdb::dbi wdbi = lmdb::dbi::open(wtxn, "tx_public_keys", flags);

                //cout << "Saving public_key: " << pk_str << endl;

                lmdb::val public_key_val {pk_str};
                lmdb::val tx_hash_val    {tx_hash_str};

                wdbi.put(wtxn, public_key_val, tx_hash_val);

                wtxn.commit();
            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        bool
        write_payment_id(const transaction& tx)
        {
            crypto::hash tx_hash = get_transaction_hash(tx);

            string tx_hash_str = pod_to_hex(tx_hash);

            crypto::hash  payment_id;
            crypto::hash8 payment_id8;

            get_payment_id(tx, payment_id, payment_id8);

            if (payment_id == null_hash)
            {
                return true;
            }

            unsigned int flags = MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED;

            string payment_id_str = pod_to_hex(payment_id);

            try
            {
                lmdb::txn wtxn = lmdb::txn::begin(m_env);
                lmdb::dbi wdbi = lmdb::dbi::open(wtxn, "payments_id", flags);

                //cout << "Saving payiment_id: " << payment_id_str << endl;

                lmdb::val payment_id_val {payment_id_str};
                lmdb::val tx_hash_val    {tx_hash_str};

                wdbi.put(wtxn, payment_id_val, tx_hash_val);

                wtxn.commit();
            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return false;
            }

            return true;
        }

        vector<string>
        search(const string& key, const string& db_name = "key_images")
        {
            vector<string> out;

            unsigned int flags = MDB_DUPSORT | MDB_DUPFIXED;

            try
            {

                lmdb::txn rtxn  = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
                lmdb::dbi rdbi  = lmdb::dbi::open(rtxn, db_name.c_str(), flags);
                lmdb::cursor cr = lmdb::cursor::open(rtxn, rdbi);

                lmdb::val key_to_find{key};
                lmdb::val tx_hash_val;

                // set cursor the the first item
                if (cr.get(key_to_find, tx_hash_val, MDB_SET))
                {
                    //cout << key_val_to_str(key_to_find, tx_hash_val) << endl;
                    out.push_back(string(tx_hash_val.data(), tx_hash_val.size()));

                    // process other values for the same key
                    while (cr.get(key_to_find, tx_hash_val, MDB_NEXT_DUP))
                    {
                        //cout << key_val_to_str(key_to_find, tx_hash_val) << endl;
                        out.push_back(string(tx_hash_val.data(), tx_hash_val.size()));
                    }
                }

                cr.close();
                rtxn.abort();

            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return out;
            }

            return out;
        }

        void
        print_all(const string& db_name)
        {
            unsigned int flags = MDB_DUPSORT | MDB_DUPFIXED;

            try
            {

                lmdb::txn rtxn  = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
                lmdb::dbi rdbi  = lmdb::dbi::open(rtxn, db_name.c_str(), flags);
                lmdb::cursor cr = lmdb::cursor::open(rtxn, rdbi);

                lmdb::val key_to_find;
                lmdb::val tx_hash_val;


                // process other values for the same key
                while (cr.get(key_to_find, tx_hash_val, MDB_NEXT))
                {
                    cout << key_val_to_str(key_to_find, tx_hash_val) << endl;
                }

                cr.close();
                rtxn.abort();

            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
            }
        }

        static uint64_t
        get_blockchain_height(string blk_path = "/home/mwo/.blockchain/lmdb")
        {
            uint64_t height {0};

            try
            {
                auto env = lmdb::env::create();
                env.set_mapsize(DEFAULT_MAPSIZE * 3);
                env.set_max_dbs(20);
                env.open(blk_path.c_str(), MDB_CREATE, 0664);

                //auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
                auto rtxn = lmdb::txn::begin(env, nullptr);
                auto rdbi = lmdb::dbi::open(rtxn, "blocks");

                MDB_stat stats = rdbi.stat(rtxn);

                height = static_cast<uint64_t>(stats.ms_entries);

                rtxn.abort();
            }
            catch (lmdb::error& e)
            {
                cerr << e.what() << endl;
                return height;
            }
            catch (exception& e)
            {
                cerr << e.what() << endl;
                return height;
            }

            cout << height << endl;

            return height;

        }

        string
        key_val_to_str(const lmdb::val& key, const lmdb::val& val)
        {
            return "key: "     + string(key.data(), key.size())
                   + ", val: " + string(val.data(), val.size());
        }



    };

}

#endif //XMRLMDBCPP_MYLMDB_H

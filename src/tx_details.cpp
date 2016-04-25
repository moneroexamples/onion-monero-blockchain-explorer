//
// Created by mwo on 14/11/15.
//

#include "tx_details.h"


namespace xmreg
{



    crypto::hash
    transfer_details::tx_hash() const
    {
        return get_transaction_hash(m_tx);
    };


    uint64_t
    transfer_details::amount() const
    {
        return m_tx.vout[m_internal_output_index].amount;
    }


    ostream&
    operator<<(ostream& os, const transfer_details& td)
    {
        os << "Block: "    << td.m_block_height
           << " time: "    << timestamp_to_str(td.m_block_timestamp)
           << " tx hash: " << td.tx_hash()
           << " out idx: " << td.m_internal_output_index
           << " amount: "  << print_money(td.amount());

        return os;
    }




    /**
     * Get tx outputs associated with the given private view and public spend keys
     *
     *
     */
    vector<xmreg::transfer_details>
    get_belonging_outputs(const block& blk,
                          const transaction& tx,
                          const secret_key& private_view_key,
                          const public_key& public_spend_key,
                          uint64_t block_height)
    {
        // vector to be returned
        vector<xmreg::transfer_details> our_outputs;


        // get transaction's public key
        public_key pub_tx_key = get_tx_pub_key_from_extra(tx);

        // check if transaction has valid public key
        // if no, then skip
        if (pub_tx_key == null_pkey)
        {
            return our_outputs;
        }


        // get the total number of outputs in a transaction.
        size_t output_no = tx.vout.size();

        // check if the given transaction has any outputs
        // if no, then finish
        if (output_no == 0)
        {
            return our_outputs;
        }


        // public transaction key is combined with our viewkey
        // to create, so called, derived key.
        key_derivation derivation;

        if (!generate_key_derivation(pub_tx_key, private_view_key, derivation))
        {
            cerr << "Cant get dervied key for: "  << "\n"
                 << "pub_tx_key: " << private_view_key << " and "
                 << "prv_view_key" << private_view_key << endl;
            return our_outputs;
        }


        // each tx that we (or the address we are checking) received
        // contains a number of outputs.
        // some of them are ours, some not. so we need to go through
        // all of them in a given tx block, to check which outputs are ours.



        // sum amount of xmr sent to us
        // in the given transaction
        uint64_t money_transfered {0};

        // loop through outputs in the given tx
        // to check which outputs our ours. we compare outputs'
        // public keys with the public key that would had been
        // generated for us if we had gotten the outputs.
        // not sure this is the case though, but that's my understanding.
        for (size_t i = 0; i < output_no; ++i)
        {
            // get the tx output public key
            // that normally would be generated for us,
            // if someone had sent us some xmr.
            public_key pubkey;

            derive_public_key(derivation,
                              i,
                              public_spend_key,
                              pubkey);

            // get tx output public key
            const txout_to_key tx_out_to_key
                    = boost::get<txout_to_key>(tx.vout[i].target);


            //cout << "Output no: " << i << ", " << tx_out_to_key.key;

            // check if the output's public key is ours
            if (tx_out_to_key.key == pubkey)
            {
                // if so, then add this output to the
                // returned vector
                //our_outputs.push_back(tx.vout[i]);
                our_outputs.push_back(
                        xmreg::transfer_details {block_height,
                                                 blk.timestamp,
                                                 tx, i, false}
                );
            }
        }

        return our_outputs;
    }



    /**
     * Check if given output (specified by output_index)
     * belongs is ours based
     * on our private view key and public spend key
     */
    bool
    is_output_ours(const size_t& output_index,
                   const transaction& tx,
                   const secret_key& private_view_key,
                   const public_key& public_spend_key)
    {
        // get transaction's public key
        public_key pub_tx_key = get_tx_pub_key_from_extra(tx);

        // check if transaction has valid public key
        // if no, then skip
        if (pub_tx_key == null_pkey)
        {
            return false;
        }

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


}

template<>
csv::ofstream&
operator<<(csv::ofstream& ostm, const xmreg::transfer_details& td)
{

    ostm << xmreg::timestamp_to_str(td.m_block_timestamp, "%F");
    ostm << xmreg::timestamp_to_str(td.m_block_timestamp, "%T");
    ostm << td.m_block_height;
    ostm << td.tx_hash();
    ostm << td.m_internal_output_index;
    ostm << cryptonote::print_money(td.amount());

    return ostm;
}
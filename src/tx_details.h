//
// Created by mwo on 14/11/15.
//

#ifndef XMR2CSV_TXDATA_H
#define XMR2CSV_TXDATA_H



#include "../ext/minicsv.h"

#include "monero_headers.h"
#include "tools.h"

namespace xmreg
{

    using namespace cryptonote;
    using namespace crypto;
    using namespace std;


    struct transfer_details
    {
        uint64_t m_block_height;
        uint64_t m_block_timestamp;
        transaction m_tx;
        size_t m_internal_output_index;
        bool m_spent;


        crypto::hash tx_hash() const;

        uint64_t amount() const;
    };


    ostream&
    operator<<(ostream& os, const transfer_details& dt);


    vector<xmreg::transfer_details>
    get_belonging_outputs(const block& blk,
                          const transaction& tx,
                          const secret_key& private_view_key,
                          const public_key& public_spend_key,
                          uint64_t block_height = 0);

    bool
    is_output_ours(const size_t& output_index,
                   const transaction& tx,
                   const secret_key& private_view_key,
                   const public_key& public_spend_key);

    bool
    get_payment_id(const transaction& tx,
                   crypto::hash& payment_id);

    bool
    get_encrypted_payment_id(const transaction& tx,
                             crypto::hash8& payment_id);


}

template<>
csv::ostringstream&
operator<<(csv::ostringstream& ostm, const xmreg::transfer_details& td);


#endif //XMR2CSV_TXDATA_H

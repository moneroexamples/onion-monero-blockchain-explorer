//
// Created by igor on 16.8.18..
//

#ifndef XMRBLOCKS_DISPLAY_TYPES_H
#define XMRBLOCKS_DISPLAY_TYPES_H

#include <boost/variant.hpp>
#include "monero_headers.h"
#include <vector>

namespace xmreg {

using cryptonote::txout_to_key;
using cryptonote::txout_token_to_key;
using cryptonote::txin_to_key;
using cryptonote::txin_token_to_key;
using cryptonote::tx_out_type;
using crypto::public_key;
using crypto::key_image;


using displayable_output = boost::variant<txout_to_key, txout_token_to_key>;
using displayable_input = boost::variant<txin_to_key, txin_token_to_key>;


public_key const &get_public_key(displayable_output const &d_out);

key_image const &get_key_image(displayable_input const &d_in);

uint64_t get_amount(displayable_input const &d_in);

std::vector<uint64_t> const &get_key_offsets(displayable_input const &d_in);

tx_out_type get_out_type(displayable_input const &d_in);

tx_out_type get_out_type(displayable_output const &d_in);
}

#endif //XMRBLOCKS_DISPLAY_TYPES_H

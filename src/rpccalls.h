//
// Created by mwo on 13/04/16.
//


#ifndef CROWXMR_RPCCALLS_H
#define CROWXMR_RPCCALLS_H

#include "monero_headers.h"

#include <mutex>

namespace xmreg
{

using namespace cryptonote;
using namespace crypto;
using namespace std;


class rpccalls
{
    string deamon_url ;
    uint64_t timeout_time;

    std::chrono::milliseconds timeout_time_ms;

    epee::net_utils::http::url_content url;

    epee::net_utils::http::http_simple_client m_http_client;
    std::mutex m_daemon_rpc_mutex;

    string port;

public:

    rpccalls(string _deamon_url = "http:://127.0.0.1:18081",
             uint64_t _timeout = 200000);

    bool
    connect_to_monero_deamon();

    uint64_t
    get_current_height();

    bool
    get_mempool(vector<tx_info>& mempool_txs);

    bool
    commit_tx(tools::wallet2::pending_tx& ptx, string& error_msg);

    bool
    get_network_info(COMMAND_RPC_GET_INFO::response& info);

    bool
    get_dynamic_per_kb_fee_estimate(
            uint64_t grace_blocks,
            uint64_t& fee,
            string& error_msg);

};


}



#endif //CROWXMR_RPCCALLS_H

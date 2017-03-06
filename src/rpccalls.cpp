//
// Created by mwo on 13/04/16.
//

#include "rpccalls.h"

namespace xmreg
{


rpccalls::rpccalls(string _deamon_url,
         uint64_t _timeout)
        : deamon_url {_deamon_url},
          timeout_time {_timeout}
{
    epee::net_utils::parse_url(deamon_url, url);

    port = std::to_string(url.port);

    timeout_time_ms = std::chrono::milliseconds {timeout_time};

    m_http_client.set_server(
            deamon_url,
            boost::optional<epee::net_utils::http::login>{});
}

bool
rpccalls::connect_to_monero_deamon()
{
    //std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

    if(m_http_client.is_connected())
    {
        return true;
    }

    return m_http_client.connect(timeout_time_ms);
}

uint64_t
rpccalls::get_current_height()
{
    COMMAND_RPC_GET_HEIGHT::request   req;
    COMMAND_RPC_GET_HEIGHT::response  res;

    std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

    if (!connect_to_monero_deamon())
    {
        cerr << "get_current_height: not connected to deamon" << endl;
        return false;
    }

    bool r = epee::net_utils::invoke_http_json(
            "/getheight",
            req, res, m_http_client, timeout_time_ms);

    if (!r)
    {
        cerr << "Error connecting to Monero deamon at "
             << deamon_url << endl;
        return 0;
    }
    else
    {
        cout << "rpc call /getheight OK: " << endl;
    }

    return res.height;
}

bool
rpccalls::get_mempool(vector<tx_info>& mempool_txs)
{

    COMMAND_RPC_GET_TRANSACTION_POOL::request  req;
    COMMAND_RPC_GET_TRANSACTION_POOL::response res;

    std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

    if (!connect_to_monero_deamon())
    {
        cerr << "get_mempool: not connected to deamon" << endl;
        return false;
    }

    bool r = epee::net_utils::invoke_http_json(
            "/get_transaction_pool",
            req, res, m_http_client, timeout_time_ms);

    if (!r)
    {
        cerr << "Error connecting to Monero deamon at "
             << deamon_url << endl;
        return false;
    }


    mempool_txs = res.transactions;

    return true;
}


bool
rpccalls::commit_tx(tools::wallet2::pending_tx& ptx, string& error_msg)
{
    COMMAND_RPC_SEND_RAW_TX::request  req;
    COMMAND_RPC_SEND_RAW_TX::response res;

    req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(
            tx_to_blob(ptx.tx)
    );

    req.do_not_relay = false;

    std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

    if (!connect_to_monero_deamon())
    {
        cerr << "commit_tx: not connected to deamon" << endl;
        return false;
    }

    bool r = epee::net_utils::invoke_http_json(
            "/sendrawtransaction",
            req, res, m_http_client, timeout_time_ms);

    if (!r || res.status == "Failed")
    {
        error_msg = res.reason;

        cerr << "Error sending tx: " << res.reason << endl;
        return false;
    }

    return true;
}



}

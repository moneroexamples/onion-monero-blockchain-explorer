//
// Created by mwo on 13/04/16.
//

#ifndef CROWXMR_RPCCALLS_H
#define CROWXMR_RPCCALLS_H

#include "monero_headers.h"

namespace xmreg
{

    using namespace cryptonote;
    using namespace crypto;
    using namespace std;


    using request = cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request;
    using response = cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response;
    using http_simple_client = epee::net_utils::http::http_simple_client;


    class rpccalls
    {
        string deamon_url {"http:://127.0.0.1:18081"};
        uint64_t timeout_time {200000};

        epee::net_utils::http::url_content url;

        http_simple_client m_http_client;
        boost::mutex m_daemon_rpc_mutex;

        string port;

    public:

        rpccalls()
        {
            epee::net_utils::parse_url(deamon_url, url);

            port = std::to_string(url.port);
        }

        bool
        check_connection()
        {
            m_daemon_rpc_mutex.lock();

            return m_http_client.connect(url.host,
                                         port,
                                         timeout_time);
        }

        uint64_t
        get_current_height()
        {
            COMMAND_RPC_GET_HEIGHT::request   req;
            COMMAND_RPC_GET_HEIGHT::response  res;

            m_daemon_rpc_mutex.lock();

            bool r = epee::net_utils::invoke_http_json_remote_command2(
                    deamon_url + "/getheight",
                    req, res, m_http_client, timeout_time);

            m_daemon_rpc_mutex.unlock();

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
        get_mempool(vector<tx_info>& mempool_txs) {

            COMMAND_RPC_GET_TRANSACTION_POOL::request  req;
            COMMAND_RPC_GET_TRANSACTION_POOL::response res;

            m_daemon_rpc_mutex.lock();

            bool r = epee::net_utils::invoke_http_json_remote_command2(
                    deamon_url + "/get_transaction_pool",
                    req, res, m_http_client, timeout_time);

            m_daemon_rpc_mutex.unlock();

            if (!r)
            {
                cerr << "Error connecting to Monero deamon at "
                     << deamon_url << endl;
                return false;
            }

            mempool_txs = res.transactions;

            return true;
        }
    };


}



#endif //CROWXMR_RPCCALLS_H

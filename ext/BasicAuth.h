#pragma once


#include "base64.h"

namespace crow_contrib
{

using namespace std;

/*
 * Based on https://github.com/camsaul/BasicAuth
 */
class BasicAuth
{

private:
    string m_serverAuthString;
    string m_realm;

    bool passAuth(const string authHeader)
    {
        cout << authHeader.length() << ',' << m_serverAuthString.length() << '\n';
        cout << (authHeader == m_serverAuthString) << '\n';


        return authHeader.length() == m_serverAuthString.length()
               && authHeader == m_serverAuthString;
    }

public:

    struct context
    {
    };

    BasicAuth()
    {
        set("default","password","Authorization Required");
    }

    void set(const string username,
              const string password,
              const string realm = "Authorization Required")
     {
            string unencodedUserPass = username + ":" + password;
            int resultLen = 0;
            string encodedUserPass = base64(unencodedUserPass.c_str(),
                                            unencodedUserPass.length(),
                                            &resultLen);

            m_realm = realm;

            m_serverAuthString = string("Basic ") + encodedUserPass;

            cout << "m_serverAuthString: " << m_serverAuthString << '\n';
    }

    void before_handle(crow::request& req, crow::response& res, context& ctx)
    {
        cout << " - MESSAGE: " << req.get_header_value("authorization") << '\n';

        int count = req.headers.count("authorization");

        if (!count || !passAuth(req.get_header_value("authorization")))
        {
            res.clear();
            res.code = 401;
            res.add_header("WWW-Authenticate", "Basic realm=\""+m_realm+"\"");
            res.end();
            return;
        }
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx)
    {
        // no-op
    }

};
}
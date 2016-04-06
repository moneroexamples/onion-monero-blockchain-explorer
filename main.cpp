
#include "src/MicroCore.h"

#include "ext/crow/crow.h"
#include "mstch/mstch.hpp"
#include "ext/format.h"

#include <iostream>

using boost::filesystem::path;

using namespace std;

int main() {


    path blockchain_path {"/home/mwo/.bitmonero/lmdb"};

    fmt::print("Blockchain path      : {}\n", blockchain_path);


    std::string view{"{{#names}}Hi {{name}}!\n{{/names}}"};

    mstch::map context{
            {"names", mstch::array{
                    mstch::map{{"name", std::string{"Chris"}}},
                    mstch::map{{"name", std::string{"Mark"}}},
                    mstch::map{{"name", std::string{"Scott"}}},
            }}
    };

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
            ([&]() {
                return mstch::render(view, context);
            });

    app.port(8080).multithreaded().run();

    return 0;
}
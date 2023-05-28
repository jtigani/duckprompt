#pragma once

#include <string>
#include <vector>

namespace duckdb_httplib_openssl {
    class SSLClient;
}

struct HTTPSResponse {
    HTTPSResponse() :code(-1), response("") {}
    HTTPSResponse(int c, std::string r) :code(c), response(r) {}
    static HTTPSResponse InvalidResponse() {return HTTPSResponse(-1, "");}

    int code;
    std::string response;
};

class HTTPS {
public:
    HTTPS(std::string host);
    ~HTTPS();
    HTTPSResponse Post(
        std::string path, 
        const std::vector<std::pair<std::string, std::string>> & all_headers,
        std::string body);

private:
    duckdb_httplib_openssl::SSLClient* client_;
    std::string host_;
};

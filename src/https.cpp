#include <iostream>
#include <string>
#include <cstring>

#include "https.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.hpp"

#include "duckdb/common/exception.hpp"

// Set PROMPT_DEBUG = 1 to see request bodies / responses
// Set PROMPT_DEBUG = 2 to see headers as well.
int GetDebugLevel() {
    char * debug_env = std::getenv("PROMPT_DEBUG");
    if (debug_env == nullptr || strlen(debug_env) == 0) {return 0;}
    int level = debug_env[0] - '0';
    if (level < 0 || level > 9) {return 0;}
    return level;
}

static void logger(const duckdb_httplib_openssl::Request& request, const duckdb_httplib_openssl::Response& response) {
    int debug_level = GetDebugLevel();
    if (debug_level <= 0) {
        return;
    }
    std::cerr << "Remote Addr: " << request.remote_addr << " Port:" << request.remote_port << "\n";
    std::cerr << "Request:" << request.method << " " << request.path << "\n";

    if (debug_level > 1) {
        for (auto header : request.headers) {
            if (header.first == "Authorization") {
                std::cerr << "  " << header.first << ": " << "[Redacted]\n";
            } else {
                std::cerr << "  " << header.first << ": " << header.second << "\n";
            }
        }
    }

    std::cerr << "   Body:\n    " << request.body;
    std::cerr << "\n\nResponse Code: " << response.status << "\n";
    if (debug_level > 1) {
        for (auto header : response.headers) {
            std::cerr << "  " << header.first << ": " << header.second << "\n";
        }
    }
    std::cerr << "   Body:\n    " << response.body;
}


duckdb_httplib_openssl::SSLClient* GetClient(std::string host_port) {
    duckdb_httplib_openssl::SSLClient* client = new duckdb_httplib_openssl::SSLClient(host_port.c_str(), 443);
    client->set_follow_location(true);
    client->set_keep_alive(true);
    client->enable_server_certificate_verification(false);
    client->set_decompress(false);
    client->set_logger(logger);
    client->set_read_timeout(300); // seconds
    client->set_write_timeout(300); // seconds
    client->set_connection_timeout(300); // seconds
    return client;
}

HTTPS::HTTPS(std::string host) : client_(GetClient(host)) , host_(host) { }

HTTPS::~HTTPS() {
    if (client_ != nullptr) {
        delete client_;
        client_ = nullptr;
    }
}

HTTPSResponse HTTPS::Post(
    std::string path, const std::vector<std::pair<std::string, std::string>> & all_headers,
    std::string body) {

    if (!client_->is_valid()) {
        std::cerr << "SSL Client invalid\n";
        throw duckdb::IOException("Unable to open SSL path to %s", host_);
    }
    duckdb_httplib_openssl::Headers headers;
    for(auto h : all_headers) {
        headers.emplace(h.first, h.second);
    }

    auto uri = "https://" + host_ + "/" + path;
    auto res = client_->Post(uri.c_str(), headers, body.c_str(), body.size(), "application/json");

    if (res == nullptr) {
        std::cerr << "Post returned null\n";
        throw duckdb::IOException("No response for HTTP %s to '%s'", "POST", uri);
    }
    return HTTPSResponse(res->status, res->body);
}

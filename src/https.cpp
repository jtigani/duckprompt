#include <iostream>
#include <string>
#include <cstring>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.hpp"
#include "https.hpp"


static void logger(const duckdb_httplib_openssl::Request& request, const duckdb_httplib_openssl::Response& response) {
    std::cerr << "Remote Addr: " << request.remote_addr << " Port:" << request.remote_port << "\n";
    std::cerr << "Request:" << request.method << " " << request.path << "\n";
    for (auto header : request.headers) {
        if (header.first == "Authorization") {
            std::cerr << "  " << header.first << ": " << "[Redacted]\n";
        } else {
            std::cerr << "  " << header.first << ": " << header.second << "\n";
        }
    }
    std::cerr << "   Body:\n    " << request.body;  
    std::cerr << "\n\nResponse. Code: " << response.status << "Location: " << response.location;
    for (auto header : response.headers) {
        std::cerr << "  " << header.first << ": " << header.second << "\n";
    }
    std::cerr << "   Body:\n    " << response.body;  
}

bool GetDebugMode() {
    char * debug_env = std::getenv("PROMPT_DEBUG");
    return debug_env != nullptr && strlen(debug_env) > 0;
}

duckdb_httplib_openssl::SSLClient* GetClient(std::string host_port) {
    duckdb_httplib_openssl::SSLClient* client = new duckdb_httplib_openssl::SSLClient(host_port.c_str(), 443);
    client->set_follow_location(true);
    client->set_keep_alive(true);
    client->enable_server_certificate_verification(false);
    client->set_decompress(false);
    if (GetDebugMode()) {
        client->set_logger(logger);
    }
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
        std::cerr << "SSL Client invalid";
        return HTTPSResponse::InvalidResponse();
    }
    duckdb_httplib_openssl::Headers headers;
    for(auto h : all_headers) {
        headers.emplace(h.first, h.second);
    }

    auto uri = "https://" + host_ + "/" + path; 
    auto res = client_->Post(uri.c_str(), headers, body.c_str(), body.size(), "application/json");

    if (res != nullptr) {
        return HTTPSResponse(res->status, res->body);
    } else {
        std::cerr << "Post returned null";
        return HTTPSResponse::InvalidResponse();
    }
}

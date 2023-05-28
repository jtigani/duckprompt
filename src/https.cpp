#include <iostream>
#include <string>
#include <cstring>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "https.hpp"

Https::Https() {
    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    // ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();

}

Https::~Https() {
}

std::string Https::Get(std::string host,
    std::string uri, const std::vector<std::string> & headers) {

    return Request(host, uri, "GET", headers, "");
}

std::string Https::Post(std::string host, 
    std::string uri, const std::vector<std::string> & headers, 
    std::string post_data) {

    return Request(host, uri, "POST", headers, post_data);
}


std::string Https::Request(std::string host,
    std::string uri, std::string verb, const std::vector<std::string> & headers,
    std::string request_data) {
    // Create an SSL context and set up the SSL method
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == nullptr) {
        std::cerr << "Failed to create SSL context." << std::endl;
        return "";
    }

    std::string ssl_host = host;
    if (ssl_host.find(":") == std::string::npos) {
        // Add SSH Port
        ssl_host = host + ":443";
    }

    // Create an SSL BIO object to establish the connection
    BIO* bio = BIO_new_ssl_connect(ctx);
    if (bio == nullptr) {
        std::cerr << "Failed to create SSL BIO." << std::endl;
        SSL_CTX_free(ctx);
        return "";
    }

    // Set the hostname and port for the connection
    BIO_set_conn_hostname(bio, ssl_host.c_str());

    // Perform the connection
    if (BIO_do_connect(bio) <= 0) {
        std::cerr << "Failed to establish connection." << std::endl;
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return "";
    }

    // std::cerr << "Succesfully established SSL connection." << std::endl;

    // Perform the SSL handshake
    if (BIO_do_handshake(bio) <= 0) {
        std::cerr << "Failed to perform SSL handshake." << std::endl;
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return "";
    }

    // std::cerr << "Succesfully performed SSL handshake." << std::endl;
    // Send the HTTPS request
    std::string request = verb + " / HTTP/1.1\r\nHost: " + host + "\r\n";
    for (const std::string& header: headers) {
        request.append(header);
        request.append("\r\n");
    }
    request.append("\r\n");
    request.append(request_data);

    if (BIO_write(bio, request.c_str(), request.length()) <= 0) {
        std::cerr << "Failed to send HTTPS request." << std::endl;
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return "";
    }
    // std::cerr << "Succesfully sent HTTPS request." << std::endl;

    // Read the response
    char r[0x1000];
    memset(r, '\0', sizeof(r));
    std::string response;
    int len;
    while((len = BIO_read(bio, r, sizeof(r)-1)) > 0) {
        // std::cerr << "Succesfully read " << len << " bytes" << std::endl;
        // std::cerr << "Read " << r << std::endl; 
        response.append(r, len);
        memset(r, '\0', sizeof(r));
        if (len < sizeof(r) - 1) {
            break;
        }
    }
    // std::cerr << "Finished reading " << response.length() << " bytes" <<  std::endl;
    BIO_free_all(bio);
    SSL_CTX_free(ctx);

    return response;
}

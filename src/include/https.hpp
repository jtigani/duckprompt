#pragma once

#include <string>
#include <vector>

class Https {
public:
    Https();
    ~Https();
    std::string Get(std::string host,
        std::string uri, const std::vector<std::string> & headers);
    std::string Post(std::string host,
        std::string uri, const std::vector<std::string> & headers,
        std::string post_data);

private:
    std::string Request(std::string host,
        std::string uri, std::string verb, const std::vector<std::string> & headers, std::string request_data);
};

#pragma once

#include <vector>
#include <string>

#include "chat.hpp"

class QuackingDuck {
public:
    QuackingDuck();
    std::string Ask(std::string question);
    std::string ExplainSchema(std::string detail = "one sentence");

private:
    Chat chat_;
};

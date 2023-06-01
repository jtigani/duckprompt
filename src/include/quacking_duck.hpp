#pragma once

#include <vector>
#include <string>

#include "chat.hpp"

struct ExtractedSchema {
    std::vector<std::string> table_ddl;
    std::string SchemaToString() const;
};

class QuackingDuck {
public:
    QuackingDuck();
    // In order to get QuackingDuck to take into account schema, call
    // this before asking other questions.
    std::string ExplainSchema(const ExtractedSchema& extracted_schema); 

    // Ask a question, get a SQL query in response.
    std::string Ask(std::string question);
    
    // Analyze a query. This is useful for adding context before Fixup.
    std::string AnalyzeQuery(std::string query);

    // You should call Ask or AnalyzeQuery before calling FixupQuery.
    std::string FixupQuery(std::string error_message);

    bool HasSeenQuery(std::string query) {return query == query_;}

private:
    Chat chat_;
    std::string query_;
};

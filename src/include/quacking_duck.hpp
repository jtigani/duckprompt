#pragma once

#include <vector>
#include <string>

#include "chat.hpp"

struct ExtractedSchema {
    std::vector<std::string> table_ddl;
    std::string SchemaToString() const;
};

class DatabaseInterface {
  public:
    virtual void ExtractSchema(ExtractedSchema& ex) = 0;
    virtual std::string ValidateParse(std::string query) = 0;
    virtual std::string ValidateSemantics(std::string query) = 0;
};

// Runs natual language to SQL prompting.
class QuackingDuck {
public:
    QuackingDuck(DatabaseInterface& db, std::string model = "") : db_(db), chat_(model) { }

    // Returns a one sentance summary of a schema.
    std::string ExplainSchema();

    // Asks a Natural Language prompt, returns a SQL query.
    std::string Ask(std::string prompt);

    // Given a query that may or may not be syntactically correct,
    // returns a fixed version of the query.
    std::string FixupQuery(std::string query);

   private:
    // In order to get QuackingDuck to take into account schema, call
    // this before asking other questions.
    std::string ExplainSchemaPrompt(const ExtractedSchema& extracted_schema); 

    // Ask a question, get a SQL query in response.
    std::string AskPrompt(std::string question);
    
    // Analyze a query. This is useful for adding context before Fixup.
    std::string AnalyzeQueryPrompt(std::string query);

    // You should call Ask or AnalyzeQuery before calling FixupQuery.
    std::string FixupQueryPrompt(std::string error_message);

private:
    Chat chat_;
    DatabaseInterface& db_;
    std::string query_;
    std::string schema_summary_;
};

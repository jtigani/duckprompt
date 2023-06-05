#include <iostream>
#include <string>
#include <vector>
#include <regex>

#include "quacking_duck.hpp"
#include "chat.hpp"


#include "yyjson.hpp"


std::string QuackingDuck::ExplainSchema() {
    ExtractedSchema extracted_schema;
    db_.ExtractSchema(extracted_schema);
    return ExplainSchemaPrompt(extracted_schema);
}

std::string QuackingDuck::Ask(std::string prompt) {
    ExplainSchema();
    std::string query = AskPrompt(prompt);
    std::string fixed = FixupQuery(query);
    return (query == fixed)? query : AskPrompt(prompt);
}

std::string QuackingDuck::FixupQuery(std::string query) {
    std::string error = db_.ValidateParse(query);
    if (error.size() > 0) {
        AnalyzeQueryPrompt(query);
        return FixupQueryPrompt(error);
    }
    ExplainSchema();
    error = db_.ValidateSemantics(query);
    if (error.size() > 0) {
        AnalyzeQueryPrompt(query);
        return FixupQueryPrompt(error);
    }
    return query;
}

static const char * c_ask_template = "Output a single SQL query without any explanation and do "
        "not add anything to the query that was not part of the question. "
        "Make sure to only use tables and columns from the schema above and write a query "
        "to answer the following question:\n"
        "{question}"
        "\n";

std::string TemplateReplace(std::string prompt_template, std::initializer_list<std::string> args) {
    int ii = 0;
    std::string name;
    int idx = 0;
    for (auto cur = args.begin(); cur != args.end(); ++cur, ++idx) {
        if (idx % 2 == 0) {
            name = *cur;
            continue;
        }
        std::string value = *cur;
        int pos = prompt_template.find("{" + name + "}");
        if (pos == std::string::npos) {
            throw duckdb::IOException("Key %s not found in prompt template %s", name, prompt_template);
        }
        auto len = name.length() + 2;
        prompt_template = prompt_template.replace(pos, len, value);
    }
    return prompt_template;
}

std::string QuackingDuck::AskPrompt(std::string question) {
    chat_.SetSystemContext("You are a helpful assistant that can generate Postgresql code based on "
        "the user input. You do not respond with any human readable text, only SQL code.");
    std::string whole_prompt = TemplateReplace(c_ask_template, {"question", question});
    query_ = chat_.SendPrompt(whole_prompt);
    return query_;
}

static const char* c_schema_template = "SQL schema of my database:\n"
        "```{schema}```"
        "\nExplain in one sentence what the data is about";
std::string QuackingDuck::ExplainSchemaPrompt(const ExtractedSchema& extracted_schema) {
    if (schema_summary_.length() > 0) {
        return schema_summary_;
    }
    chat_.Reset("You are a helpful assistant that can generate an human redable summary "
            " of database content based on the schema.");

    std::string whole_prompt = TemplateReplace(c_schema_template, {"schema", extracted_schema.SchemaToString()});

    schema_summary_ = chat_.SendPrompt(whole_prompt);
    return schema_summary_;
}

std::string ExtractedSchema::SchemaToString() const {
    std::string schema = "";
    for (std::string current_table_ddl : table_ddl) {
        schema.append(current_table_ddl);
        schema.append("\n");
    }

    return schema;
}

static const char* c_analyze_template =  "Here is my SQL query:\n"
        "```{query}```"
        "\nPlease respond with the SQL query and an explaiation about what the query will do.";

std::string QuackingDuck::AnalyzeQueryPrompt(std::string query) {
    if (query_.length() > 0) {
        return query_;
    }
    chat_.SetSystemContext("You are a helpful assistant that is an expert in SQL code who can output "
           "a human readable summary of a SQL query.");
    std::string whole_prompt = TemplateReplace(c_analyze_template, {"query", query});
    std::string result = chat_.SendPrompt(whole_prompt);
    query_ = query;
    return result;
}

// Watch out, the . character doesn't match newlines, so we need to handle them specially.
static const std::string c_tripple_quote_enclosure_str = "```\\n*((.|\\n)*.?)```";
static const std::regex c_tripple_quote_enclosure_pattern(c_tripple_quote_enclosure_str);

std::string ExtractMarkdownSelect(const std::string& message) {
    std::smatch match;
    if (!std::regex_search(message, match, c_tripple_quote_enclosure_pattern)) {
        return message;
    }
    std::string matched_string = match[1];
    return matched_string;
}

std::string ExtractSelect(std::string message) {
    message = ExtractMarkdownSelect(message);
    size_t pos = message.find(" SELECT");
    if (pos == std::string::npos ) {
        pos = message.find("\nSELECT");
    }

    std::string result = (pos == std::string::npos)
        ? message
        : message.substr(pos + 1); // go beyond the ' '.

    pos = result.find(";\n");
    result = (pos == std::string::npos)
        ? result
        : result.substr(0, pos);

    return result;
}

static const char* c_fixup_template = "When I ran the previous query, I got the following exception:\n"
        "{error_message}"
        "\nPlease correct and output only the resulting SQL code.";

std::string QuackingDuck::FixupQueryPrompt(std::string error_message) {
    if (query_.length() == 0) {
        // Should have called Ask or AnalyzeQuery to set the query.
        return "";
    }

    chat_.SetSystemContext("You are a helpful assistant that can generate Postgresql code based on "
        "the user input. You do not respond with any human readable text, only SQL code.");

    std::string whole_prompt = TemplateReplace(c_fixup_template, {"error_message", error_message});

    std::string response = chat_.SendPrompt(whole_prompt);
    // This is a hack .... if the select statement is at the end, return it.
    query_ = ExtractSelect(response);
    return query_;
}

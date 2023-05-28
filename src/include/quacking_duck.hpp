pragma once

#include <vector>
#include <string>

class QuackingDuck {
public:
    QuackingDuck();
    std::string ask(std::string question, bool debug = false);

    std::string explain_schema(std::string detail = "one sentence");
private:
    std::string _get_schemas();

    std::vector<std::string> _schema_summary_internal(std::string detail = "one sentence");

    std::vector<std::string> _generate_sql(std::string question, bool debug = false);

    std::string _regenerate_sql(std::string content_prompt, 
        std::string content_summary, 
        std::string sql_prompt, std::string sql_query, std::string error, bool debug = false);

private:
    std::string schemas;
};

#pragma once

#include "duckdb.hpp"

#include "quacking_duck.hpp"

namespace duckdb {

class QuackExtension : public Extension {
public:
	void Load(DuckDB &db) override;
	std::string Name() override;

private:
    QuackingDuck quacking_duck;
};

} // namespace duckdb

# duckprompt

This is a simple DuckDB extension that calls OpenAI's ChatGPT to do natural language queries. It is based on work from
Till Döhmen that he described in his blog post here: https://tdoehmen.github.io/blog/2023/03/07/quackingduck.html. 

To summarize, we first call ChatGPT to provide the schema, then we call again to get it generate a SQL query. Till goes a step
further and checks whether the output parses, and if it doesn't asks ChatGPT to fix it. We don't go so far yet. We also don't
use samples of the data, only the schema, which makes it harder for chatGPT to answer some questions. But it works surprisingly 
well with the toy schema I've been working with, even coming up with JOIN queries.

---


## Running the extension

To run, you'll need an openai key. You can get one here:
https://platform.openai.com/account/api-keys

To run, first install and load the extension.
```
force install '<path to duckprompt>/duckprompt'
load duckprompt

```

To run a natual language query, use `pragma prompt_query`. For example:
```
D pragma prompt_query('Return the minimum amount paid by customers who used a visa card (debit or credit) to purchase a product.') ;
┌───────────┐
│ min(paid) │
│  double   │
├───────────┤
│     360.0 │
└───────────┘
```

If you want to see what queries actually get run, try the `prompt_sql` table function:
```
D select * from prompt_sql('Return the minimum amount paid by customers who used a visa card (debit or credit) to purchase a product.') ;
┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ prompt('Return the minimum amount paid by customers who used a visa card (debit or credit) to purchase a product.') │
│                                                       varchar                                                       │
├─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ SELECT MIN(paid) FROM sales WHERE type_of_payment LIKE '%visa%';                                                    │
└─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

You can also "fix" a query. To fix a query, run `prompt_fixup' table function. This can detect errors with syntax,
usage, and even fix problems relating to the schame.
```
D select * from prompt_fixup("SEELECT * from customers");
┌─────────────────────────┐
│          query          │
│         varchar         │
├─────────────────────────┤
│ SELECT * FROM customers │
└─────────────────────────┘
```

Also note that if you want to set up sample data with a sales star schema you
can run the following script
```
build/release/duckdb -init ./scripts/build_sample_db.sql ./build/release/sales.db 
```

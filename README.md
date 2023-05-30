# duckprompt

This is a simple DuckDB extension that calls OpenAI's ChatGPT to do natural language queries. It is based on work from
Till Döhmen that he described in his blog post here: https://tdoehmen.github.io/blog/2023/03/07/quackingduck.html. 

To summarize, we first call ChatGPT to provide the schema, then we call again to get it generate a SQL query. Till goes a step
further and checks whether the output parses, and if it doesn't asks ChatGPT to fix it. We don't go so far yet. We also don't
use samples of the data, only the schema, which makes it harder for chatGPT to answer some questions. But it works surprisingly 
well with the toy schema I've been working with, even coming up with JOIN queries.

---


## Running the extension

To run, first install and load the extension.
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

If you want to see what queries actually get run, try the `prompt` function:
```
D select prompt('Return the minimum amount paid by customers who used a visa card (debit or credit) to purchase a product.') ;
┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ prompt('Return the minimum amount paid by customers who used a visa card (debit or credit) to purchase a product.') │
│                                                       varchar                                                       │
├─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ SELECT MIN(paid) FROM sales WHERE type_of_payment LIKE '%visa%';                                                    │
└─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef size_t symbol_table_index_t;
typedef size_t register_table_index_t;

symbol_table_index_t symbol_add(void);

// N.B. Illegal to delete unused location!
void symbol_del(symbol_table_index_t);

bool symbol_give_me_my_stuff(size_t num_symbols, const char **out_registers, symbol_table_index_t symbol0, ...);

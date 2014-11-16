#pragma once

#include <stdbool.h>

typedef enum {
    SYM_REGI,
    SYM_ADDR,
} symbol_type_t;

// TODO: Could be static/opaque
typedef enum {
    R_EAX,
    R_ECX,
    R_EDX,
    R_ESI,
    R_EDI,
} register_t;

// TODO: Could be static/opaque
typedef union {
    register_t regis;
    memory_addr_t addr;
} symbol_location_t;

// TODO: Could be static/opaque
typedef struct {
    symbol_type_t type;
    symbol_location_t loc;
} symbol_entry_t;

typedef size_t symbol_table_index_t;

// TODO: Could be static/opaque
static symbol_entry_t *symbol_table;

symbol_table_index_t symbol_add(void);

// TODO: Could be static/opaque
symbol_entry_t symbol_get(symbol_table_index_t);

void symbol_del(symbol_table_index_t);

// TODO: Could be static/opaque
char *symbol_register_string(register_t);

bool symbol_give_me_my_stuff(size_t num_symbols, const char **out_registers, sybol_table_index_t symbol0, ...);

// TODO: Could be static/opaque
static bool symbol_request_reg(size_t, symbol_table_index_t, ...);

// TODO: Could be static/opaque
static bool symbol_demote_to_mem(symbol_table_index_t);

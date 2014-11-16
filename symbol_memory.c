#include "symbol_memory.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define INIT_SYMB_TAB_LEN ((size_t) 5)
#define SYMB_TAB_GROWTH_FACTOR ((size_t) 2)

#define REG_TAB_INVALID_ADDR ((size_t) -1)
#define REG_TAB_FULL ((size_t) -1)

typedef struct {
    const char *const repr;
    bool in_use;
    symbol_table_index_t symb;
} register_entry_t;

// Contains only scratch registers.
static register_entry_t register_table[] = {
    {.repr = "%eax", .in_use = false, .symb = REG_TAB_INVALID_ADDR},
    {.repr = "%ecx", .in_use = false, .symb = REG_TAB_INVALID_ADDR},
    {.repr = "%edx", .in_use = false, .symb = REG_TAB_INVALID_ADDR},
    {.repr = "%esi", .in_use = false, .symb = REG_TAB_INVALID_ADDR},
    {.repr = "%edi", .in_use = false, .symb = REG_TAB_INVALID_ADDR},
};
static const size_t REGISTER_TABLE_LEN = sizeof register_table / sizeof(*register_table);

// XXX: Add memory_something_index_t, but there are caveats:
//  - globals cannot safely be stacked without significant care
//  - want to avoid stack fragmentation (e.g. holes from unstacking)
typedef bool memory_something_index_t;

typedef enum {
    // N.B. SYMB_UNUSED *must* be 0 because we calloc() the symbol table!
    SYMB_UNUSED = 0,
    SYMB_REGI,
    SYMB_ADDR,
} symbol_type_t;

typedef union {
    register_table_index_t regis;
    memory_something_index_t addr;
} symbol_location_t;

typedef struct {
    symbol_type_t type;
    symbol_location_t loc;
} symbol_entry_t;

static symbol_entry_t *symbol_table = NULL;
static size_t symbol_table_len = INIT_SYMB_TAB_LEN;

static symbol_table_index_t next_avail_symb_tab_entry(void);
static register_table_index_t next_avail_reg_tab_entry(void);
static bool symbol_request_reg(size_t, const symbol_table_index_t *);
static bool symbol_demote_to_mem(symbol_table_index_t);

symbol_table_index_t symbol_add(void) {
    if(symbol_table == NULL) {
        symbol_table = calloc(INIT_SYMB_TAB_LEN, sizeof(symbol_entry_t));
        // TODO: Compiler error if out of memory!
    }

    symbol_table_index_t symb_spot = next_avail_symb_tab_entry();
    register_table_index_t reg_spot = next_avail_reg_tab_entry();
    if(reg_spot != REG_TAB_FULL) { // Going into register
        register_table[reg_spot].in_use = true;
        register_table[reg_spot].symb = symb_spot;
    }
    else { // Going into memory
        // XXX: Actually do memory fallback here!
        assert(false);
    }

    return symb_spot;
}

void symbol_del(symbol_table_index_t index) {
    assert(symbol_table != NULL);

    switch(symbol_table[index].type) {
        case SYMB_UNUSED: {
            assert(false);
            break;
        }
        case SYMB_REGI: {
            register_table_index_t r_idx = symbol_table[index].loc.regis;
            register_table[r_idx].in_use = false;
            register_table[r_idx].symb = REG_TAB_INVALID_ADDR;
            break;
        }
        case SYMB_ADDR: {
            // XXX: Actually handle memory!
            assert(false);
            break;
        }
    }

    symbol_table[index].type = SYMB_UNUSED;
}

bool symbol_give_me_my_stuff(size_t num_symbols, const char **out_registers, symbol_table_index_t symbol0, ...) {
    // XXX: Implement this, using symbol_request_reg()
    assert(false);
    return false;
}

static symbol_table_index_t next_avail_symb_tab_entry(void) {
    assert(symbol_table != NULL);

    // Look for an existing unused symbol table entry.
    for(symbol_table_index_t index = 0; index < symbol_table_len; ++index) {
        if(symbol_table[index].type == SYMB_UNUSED) {
            return index;
        }
    }

    // Couldn't find any available entries, so grow the table.
    size_t old_len = symbol_table_len;
    symbol_table_len *= SYMB_TAB_GROWTH_FACTOR;
    realloc(symbol_table, symbol_table_len * sizeof(*symbol_table));
    // TODO: Compiler error if out of memory
    memset(symbol_table + old_len, 0, (symbol_table_len - old_len) * sizeof(*symbol_table));
    return old_len;
}

// N.B. Returns REG_TAB_FULL on (non-fatal) failure
static register_table_index_t next_avail_reg_tab_entry(void) {
    for(register_table_index_t index = 0; index < REGISTER_TABLE_LEN; ++index) {
        if(!register_table[index].in_use) {
            return index;
        }
    }
    return REG_TAB_FULL;
}

static bool symbol_request_reg(size_t num_requested, const symbol_table_index_t *symbols) {
    // XXX: Implement this function!
    assert(false);
    return false;

    // For each one of the symbols:
        // Check whether it's already stored in a register
            // If not:
                // Call next_avail_reg_tab_entry()
                    // If true, use that location
                    // If false:
                        // For each used register table location
                            // If not pointing to another requested symbol index:
                                // Call symbol_demote_to_mem() on it
                                    // On failure, the world is over
                                // Use this newly-freed location
                    // Setup the location and return it
}

static bool symbol_demote_to_mem(symbol_table_index_t index) {
    // XXX: Actually do memory stuff!
    return false;
}

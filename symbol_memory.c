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
//  IDEA: use another array similar to register thingy, indexed relative to %rbp, but of adjustable size
//  IDEA: whenever we alloc in mem, use a tagged union so that all values are the same size + stay relatively alligned
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
static bool symbol_promote_to_reg(register_table_index_t, symbol_table_index_t);

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
    symbol_table_index_t symbols[num_symbols];
    va_list list;
    va_start(list, symbol0);
    symbols[0] = symbol0;
    for(unsigned idx = 1; idx < num_symbols; ++idx) {
        symbols[idx] = va_arg(list, symbol_table_index_t);
    }
    va_end(list);
    return symbol_request_reg(num_symbols, symbols);
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
    // For each of the requested symbols
    for(unsigned index = 0; index < num_requested; ++index) {
        // If not already stored in a register
        if(symbol_table[symbols[index]].type != SYMB_REGI) {
            bool successfully_promoted = false;

            // Call next_avail_reg_tab_entry()
            register_table_index_t r_idx = next_avail_reg_tab_entry();
            if(r_idx == REG_TAB_FULL) {
                // For each used register table location (i.e. all of them)
                for(unsigned other_r_idx = 0; other_r_idx < REGISTER_TABLE_LEN; ++other_r_idx) {
                    unsigned potentially_conflicting;
                    for(potentially_conflicting = 0; potentially_conflicting < num_requested; ++potentially_conflicting) {
                        if(register_table[other_r_idx].symb == symbols[potentially_conflicting]) {
                            // The client also requested the symbol stored in this register. Skip further checks and try a different (destination) register.
                            break;
                        }
                    }
                    // We made it all the way through, so this one is safe to swap out!
                    if(potentially_conflicting == num_requested) {
                        bool safe_and_sound = symbol_demote_to_mem(register_table[other_r_idx].symb);
                        // TODO: More graceful user-facing error here
                        assert(safe_and_sound);

                        // Use this newly-freed location
                        safe_and_sound = symbol_promote_to_reg(other_r_idx, symbols[index]);
                        // TODO: More graceful user-facing error here
                        assert(safe_and_sound);
                        successfully_promoted = true;
                        break;
                    }
                }
            }
            else { // register_table is not full: place in empty space
                bool safe_and_sound = symbol_promote_to_reg(r_idx, symbols[index]);
                // TODO: More graceful user-facing error here
                assert(safe_and_sound);
                successfully_promoted = true;
            }

            if(!successfully_promoted) {
                // We were unable to promote one of the missing values!
                return false;
            }
        }
    }

    // Sanity-check!
#ifndef NDEBUG
    for(unsigned index = 0; index < num_requested; ++index) {
        // If not already stored in a register
        if(symbol_table[symbols[index]].type != SYMB_REGI) {
            // We should have fixed this already!
            assert(false);
        }
    }
#endif

    // All requested values were either already in registers or promoted successfully.
    return true;
}

static bool symbol_demote_to_mem(symbol_table_index_t index) {
    // XXX: Actually do memory stuff!
    return false;
}

// N.B. It is an error to promote into a nonfree register ("...not to mention immoral" -- RMS)
static bool symbol_promote_to_reg(register_table_index_t dest, symbol_table_index_t benefactor) {
    assert(!register_table[dest].in_use);

    // XXX: Actually do memory stuff!
    return false;
}

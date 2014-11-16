#include "et_compiler.h"
#include "parse_tree.h"
#include "symbol_memory.h"

#include <assert.h>
#include <search.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define VAR_HASHTAB_LEN ((size_t) 64)

// N.B. We're storing the symbol_table_index_t s as casted void * s
static struct hsearch_data var_hashtab;
static bool var_hashtab_has_been_initialized = false;

// N.B. Associated functions return indices that are either symbol_table_index_t s or literal integers,
// depending on the verdict of questionable_return_props() operating on the child node's type.
typedef union {
    symbol_table_index_t indirect;
    int direct;
} questionable_return_t;

// Tag for the above type, provided by questionable_return_props()
typedef struct {
    bool indirect;
    bool cleanup;
} questionable_properties_t;

static questionable_properties_t questionable_return_props(parse_node_tag_t node_type);
static questionable_return_t code_gen_rec(const parse_node_t *expression);
static symbol_table_index_t code_variable(const parse_node_var_t *variable);
static symbol_table_index_t code_operate(const parse_node_operation_t *operation);
static const char * code_gen_op_to_mnem(parse_node_operator_t operr);

void code_gen(const parse_node_t *expression) {
    questionable_return_t ex_ret = code_gen_rec(expression);
    questionable_properties_t what_i_need_to_know = questionable_return_props(expression->type);
    if(what_i_need_to_know.indirect && what_i_need_to_know.cleanup) {
        symbol_del(ex_ret.indirect);
    }
}

static questionable_properties_t questionable_return_props(parse_node_tag_t node_type) {
    switch(node_type) {
        case NODE_TYPE_INT: {
            return (questionable_properties_t) { .indirect = false, .cleanup = false };
        }
        case NODE_TYPE_VAR: {
            return (questionable_properties_t) { .indirect = true, .cleanup = false };
        }
        case NODE_TYPE_OPERATION: {
            return (questionable_properties_t) { .indirect = true, .cleanup = true };
        }
        default: {
            assert(false);
            return (questionable_properties_t) { .indirect = false, .cleanup = false };
        }
    }
}

static questionable_return_t code_gen_rec(const parse_node_t *expression) {
    assert(expression);

    switch(expression->type) {
        case NODE_TYPE_INT:
            return (questionable_return_t){ .direct = expression->contents.integer.value };
        case NODE_TYPE_VAR:
            return (questionable_return_t) { .indirect = code_variable(&(expression->contents.variable)) };
        case NODE_TYPE_OPERATION:
            return (questionable_return_t) { .indirect = code_operate(&(expression->contents.operation)) };
        default:
            assert(false);
            return (questionable_return_t) { .direct = -1};
    }
}

static symbol_table_index_t code_variable(const parse_node_var_t *variable) {
    // TODO: All the assignment-checking, hashing, and compiler errors
    if(!var_hashtab_has_been_initialized) {
        hcreate_r(VAR_HASHTAB_LEN, &var_hashtab);
    }
    // N.B. We're storing the symbol_table_index_t s as casted void * s
    ENTRY *entry;
    symbol_table_index_t st_idx;

    if(variable->declaration) {
        if(hsearch_r((ENTRY) { .key = (char *) variable->identifier }, FIND, &entry, &var_hashtab)) { // Succeeds
            // TODO: This should be a user-facing check (as it ensures this isn't a duplicate declaration)!
            assert(false);
        }
        st_idx = symbol_add();
        int res = hsearch_r((ENTRY) { .key = (char *) variable->identifier, .data = (symbol_table_index_t *) st_idx }, ENTER, &entry, &var_hashtab);
        if(!res) { // Fails
            // TODO: User-facing "out of symbols" error
            assert(false);
        }
    }
    else  {
        if(!hsearch_r((ENTRY) { .key = (char *) variable->identifier }, FIND, &entry, &var_hashtab)) { // Fails
            // TODO: This should be a user-facing check (as it ensures we don't use nonexistant variables)!
            assert(false);
        }
        st_idx = (symbol_table_index_t) entry->data;
    }

    // Implied by declaration, but possible even without
    if(variable->assignment) {
        questionable_properties_t subprops = questionable_return_props(variable->subexpr->type);
        if(subprops.indirect) {
            const char *myregs[2];
            symbol_table_index_t sub_idx = code_gen_rec(variable->subexpr).indirect;
            symbol_give_me_my_stuff(2, myregs, st_idx, sub_idx);
            printf("\tmovl %s, %s\n", myregs[0], myregs[1]);
            if(subprops.cleanup) {
                symbol_del(sub_idx);
            }
        } else { // direct
            const char *myreg[1];
            symbol_give_me_my_stuff(1, myreg, st_idx);
            printf("\tmovl %s, $%d\n", myreg[0], code_gen_rec(variable->subexpr).direct);
        }
    }

    return st_idx;
}

static symbol_table_index_t code_operate(const parse_node_operation_t *operation) {
    switch(operation->operr) {
        case OP_NOOP:
            return -1;
        case OP_ADD2:
        case OP_SUB2:
            assert(operation->num_ops == 2);
            if (questionable_return_props(operation->ops[0]->type).indirect &&
                questionable_return_props(operation->ops[1]->type).indirect) {
                symbol_table_index_t sindex = symbol_add();
                const char * symbol_text[1];
                if( ! symbol_give_me_my_stuff(1, symbol_text, sindex) ) {
                    // TODO: Don't die.
                    assert(false);
                }
                printf("\tmovl $%d, %s\n", code_gen_rec(operation->ops[0]).direct, symbol_text[0]);
                printf("\t%s $%d, %s\n", code_gen_op_to_mnem(operation->operr), code_gen_rec(operation->ops[1]).direct, symbol_text[0]);

                return sindex;
            }
            else if (questionable_return_props(operation->ops[0]->type).indirect) {
                symbol_table_index_t dindex = code_gen_rec(operation->ops[1]).indirect;
                symbol_table_index_t sindex = symbol_add();
                const char * symbol_text[2];
                if( ! symbol_give_me_my_stuff(2, symbol_text, sindex, dindex) ) {
                    // TODO: Don't die.
                    assert(false);
                }
                printf("\tmovl $%d, %s\n", code_gen_rec(operation->ops[0]).direct, symbol_text[0]);
                printf("\t%s %s, %s\n", code_gen_op_to_mnem(operation->operr), symbol_text[1], symbol_text[0]);
                if(questionable_return_props(operation->ops[1]->type).cleanup) {
                    symbol_del(dindex);
                }

                return sindex;
            }
            else if (questionable_return_props(operation->ops[1]->type).indirect) {
                symbol_table_index_t dindex = code_gen_rec(operation->ops[0]).indirect;
                const char * symbol_text[1];
                if( ! symbol_give_me_my_stuff(1, symbol_text, dindex) ) {
                    // TODO: Live, damn you!
                    assert(false);
                }
                printf("\t%s $%d, %s\n", code_gen_op_to_mnem(operation->operr), code_gen_rec(operation->ops[1]).direct, symbol_text[0]);

                return dindex;
            }
            else {
                symbol_table_index_t dindex = code_gen_rec(operation->ops[0]).indirect;
                symbol_table_index_t sindex = code_gen_rec(operation->ops[1]).indirect;
                const char * symbol_text[2];
                if( ! symbol_give_me_my_stuff(2, symbol_text, sindex, dindex) ) {
                    // TODO: Die if you must
                    assert(false);
                }
                printf("\t%s %s %s\n", code_gen_op_to_mnem(operation->operr), symbol_text[0], symbol_text[1]);
                if(questionable_return_props(operation->ops[1]->type).cleanup) {
                    symbol_del(sindex);
                }

                return dindex;
            }

            // TODO: remove after cases above are complete (they should all return).
            return -1;
        default:
            assert(false);
            return -1;
    }
}

static const char * code_gen_op_to_mnem(parse_node_operator_t operr) {
    switch(operr) {
        case OP_ADD2:
            return "addl";
        case OP_SUB2:
            return "subl";
        default:
            assert(false);
            return "faill";
    }
}

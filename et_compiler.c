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
            return (questionable_properties_t) { .indirect = true, .cleanup = true };
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
            printf("\tmovl %s, %s\n", myregs[1], myregs[0]);
            if(subprops.cleanup) {
                symbol_del(sub_idx);
            }
        } else { // direct
            const char *myreg[1];
            symbol_give_me_my_stuff(1, myreg, st_idx);
            printf("\tmovl $%d, %s\n", code_gen_rec(variable->subexpr).direct, myreg[0]);
        }
    }

    // Make a copy so that we don't clobber the variable's value during calculations.
    symbol_table_index_t tmp_idx = symbol_add();
    const char *myregs[2];
    symbol_give_me_my_stuff(2, myregs, st_idx, tmp_idx);
    printf("\tmovl %s, %s\n", myregs[0], myregs[1]);

    return tmp_idx;
}

static symbol_table_index_t code_operate(const parse_node_operation_t *operation) {
    static int jump_target_num = 0;

    switch(operation->operr) {
        case OP_NOOP:
            return -1;
        case OP_ADD2:
        case OP_SUB2:
            assert(operation->num_ops == 2);
            if (!questionable_return_props(operation->ops[0]->type).indirect &&
                !questionable_return_props(operation->ops[1]->type).indirect) {
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
            else if (!questionable_return_props(operation->ops[0]->type).indirect) {
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
            else if (!questionable_return_props(operation->ops[1]->type).indirect) {
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
                printf("\t%s %s, %s\n", code_gen_op_to_mnem(operation->operr), symbol_text[0], symbol_text[1]);
                if(questionable_return_props(operation->ops[1]->type).cleanup) {
                    symbol_del(sindex);
                }

                return dindex;
            }

            // TODO: remove after cases above are complete (they should all return).
            return -1;
        case OP_EQUL:
        case OP_NEQL:
        case OP_LESS:
        case OP_GREA:
            assert(operation->num_ops == 2);
            bool swapped = false;
            if(!questionable_return_props(operation->ops[0]->type).indirect && !questionable_return_props(operation->ops[1]->type).indirect) { // both direct
                symbol_table_index_t dindex = symbol_add();
                const char * ret_symbol_text[1];
                if( ! symbol_give_me_my_stuff(1, ret_symbol_text, dindex) ) {
                    // TODO: Die die die
                    assert(false);
                }
                //XXX: Oops, always does equals, no others work.
                switch(operation->operr) {
                    case OP_EQUL:
                        if(code_gen_rec(operation->ops[0]).direct == code_gen_rec(operation->ops[1]).direct) {
                            printf("\tmovl $1, %s\n", ret_symbol_text[0]);
                        }
                        else {
                            printf("\tmovl $0, %s\n", ret_symbol_text[0]);
                        }
                        break;
                    case OP_NEQL:
                        if(code_gen_rec(operation->ops[0]).direct != code_gen_rec(operation->ops[1]).direct) {
                            printf("\tmovl $1, %s\n", ret_symbol_text[0]);
                        }
                        else {
                            printf("\tmovl $0, %s\n", ret_symbol_text[0]);
                        }
                        break;
                    case OP_GREA:
                        if(code_gen_rec(operation->ops[0]).direct > code_gen_rec(operation->ops[1]).direct) {
                            printf("\tmovl $1, %s\n", ret_symbol_text[0]);
                        }
                        else {
                            printf("\tmovl $0, %s\n", ret_symbol_text[0]);
                        }
                        break;
                    case OP_LESS:
                        if(code_gen_rec(operation->ops[0]).direct < code_gen_rec(operation->ops[1]).direct) {
                            printf("\tmovl $1, %s\n", ret_symbol_text[0]);
                        }
                        else {
                            printf("\tmovl $0, %s\n", ret_symbol_text[0]);
                        }
                        break;
                    default:
                        assert(false); // Never ever
                        break;
                }
                return dindex; //I want to return here fo sho, not so for the later cases
            }
            else if(questionable_return_props(operation->ops[0]->type).indirect && questionable_return_props(operation->ops[1]->type).indirect) { // both indirect
                symbol_table_index_t lindex = code_gen_rec(operation->ops[0]).indirect;
                symbol_table_index_t rindex = code_gen_rec(operation->ops[1]).indirect;
                const char * symbol_text[2];
                if( ! symbol_give_me_my_stuff(2, symbol_text, lindex, rindex) ) {
                    // TODO: Die if you must
                    assert(false);
                }
                printf("\tcmpl %s, %s\n", symbol_text[1], symbol_text[0]);
                symbol_del(rindex);
                symbol_del(lindex);
            }
            else { // one direct one indirect
                int numba;
                symbol_table_index_t sindex;
                if(!questionable_return_props(operation->ops[0]->type).indirect) {
                    numba = code_gen_rec(operation->ops[0]).direct;
                    sindex = code_gen_rec(operation->ops[1]).indirect;
                    swapped = true;
                }
                else {
                    numba = code_gen_rec(operation->ops[1]).direct;
                    sindex = code_gen_rec(operation->ops[0]).indirect;
                    swapped = false;
                }
                const char * symbol_text[1];
                if( ! symbol_give_me_my_stuff(1, symbol_text, sindex) ) {
                    // TODO: Die if you must
                    assert(false);
                }
                printf("\tcmpl $%d, %s\n", numba, symbol_text[0]);
                symbol_del(sindex);
            }
            // Get here because it's in a second instruction
            symbol_table_index_t dindex = symbol_add();
            const char * ret_symbol_text[1];
            if( ! symbol_give_me_my_stuff(1, ret_symbol_text, dindex) ) {
                // TODO: Die die die
                assert(false);
            }
            // XXX: Hmm, greater and less than may use the wrong assembly operation?
            const char *op_mnem;
            if(!swapped || operation->operr == OP_EQUL || operation->operr == OP_NEQL) {
                op_mnem = code_gen_op_to_mnem(operation->operr);
            } else { // If swapped and less than or greater then
                op_mnem = code_gen_op_to_mnem((operation->operr == OP_LESS) ? OP_GREA : OP_LESS);
            }
            printf("\t%s cmp_ll%d\n", op_mnem, jump_target_num);
            printf("\tmovl $0, %s\n", ret_symbol_text[0]);
            printf("\tjmp cmp_ll%d\n", jump_target_num+1);
            printf("cmp_ll%d:\n", jump_target_num);
            printf("\tmovl $1, %s\n", ret_symbol_text[0]);
            printf("cmp_ll%d:\n", jump_target_num+1);
            jump_target_num += 2;
            return dindex;
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
        case OP_EQUL:
            return "je";
        case OP_NEQL:
            return "jne";
        case OP_GREA:
            return "jg";
        case OP_LESS:
            return "jl";
        default:
            assert(false);
            return "faill";
    }
}

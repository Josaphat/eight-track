#include "et_compiler.h"
#include "parse_tree.h"
#include "symbol_memory.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

// N.B. Associated functions return indices that are either symbol_table_index_t s or literal integers,
// depending on whether the child node is a constant or not, respectively.
typedef union {
    symbol_table_index_t indirect;
    int direct;
} questionable_return_t;

static questionable_return_t code_gen_rec(const parse_node_t *expression);
static symbol_table_index_t code_operate(const parse_node_operation_t *operation);
static const char * code_gen_op_to_mnem(parse_node_operator_t operr);

void code_gen(const parse_node_t *expression) {
    questionable_return_t ex_ret = code_gen_rec(expression);
    if(expression->type != NODE_TYPE_INT) { // Must be indirect, got to clean up
        symbol_del(ex_ret.indirect);
    }
}

static questionable_return_t code_gen_rec(const parse_node_t *expression) {
    assert(expression);

    switch(expression->type) {
        case NODE_TYPE_INT:
            return (questionable_return_t){ .direct = expression->contents.integer.value };
        case NODE_TYPE_OPERATION:
            return (questionable_return_t) { .direct = code_operate(&(expression->contents.operation)) };
        default:
            assert(false);
            return (questionable_return_t) { .direct = -1};
    }
}

static symbol_table_index_t code_operate(const parse_node_operation_t *operation) {
    static int jump_target_num = 0;

    switch(operation->operr) {
        case OP_NOOP:
            return -1;
        case OP_ADD2:
        case OP_SUB2:
            assert(operation->num_ops == 2);
            if (operation->ops[0]->type == NODE_TYPE_INT &&
                operation->ops[1]->type == NODE_TYPE_INT) {
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
            else if (operation->ops[0]->type == NODE_TYPE_INT) {
                symbol_table_index_t dindex = code_gen_rec(operation->ops[1]).indirect;
                symbol_table_index_t sindex = symbol_add();
                const char * symbol_text[2];
                if( ! symbol_give_me_my_stuff(2, symbol_text, sindex, dindex) ) {
                    // TODO: Don't die.
                    assert(false);
                }
                printf("\tmovl $%d, %s\n", code_gen_rec(operation->ops[0]).direct, symbol_text[0]);
                printf("\t%s %s, %s\n", code_gen_op_to_mnem(operation->operr), symbol_text[1], symbol_text[0]);
                symbol_del(dindex);

                return sindex;
            }
            else if (operation->ops[1]->type == NODE_TYPE_INT) {
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
                symbol_del(sindex);

                return dindex;
            }

            // TODO: remove after cases above are complete (they should all return).
            return -1;
        case OP_EQUL:
        case OP_NEQL:
        case OP_LESS:
        case OP_GREA:
            assert(operation->num_ops == 2);
            if(operation->ops[0]->type == NODE_TYPE_INT && operation->ops[1]->type == NODE_TYPE_INT) { // both direct
                symbol_table_index_t dindex = symbol_add();
                const char * ret_symbol_text[1];
                if( ! symbol_give_me_my_stuff(1, ret_symbol_text, dindex) ) {
                    // TODO: Die die die
                    assert(false);
                }
                if(code_gen_rec(operation->ops[0]).direct == code_gen_rec(operation->ops[1]).direct) {
                    printf("\tmovl $1, %s\n", ret_symbol_text[0]);
                }
                else {
                    printf("\tmovl $0, %s\n", ret_symbol_text[0]);
                }
                return dindex;
            }
            else if(operation->ops[0]->type != NODE_TYPE_INT && operation->ops[1]->type != NODE_TYPE_INT) { // both indirect
                symbol_table_index_t lindex = code_gen_rec(operation->ops[0]).indirect;
                symbol_table_index_t rindex = code_gen_rec(operation->ops[1]).indirect;
                const char * symbol_text[2];
                if( ! symbol_give_me_my_stuff(2, symbol_text, lindex, rindex) ) {
                    // TODO: Die if you must
                    assert(false);
                }
                printf("\tcmpl %s, %s\n", symbol_text[0], symbol_text[1]);
                symbol_del(rindex);
                symbol_del(lindex);
            }
            else { // one direct one indirect
                assert(false);
            }
            // Get here because it's in a second instruction
            symbol_table_index_t dindex = symbol_add();
            const char * ret_symbol_text[1];
            if( ! symbol_give_me_my_stuff(1, ret_symbol_text, dindex) ) {
                // TODO: Die die die
                assert(false);
            }
            printf("\t%s cmp_ll%d\n", code_gen_op_to_mnem(operation->operr), jump_target_num);
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

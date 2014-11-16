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
    code_gen_rec(expression);
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
                // TODO: ME
            }
            else if (operation->ops[1]->type == NODE_TYPE_INT) {
                // TODO: ME
            }
            else {
                // TODO: ME. Or else.
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

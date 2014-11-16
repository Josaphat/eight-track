#include "et_compiler.h"
#include "parse_tree.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static const char *const SCRATCH_REGS[] = {"%eax", "%ecx", "%edx", "%esi", "%edi"};
static const size_t SCRATCH_REGS_LEN = sizeof SCRATCH_REGS / sizeof(*SCRATCH_REGS);

static size_t regs_used = 0;

// Funky
static size_t code_gen_rec(const parse_node_t *expression);
static size_t code_operate(const parse_node_operation_t *operation);

void code_gen(const parse_node_t *expression) {
    code_gen_rec(expression);
    regs_used = 0; // TODO: When we add variables, this caused the problem
}

static size_t code_gen_rec(const parse_node_t *expression) {
    assert(expression);

    switch(expression->type) {
        case NODE_TYPE_INT:
            assert(regs_used < SCRATCH_REGS_LEN);
            size_t return_reg = regs_used++;
            printf("\tmovl $%d, %s\n", expression->contents.integer.value, SCRATCH_REGS[return_reg]);
            return return_reg;
        case NODE_TYPE_OPERATION:
            return code_operate(&(expression->contents.operation));
        default:
            assert(false);
            return -1;
    }
}

static size_t code_operate(const parse_node_operation_t *operation) {
    switch(operation->operr) {
        case OP_NOOP:
            return -1;
        case OP_ADD2:
            assert(operation->num_ops == 2);
            assert(regs_used < SCRATCH_REGS_LEN);
            size_t return_reg = regs_used++; 
            size_t node0_reg = code_gen_rec(operation->ops[0]);
            size_t node1_reg = code_gen_rec(operation->ops[1]);
            printf("\tleal (%s, %s), %s\n", SCRATCH_REGS[node0_reg], SCRATCH_REGS[node1_reg], SCRATCH_REGS[return_reg]);
            regs_used -= 2;
            return return_reg;
        default:
            assert(false);
            return -1;
    }
}

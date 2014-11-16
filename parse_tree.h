#ifndef PARSE_TREE_H_
#define PARSE_TREE_H_

#include <stddef.h>

typedef enum {
    NODE_TYPE_INT,
    NODE_TYPE_OPERATION,
} parse_node_tag_t;

typedef enum {
    OP_NOOP = 0,
    OP_ADD2,
    OP_SUB2,
    OP_EQUL,
    OP_NEQL,
    OP_LESS,
    OP_GREA,
    OP_LINT,
} parse_node_operator_t;

typedef struct parse_node parse_node_t;

typedef struct {
    int value;
} parse_node_int_t;

typedef struct {
    parse_node_operator_t operr;
    size_t num_ops;
    const parse_node_t **ops;
} parse_node_operation_t;

typedef union {
    parse_node_int_t integer;
    parse_node_operation_t operation;
} parse_node_contents_t;

struct parse_node {
    parse_node_tag_t type;
    parse_node_contents_t contents;
};

const parse_node_t *parse_node_int(int value);
const parse_node_t *parse_node_operation(parse_node_operator_t operr, size_t num_ops, const parse_node_t *node0, ...);

#endif

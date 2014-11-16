%code requires {
    #include "parse_tree.h"
}

%{
    #include "et_compiler.h"

    #include <stdarg.h>
    #include <stdio.h>
    #include <stdlib.h>

    void yyerror(char *s);
    int yylex(void);
%}

%union {
    int iValue;
    const parse_node_t *oValue;
}

%token <iValue> INTEGER

%type <oValue> expr

%left '+' '-'

%%

input:
     %empty
     | input line
     ;

line:
    '\n'
    | expr '\n'     { code_gen($1); }
    ;

expr:
    INTEGER         { $$ = parse_node_int($1); }
    | expr '+' expr { $$ = parse_node_operation(OP_ADD2, 2, $1, $3); }
    ;

%%

const parse_node_t *parse_node_int(int value) {
    parse_node_t *node = malloc(sizeof(parse_node_t));
    if(node == NULL) {
        yyerror("Out of memory");
    }
    node->type = NODE_TYPE_INT;
    node->contents.integer.value = value;
    return node;
}

const parse_node_t *parse_node_operation(parse_node_operator_t operr, size_t num_ops, const parse_node_t *node0, ...) {
    parse_node_t *node = malloc(sizeof(parse_node_t));
    if(node == NULL) {
        yyerror("Out of memory");
    }
    node->type = NODE_TYPE_OPERATION;
    node->contents.operation.operr = operr;
    node->contents.operation.num_ops = num_ops;
    node->contents.operation.ops = malloc(num_ops * sizeof(const parse_node_t *));
    if(node->contents.operation.ops == NULL) {
        yyerror("Out of memory");
    }
    va_list list;
    va_start(list, node0);
    const parse_node_t *curr_node = node0;
    node->contents.operation.ops[0] = curr_node;
    for(unsigned idx = 1; idx < num_ops; ++idx) {
        curr_node = va_arg(list, const parse_node_t *);
        node->contents.operation.ops[idx] = curr_node;
    }
    va_end(list);
    return node;
}

void yyerror(char *s) {
    fprintf(stderr, "ERROR: %s\n", s);
}

int main(void)
{
    yyparse();
    return 0;
}

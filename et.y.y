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
%token EQUAL NEQUAL
%token LINT_START LINT_END
%token BADLEX

%type <oValue> expr logic

%left '+' '-' EQUAL NEQUAL

%%

input:
     %empty
     | input line
     ;

line:
    '\n'
    | BADLEX '\n'   { YYABORT; }
    | expr '\n'     { code_gen($1); }
    ;

logic:
    expr EQUAL expr     {
                      $$ = parse_node_operation(OP_EQ,  2, $1, $3);
                        }
    | expr NEQUAL expr  {
                      $$ = parse_node_operation(OP_NEQ, 2, $1, $3);
                        }
    | expr '<' expr  { $$ = parse_node_operation(OP_LESS, 2, $1, $3); }
    | expr '>' expr  { $$ = parse_node_operation(OP_GREA, 2, $1, $3); }
    ;


expr:
    INTEGER         { $$ = parse_node_int($1); }
    | LINT_START logic LINT_END {
                                    $$ = parse_node_operation(OP_LINT, 1, $2);
                                }
    | expr '+' expr { $$ = parse_node_operation(OP_ADD2, 2, $1, $3); }
    | expr '-' expr { $$ = parse_node_operation(OP_SUB2, 2, $1, $3); }
    | '(' expr ')'  { $$ = $2; }
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
    return yyparse();
}

%{
    #include <stdio.h>
%}

%token INTEGER

%left '+' '-'

%%

input:
     %empty
     | input line
     ;

line:
    '\n'
    | expr '\n' { printf ("\t%.10g\n", $1); }

expr:
    INTEGER         { $$ = $1; }
    | expr '+' expr { $$ = $1 + $3; }
    | expr '-' expr { $$ = $1 - $3; }
    | '(' expr ')'  { $$ = $2; }
    ;

%%

int main(void)
{
    yyparse();
    return 0;
}

%{
    #include <stdio.h>
    void yyerror(char *s);
%}

%union {
    int iValue;
};

%token <iValue> INTEGER

%type <iValue> expr

%left '+' '-'

%%

input:
     %empty
     | input line
     ;

line:
    '\n'
    | expr '\n' { printf ("\t%d\n", $1); }
    ;

expr:
    INTEGER         { $$ = $1; }
    | expr '+' expr { $$ = $1 + $3; }
    | expr '-' expr { $$ = $1 - $3; }
    | '(' expr ')'  { $$ = $2; }
    ;

%%

void yyerror(char *s) {
    fprintf(stderr, "ERROR: %s\n", s);
}

int main(void)
{
    yyparse();
    return 0;
}

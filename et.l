%{
    #include "y.tab.h"
    void yyerror(char *);
%}

INTEGER 0|[1-9][0-9]*

%%

INTEGER     {
                yylval.iValue = atoi(yytext);
                return INTEGER;
            }

[+-]        {
                return *yytext;
            }

%%

int yywrap(void) {
    return 1;
}

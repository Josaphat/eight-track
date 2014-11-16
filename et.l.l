%{
    #include "et.y.h"

    void yyerror(char *);
%}

%%

0           {
                yylval.iValue = atoi(yytext);
                return INTEGER;
            }

[1-9][0-9]* {
                yylval.iValue = atoi(yytext);
                return INTEGER;
            }

"int"       {
                return *yytext;
            }

[:alpha:][:alnum:]* {
                yylval.sValue = yytext;
                return VARIABLE;
                    }

[+\-()=]     {
                return *yytext;
            }

[\n]        {
                return *yytext;
            }

[ \t]       // Ignored

.           {
                yyerror("Unrecognized symbol");
                return BADLEX;
            }

%%

int yywrap(void) {
    return 1;
}

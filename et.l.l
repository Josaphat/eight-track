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

[+]      {
                return *yytext;
            }

[\n]        {
                return *yytext;
            }

.           {
                yyerror("Unrecognized symbol");
                return BADLEX;
            }

%%

int yywrap(void) {
    return 1;
}

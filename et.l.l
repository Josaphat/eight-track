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

int       {
                return INTKEYWORD;
          }

[a-zA-Z][a-zA-Z0-9]* {
                char *newbuf = malloc(yyleng + 1);
                strcpy(newbuf, yytext);
                yylval.sValue = newbuf;
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

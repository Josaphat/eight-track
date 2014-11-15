%{
    #include "y.tab.h"
%}

INTEGER 0|[1-9][0-9]*

%%

INTEGER     {
                yylval.iValue = atoi(yytext);
                return INTEGER;
            }

[+-]        {
                return *yyxtext;
            }

/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;
bool is_contain_null;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */

%}

%option noyywrap

%x IN_COMMENT
%x IN_STR

/*
 * Define names for regular expressions here.
 */

DIGITS          [0-9]+
TPID            [A-Z][A-Za-z0-9_]*
OBJID           [a-z][A-Za-z0-9_]*

%%

 /*
  *  Nested comments
  */

"(*"    BEGIN(IN_COMMENT);

<IN_COMMENT>{
  "*)"        BEGIN(INITIAL);
  [^*\n]+     { /*skip comments*/ }
  "*"         { /*skip comments*/ }
  "\n"        { curr_lineno++; }
}
"--".*        { /*skip comments*/ }

"*)" {
  yylval.error_msg = "Unmatched *)";
  return ERROR;
}

 /*
  *  The multiple-character operators.
  */
"=>"    { return DARROW; }
"<-"    { return ASSIGN; }
"<="    { return LE;     }

 /*
  *  The single-characters
  */

"+"   { return '+'; }
"-"   { return '-'; }
"*"   { return '*'; }
"/"   { return '/'; }

"="   { return '='; }
"<"   { return '<'; }
">"   { return '>'; }
"~"   { return '~'; }

"\."  { return '.'; }
"\("  { return '('; }
"\)"  { return ')'; }
"\{"  { return '{'; }
"\}"  { return '}'; }
"\["  { return '['; }
"\]"  { return ']'; }
":"   { return ':'; }
";"   { return ';'; }
","   { return ','; }

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */
[Cc][Ll][Aa][Ss][Ss]              { return CLASS;    }
[Ee][Ll][Ss][Ee]                  { return ELSE;     }
[Ii][Ff]                          { return IF;       }
[Ff][Ii]                          { return FI;       }
[Ii][Nn]                          { return IN;       }
[Ii][Nn][Hh][Ee][Rr][Ii][Tt][Ss]  { return INHERITS; }
[Ii][Ss][Vv][Oo][Ii][Dd]          { return ISVOID;   }
[Ll][Ee][Tt]                      { return LET;      }
[Ll][Oo][Oo][Pp]                  { return LOOP;     }
[Pp][Oo][Oo][Ll]                  { return POOL;     }
[Tt][Hh][Ee][Nn]                  { return THEN;     }
[Ww][Hh][Ii][Ll][Ee]              { return WHILE;    }
[Cc][Aa][Ss][Ee]                  { return CASE;     }
[Ee][Ss][Aa][Cc]                  { return ESAC;     }
[Nn][Ee][Ww]                      { return NEW;      }
[Oo][Ff]                          { return OF;       }
[Nn][Oo][Tt]                      { return NOT;      }

 /*
  * Bool constant
  */

"t"[Rr][Uu][Ee] {
  yylval.boolean = 1;
  return BOOL_CONST;
}
"f"[Aa][Ll][Ss][Ee] { 
  yylval.boolean = 0;
  return BOOL_CONST; 
}

 /*
  *  Identifier
  */
{TPID} {
  yylval.symbol = idtable.add_string(yytext);
  return TYPEID;
}

{OBJID} {
  yylval.symbol = idtable.add_string(yytext);
  return OBJECTID;
}

 /*
  * Integer constant
  */
{DIGITS} {
  yylval.symbol = inttable.add_string(yytext);
  return INT_CONST;
}

 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */

\" {
  string_buf_ptr = string_buf;
  is_contain_null = false;
  BEGIN(IN_STR);
}

<IN_STR>{
  \" {
    BEGIN(INITIAL);

    *string_buf_ptr = '\0';
    if(strlen(string_buf) > MAX_STR_CONST) {
      yylval.error_msg = "String constant too long!";
      return ERROR;
    }
    else if(is_contain_null) {
      yylval.error_msg = "String contains null character!";
      return ERROR;
    }
    else {
      yylval.symbol = stringtable.add_string(string_buf, MAX_STR_CONST);
      return STR_CONST;
    }
  }

  "\n" {
    curr_lineno++;
    yylval.error_msg = "Unterminated string constant!";

    return ERROR;
  }
  "\0"   is_contain_null = true;

  "\\n"  *string_buf_ptr++ = '\n';
  "\\t"  *string_buf_ptr++ = '\t';
  "\\r"  *string_buf_ptr++ = '\r';
  "\\b"  *string_buf_ptr++ = '\b';
  "\\f"  *string_buf_ptr++ = '\f';

  "\\".  *string_buf_ptr++ = yytext[1];
  "\\\n"  curr_lineno++;
  [^\\\n\"]+ {
    char * yptr = yytext;

    while(*yptr)  *string_buf_ptr++ = *yptr++;
  }
}

 /*
  *  Whitespace and newline
  */
[ \r\t\f\v]  { /*skip whitespace*/ }
"\n"         { curr_lineno++;      }

 /*
  *  Didn't match anything
  */
. {
  yylval.error_msg = yytext;
  return ERROR;
}

 /*
  *  EOF error
  */
<IN_COMMENT><<EOF>> {
  BEGIN(INITIAL);
  yylval.error_msg = "EOF in comment!";
  return ERROR;
}

<IN_STR><<EOF>> {
  BEGIN(INITIAL);
  yylval.error_msg = "EOF in quote!";
  return ERROR;
}

%%

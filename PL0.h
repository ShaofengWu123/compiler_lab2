#include <stdio.h>

#define NRW        12     // number of reserved words, add print
#define TXMAX      500    // length of identifier table
#define MAXNUMLEN  14     // maximum number of digits in numbers
#define NSYM       14     // maximum number of symbols in array ssym and csym, add '[' and ']',add '{' and '}'
#define MAXIDLEN   10     // length of identifiers

#define MAXADDRESS 32767  // maximum address
#define MAXLEVEL   32     // maximum depth of nesting block
#define CXMAX      500    // size of code array

#define MAXSYM     30     // maximum number of symbols  

#define STACKSIZE  1000   // maximum storage

#define MAX_DIM    100    // maximum dimensions an array can have 

enum symtype
{
	SYM_NULL,
	SYM_IDENTIFIER,
	SYM_NUMBER,
	SYM_PLUS,
	SYM_MINUS,
	SYM_TIMES,
	SYM_SLASH,
	SYM_ODD,
	SYM_EQU,
	SYM_NEQ,
	SYM_LES,
	SYM_LEQ,
	SYM_GTR,
	SYM_GEQ,
	SYM_LPAREN,
	SYM_RPAREN,
	SYM_COMMA,
	SYM_SEMICOLON,
	SYM_PERIOD,
	SYM_BECOMES,
	SYM_BEGIN,
	SYM_END,
	SYM_IF,
	SYM_THEN,
	SYM_WHILE,
	SYM_DO,
	SYM_CALL,
	SYM_CONST,
	SYM_VAR,
	SYM_PROCEDURE,
	SYM_LSQUAREBRACKET,
	SYM_RSQUAREBRACKET,
	SYM_PRINT,
	SYM_LBRACKET,
	SYM_RBRACKET
};

enum idtype
{
	ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE, ID_ARRAY
};


// added following opcode
// PRT: print the stack top element
//
enum opcode
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JPC, PRT, STA, LEA, LDA, STI, RES, RET
};

enum oprcode
{
	OPR_RET, OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ
};


typedef struct instruction
{
	int f; // function code
	int l; // level
	int a; // displacement address
} instruction;

//////////////////////////////////////////////////////////////////////
char* err_msg[] =
{
	/*  0 */    "",
	/*  1 */    "Found ':=' when expecting '='.",
	/*  2 */    "There must be a number to follow '='.",
	/*  3 */    "There must be an '=' to follow the identifier.",
	/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
	/*  5 */    "Missing ',' or ';'.",
	/*  6 */    "Incorrect procedure name.",
	/*  7 */    "Statement expected.",
	/*  8 */    "Follow the statement is an incorrect symbol.",
	/*  9 */    "'.' expected.",
	/* 10 */    "';' expected.",
	/* 11 */    "Undeclared identifier.",
	/* 12 */    "Illegal assignment.",
	/* 13 */    "':=' expected.",
	/* 14 */    "There must be an identifier to follow the 'call'.",
	/* 15 */    "A constant or variable can not be called.",
	/* 16 */    "'then' expected.",
	/* 17 */    "';' or 'end' expected.",
	/* 18 */    "'do' expected.",
	/* 19 */    "Incorrect symbol.",
	/* 20 */    "Relative operators expected.",
	/* 21 */    "Procedure identifier can not be in an expression.",
	/* 22 */    "Missing ')'.",
	/* 23 */    "The symbol can not be followed by a factor.",
	/* 24 */    "The symbol can not be as the beginning of an expression.",
	/* 25 */    "The number is too great.",
	/* 26 */    "Must define the dimension size of an array.",
	/* 27 */    "Missing ']'.",
	/* 28 */    "Function print should be used in print(...) format.",
	/* 29 */    "Wrong argument type for print",
	/* 30 */    "Only constant identifier is allowed for array declaration.",
	/* 31 */    "Too many dimensions for array declaration.",
	/* 32 */    "There are too many levels.",
	/* 33 */    "Array dimension can not be zero or negative.",
	/* 34 */    "Missing dimension(s) in array element assignment.",
	/* 35 */    "Missing '['.",
	/* 36 */    "Missing '{'.",
	/* 37 */    "Illegal initialization.",
	/* 38 */    "Missing '}'."
	/* 39 */	"The empty dimension must be the first one."
};

//////////////////////////////////////////////////////////////////////
char ch;         // last character read
int  sym;        // last symbol read
char id[MAXIDLEN + 1]; // last identifier read
int  num;        // last number read
struct _array_info* pa; //pointer to last array information read  
int  cc;         // character count
int  ll;         // line length
int  kk;
int  err;
int  cx;         // index of current instruction to be generated.
int  level = 0;
int  tx = 0;     // current number of symbols in the symbol table
int  arr_tx = 0; // current number of symbols in the array information table

char line[80];

instruction code[CXMAX];


// Add following reserved word
// print

char* word[NRW + 1] =
{
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while", "print"
};

int wsym[NRW + 1] =
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE, SYM_PRINT
};


int ssym[NSYM + 1] =
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON,SYM_LSQUAREBRACKET,SYM_RSQUAREBRACKET,
	SYM_LBRACKET,SYM_RBRACKET
};

char csym[NSYM + 1] =
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';','[',']','{','}'
};

#define MAXINS   15 //added PRT,STA,LEA,STI
char* mnemonic[MAXINS] =
{
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC", "PRT", "STA", "LEA", "LDA","STI","RES","RET"
};

//define the structure that stores array information
typedef struct _array_info
{
	short address;
	int size;
	int dim;//total dimension 
	int dim_size[MAX_DIM + 1];
} array_info;

//define array information table
array_info array_table[TXMAX];

typedef struct comtab
{
	char name[MAXIDLEN + 1];
	int  kind;
	int  value;
} comtab;

comtab table[TXMAX];//symbol table

// if kind is array, then address stores the index of the array in another array table

typedef struct mask
{
	char  name[MAXIDLEN + 1];
	int   kind;
	short level;
	short address;
} mask;

FILE* infile;

// EOF PL0.h

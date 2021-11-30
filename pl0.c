// pl0 compiler source code

#pragma warning(disable:4996)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "PL0.h"
#include "set.c"


void array_visit(short arr_index, int dim, symset fsys);

//////////////////////////////////////////////////////////////////////
// print error message.
void error(int n)
{
	int i;

	printf("      ");
	for (i = 1; i <= cc - 1; i++)
		printf(" ");
	printf("^\n");
	printf("Error %3d: %s\n", n, err_msg[n]);
	err++;
} // error

//////////////////////////////////////////////////////////////////////
void getch(void)
{
	if (cc == ll)
	{
		if (feof(infile))
		{
			printf("\nPROGRAM INCOMPLETE\n");
			exit(1);
		}
		ll = cc = 0;
		printf("%5d  ", cx);
		while ((!feof(infile)) // added & modified by alex 01-02-09
			&& ((ch = getc(infile)) != '\n'))
		{
			printf("%c", ch);
			line[++ll] = ch;
		} // while
		printf("\n");
		line[++ll] = ' ';
	}
	ch = line[++cc];
} // getch

//////////////////////////////////////////////////////////////////////
// gets a symbol from input stream.
void getsym(void)
{
	int i, k;
	char a[MAXIDLEN + 1];

	while (ch == ' ' || ch == '\t')
		getch();

	if (isalpha(ch))
	{ // symbol is a reserved word or an identifier.
		k = 0;
		do
		{
			if (k < MAXIDLEN)
				a[k++] = ch;
			getch();
		} while (isalpha(ch) || isdigit(ch));
		a[k] = 0;
		strcpy(id, a);//strcpt(dest,src)
		word[0] = id;
		i = NRW;
		while (strcmp(id, word[i--]));//serching in the reserved word array word[]
		if (++i)
			sym = wsym[i]; // symbol is a reserved word
		else
			sym = SYM_IDENTIFIER;   // symbol is an identifier
	}
	else if (isdigit(ch))
	{ // symbol is a number.
		k = num = 0;
		sym = SYM_NUMBER;
		do
		{
			num = num * 10 + ch - '0';//set number's value
			k++;
			getch();
		} while (isdigit(ch));
		if (k > MAXNUMLEN)
			error(25);     // The number is too great.
	}
	else if (ch == ':')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_BECOMES; // :=
			getch();
		}
		else
		{
			sym = SYM_NULL;       // illegal?
		}
	}
	else if (ch == '>')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_GEQ;     // >=
			getch();
		}
		else
		{
			sym = SYM_GTR;     // >
		}
	}
	else if (ch == '<')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_LEQ;     // <=
			getch();
		}
		else if (ch == '>')
		{
			sym = SYM_NEQ;     // <>
			getch();
		}
		else
		{
			sym = SYM_LES;     // <
		}
	}
	else
	{ // other tokens, these tokens are specified in head file, token in csym and token name in ssym. New token should be added into csym and ssym.
		i = NSYM;
		csym[0] = ch;
		while (csym[i--] != ch);
		if (++i)
		{
			sym = ssym[i];
			getch();
		}
		else
		{
			printf("Fatal Error: Unknown character.\n");
			exit(1);
		}
	}
} // getsym

//////////////////////////////////////////////////////////////////////
// generates (assembles) an instruction.
void gen(int x, int y, int z)
{
	if (cx > CXMAX)
	{
		printf("Fatal Error: Program too long.\n");
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx++].a = z;
} // gen

//////////////////////////////////////////////////////////////////////
// tests if error occurs and skips all symbols that do not belongs to s1 or s2.
void test(symset s1, symset s2, int n)
{
	symset s;

	if (!inset(sym, s1))
	{
		error(n);
		s = uniteset(s1, s2);
		while (!inset(sym, s))
			getsym();
		destroyset(s);
	}
} // test

//////////////////////////////////////////////////////////////////////
int dx;  // data allocation index

// enter object(constant, variable or procedure) into table.
void enter(int kind)
{
	mask* mk;

	tx++;
	strcpy(table[tx].name, id);
	table[tx].kind = kind;
	switch (kind)
	{
	case ID_CONSTANT:
		if (num > MAXADDRESS)
		{
			error(25); // The number is too great.
			num = 0;
		}
		table[tx].value = num;
		break;
	case ID_VARIABLE:
		mk = (mask*)&table[tx];
		mk->level = level;
		printf("allocate for variable: %d\n", dx);
		mk->address = dx++;//allocate store place for the variable 
		break;
	case ID_PROCEDURE:
		mk = (mask*)&table[tx];
		mk->level = level;
		break;
	case ID_ARRAY:
		mk = (mask*)&table[tx];
		mk->level = level;
		mk->address = arr_tx;//// add new entry in symble table and array table
		pa = &(array_table[arr_tx]);
		array_table[arr_tx].dim = 0;
		array_table[arr_tx].size = 1;
		//array_table[addr_tx].dim_size[0] = 1;
		array_table[arr_tx].dim_size[1] = 0; // this place is set to zero in order to judge if first dimension is set by following analysis.
		arr_tx++;
		//dx has to wait until all dimensions analyzed
		break;
	} // switch
} // enter

//////////////////////////////////////////////////////////////////////
// locates identifier in symbol table.
int position(char* id)
{
	int i;
	strcpy(table[0].name, id);
	i = tx + 1;
	while (strcmp(table[--i].name, id) != 0);
	return i;
} // position

//////////////////////////////////////////////////////////////////////
void constdeclaration()
{
	if (sym == SYM_IDENTIFIER)
	{
		getsym();
		if (sym == SYM_EQU || sym == SYM_BECOMES)
		{
			if (sym == SYM_BECOMES)
				error(1); // Found ':=' when expecting '='.
			getsym();
			if (sym == SYM_NUMBER)
			{
				enter(ID_CONSTANT);
				getsym();
			}
			else
			{
				error(2); // There must be a number to follow '='.
			}
		}
		else
		{
			error(3); // There must be an '=' to follow the identifier.
		}
	}
	else	error(4);
	// There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

//////////////////////////////////////////////////////////////////////
// dimdeclaration() added by Shaofeng Wu
// function empty first dimension added by Chen Wang
// note: array declaration must have and can only have SYM_NUMBER,SYM_IDENTIFIER in each dimension
void dimdeclaration() {
	int i;
	if (sym == SYM_LSQUAREBRACKET) {
		getsym();
		switch (sym)
		{

		case SYM_NUMBER:// number as dim size
			// store dimension size and increase dimension number
			if (pa->dim == MAX_DIM) { error(31); }//too many dimensions
			if (num <= 0) { error(33); }//dimension size can not be negative or zero
			pa->dim = pa->dim + 1;
			pa->dim_size[pa->dim] = num;
			pa->size = pa->size * num;
			getsym();
			if (sym == SYM_RSQUAREBRACKET) { getsym();dimdeclaration(); }
			else { error(27); }//missing 
			break;
		case SYM_IDENTIFIER:
			//it must be defined-const, variable is not allowed in declaration
			if ((i = position(id)) == 0)
			{
				error(11); // Undeclared identifier.
			}
			if (table[i].kind != ID_CONSTANT) { error(30); }
			// store dimension size and increase dimension number
			if (pa->dim == MAX_DIM) { error(31); }//too many dimensions
			if (table[i].value <= 0) { error(33); }//dimension size can not be negative or zero
			pa->dim = pa->dim + 1;
			pa->dim_size[pa->dim] = table[i].value;
			pa->size = pa->size * table[i].value;
			getsym();
			if (sym == SYM_RSQUAREBRACKET) { getsym();dimdeclaration(); }
			else { error(27); }//missing 
			break;
            case SYM_RSQUAREBRACKET:
                //a[][5]={1,2,3}
                //first dimension is empty, added by Chen Wang,30/11/2021
                //And if the first dimension is empty, use initilization to fill in that dimension
                if(pa->dim!=0){
                    error(39);
                }//the empty dimension must be the first one
                pa->dim = pa->dim + 1;
                pa->dim_size[pa->dim] = 0;
                pa->size = pa->size * 1;
                getsym();
                dimdeclaration();
                break;
		default:
			error(26);//missing array size, TO BE DONE: first dimension missing
			break;
		}
	}
	else {
		//analyzed dimensions, allocate space if first dimension is clear 
		if (pa->dim_size[1]) {//first dimension clear, space can be allocated
			//printf("allocate for array: %d\n",dx);
			dx += pa->size;
			pa->address = dx - 1;
			printf("allocate for array: %d\n", dx - 1);
		}
		else {
            //first dim empty,look forward to fill the first dim
            int savecc=cc;
            char savech=ch;
            int savesym=sym;
            int cntnumber=0;
            int cntdimsize=0;
            //read until id or  ";"
            getsym();
            while(sym!=SYM_SEMICOLON && sym!=SYM_IDENTIFIER ){
                if(sym==SYM_NUMBER)
                    cntnumber++;
                getsym();
            }
            pa->size = cntnumber;
            for(int i=1;i<=pa->dim;i++){
                cntdimsize+=pa->dim_size[i];
            }
            pa->dim_size[1] = pa->size/cntdimsize;
            dx += pa->size;
            pa->address = dx - 1;
            printf("allocate for array: %d\n", dx - 1);
            printf("total dim %d,first dim size %d\n", pa->dim,pa->dim_size[1]);
            cc = savecc;
            ch = savech;
            sym = savesym;
		}
	}
}

int current_level, max_level, array_index, array_dim;
int current_array[MAX_DIM + 1], max_array[MAX_DIM + 1];
int array_full,before_Lbracket;
void initializer(void);

void initializer_list1(void)
{
	if (sym == SYM_COMMA) {
		getsym();
		initializer();
		getsym();
		initializer_list1();
	}
}

void initializer_list(void)
{
	if (sym == SYM_NUMBER || sym == SYM_LBRACKET) {
		initializer();
		getsym();
		initializer_list1();
	}
}

void initializer(void)
{
	if (sym == SYM_NUMBER) {
		int k;
		before_Lbracket = 0;
		for (k = array_dim;k > 0;k--) {
			if (current_array[k] < max_array[k]) {
				current_array[k]++;
				break;
			}
			if (current_array[k] == max_array[k]) {
				current_array[k] = 0;
			}
		}
		if (k == 0) {
			error(37);
			return;
		}
		//mask* mk;
		//mk = (mask*)&table[tx];
		gen(LIT, 0, array_index++);
		gen(LIT, 0, num);
		gen(STI, 0, 0);
	}
	else if (sym == SYM_LBRACKET) {
		current_level++;
		before_Lbracket = 1;
		if (current_level > max_level) {
			error(37);
			return;
		}
		getsym();
		initializer_list();
		if (sym == SYM_COMMA) {
			getsym();
		}
		if (sym == SYM_RBRACKET) {
			if (current_level == max_level) {
				if (before_Lbracket) {
					gen(LIT, 0, array_index++);
					gen(LIT, 0, 0);
					gen(STI, 0, 0);
				}
				current_level--;
			}
			else {
				int k, offset_count, array_index_after = 0;
				for (k = array_dim;k > current_level;k--) {
					current_array[k] = max_array[k];
				}
				for (int m = 1;m < array_dim + 1;m++) {
					array_index_after = array_index_after * (max_array[m] + 1) + current_array[m];
				}
				array_index_after++;
				offset_count = array_index_after - array_index;
				for (int offset = 0;offset < offset_count;offset++) {
					gen(LIT, 0, array_index + offset);
					gen(LIT, 0, 0);
					gen(STI, 0, 0);
				}
				array_index = array_index_after;
				current_level--;
			}
			return;
		}
		else
			error(38);
	}
	else
		error(36);
}

void vardeclaration(void)
{
	if (sym == SYM_IDENTIFIER)
	{
		getsym();
		if (sym == SYM_LSQUAREBRACKET) {
			enter(ID_ARRAY);//in enter(), array information space is allocated and members are initialized 
			//array_info * temp = (array_info *)malloc(sizeof(array_info));
			//temp -> dim = 0;temp->size = 0;
			dimdeclaration();
			if (sym == SYM_BECOMES || sym == SYM_EQU) {
				getsym();
				if (sym == SYM_LBRACKET) {
					current_level = -1;
					array_index = 0;
					array_dim = pa->dim;
					max_level = pa->dim;
					array_full = 0;
					for (int k = 1;k < array_dim + 1;k++) {
						current_array[k] = 0;
						max_array[k] = pa->dim_size[k] - 1;
					}
					current_array[array_dim] = -1;
					mask* mk;
					mk = (mask*)&table[tx];
					gen(RES, 0, 0);
					gen(LEA, level - mk->level, pa->address);
					initializer();
					gen(RET, 0, 0);
					getsym();
				}
			}
		}
		else { enter(ID_VARIABLE); }//enter uses the id for adding entry to the symbol table, id is not changed by getsym if sym is not a identifier
	}
	else
	{
		error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
	}
} // vardeclaration



//////////////////////////////////////////////////////////////////////
void listcode(int from, int to)
{
	int i;

	printf("\n");
	for (i = from; i < to; i++)
	{
		printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
	}
	printf("\n");
} // listcode

//////////////////////////////////////////////////////////////////////
void factor(symset fsys)
{
	void expression(symset fsys);
	int i;int arr_index;
	symset set;

	test(facbegsys, fsys, 24); // The symbol can not be as the beginning of an expression.

	if (inset(sym, facbegsys))
	{
		if (sym == SYM_IDENTIFIER)
		{
			if ((i = position(id)) == 0)
			{
				error(11); // Undeclared identifier.
			}
			else
			{
				switch (table[i].kind)
				{
					mask* mk;
				case ID_CONSTANT:
					gen(LIT, 0, table[i].value);
					getsym();
					break;
				case ID_VARIABLE:
					mk = (mask*)&table[i];
					gen(LOD, level - mk->level, mk->address);
					getsym();
					break;
				case ID_PROCEDURE:
					error(21); // Procedure identifier can not be in an expression.
					break;
				case ID_ARRAY: //add array element as factor
					mk = (mask*)&table[i];
					arr_index = mk->address;//index in array table
					mk = (mask*)&table[i];
					gen(LEA, level - mk->level, array_table[arr_index].address);//in the end, start address - offset
					gen(LIT, 0, 0);//add with 0 first 
					set = createset(SYM_RSQUAREBRACKET);
					array_visit(arr_index, 0, set);
					//if(sym!= SYM_RPAREN){error(13);}
					//getsym(); array_visit already get next symbol
					gen(OPR, 0, OPR_MIN);//top-1 - top -> top-1
					gen(LDA, 0, 0);//use LDA to load array element to top 
					break;
				} // switch
			}
			//getsym();
		}
		else if (sym == SYM_NUMBER)
		{
			if (num > MAXADDRESS)
			{
				error(25); // The number is too great.
				num = 0;
			}
			gen(LIT, 0, num);
			getsym();
		}
		else if (sym == SYM_LPAREN)
		{
			getsym();
			set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
			expression(set);
			destroyset(set);
			if (sym == SYM_RPAREN)
			{
				getsym();
			}
			else
			{
				error(22); // Missing ')'.
			}
		}
		else if (sym == SYM_MINUS) // UMINUS,  Expr -> '-' Expr
		{
			getsym();
			factor(fsys);
			gen(OPR, 0, OPR_NEG);
		}
		test(fsys, createset(SYM_LPAREN, SYM_NULL), 23);
	} // if
} // factor

//////////////////////////////////////////////////////////////////////
void term(symset fsys)
{
	int mulop;
	symset set;

	set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_NULL));
	factor(set);
	while (sym == SYM_TIMES || sym == SYM_SLASH)
	{
		mulop = sym;
		getsym();
		factor(set);
		if (mulop == SYM_TIMES)
		{
			gen(OPR, 0, OPR_MUL);
		}
		else
		{
			gen(OPR, 0, OPR_DIV);
		}
	} // while
	destroyset(set);
} // term

//////////////////////////////////////////////////////////////////////
void expression(symset fsys)
{
	int addop;
	symset set;

	set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_NULL));

	term(set);
	while (sym == SYM_PLUS || sym == SYM_MINUS)
	{
		addop = sym;
		getsym();
		term(set);
		if (addop == SYM_PLUS)
		{
			gen(OPR, 0, OPR_ADD);
		}
		else
		{
			gen(OPR, 0, OPR_MIN);
		}
	} // while

	destroyset(set);
} // expression

//////////////////////////////////////////////////////////////////////
void condition(symset fsys)
{
	int relop;
	symset set;

	if (sym == SYM_ODD)
	{
		getsym();
		expression(fsys);
		gen(OPR, 0, 6);
	}
	else
	{
		set = uniteset(relset, fsys);
		expression(set);
		destroyset(set);
		if (!inset(sym, relset))
		{
			error(20);
		}
		else
		{
			relop = sym;
			getsym();
			expression(fsys);
			switch (relop)
			{
			case SYM_EQU:
				gen(OPR, 0, OPR_EQU);
				break;
			case SYM_NEQ:
				gen(OPR, 0, OPR_NEQ);
				break;
			case SYM_LES:
				gen(OPR, 0, OPR_LES);
				break;
			case SYM_GEQ:
				gen(OPR, 0, OPR_GEQ);
				break;
			case SYM_GTR:
				gen(OPR, 0, OPR_GTR);
				break;
			case SYM_LEQ:
				gen(OPR, 0, OPR_LEQ);
				break;
			} // switch
		} // else
	} // else
} // condition


//array_visit
void array_visit(short arr_index, int dim, symset fsys) {//dim means number of dimensions that has been analyzed
	getsym();
	if (sym == SYM_LSQUAREBRACKET) {
		//check brackets
		//if(sym!=SYM_LSQUAREBRACKET){error(35);}//missing '['
		gen(LIT, 0, array_table[arr_index].dim_size[dim + 1]);
		gen(OPR, 0, OPR_MUL);//multiply top two
		getsym();
		expression(fsys);//if not an expression, error will be raised by expression(), ']' check is done in expression()
		//getsym(); expression already get next symbol
		// Note: offset overflow will check by runtime
		gen(OPR, 0, OPR_ADD);//add calculated offset to multiplied number
		array_visit(arr_index, dim + 1, fsys);//visit next dimension 
	}
	else if (dim != array_table[arr_index].dim) { error(34); }//missing dimensions
	//test
}


//////////////////////////////////////////////////////////////////////
void statement(symset fsys)
{
	int i, cx1, cx2;short arr_index;
	symset set1, set;

	int count = 0; //for print

	if (sym == SYM_IDENTIFIER)
	{ // variable assignment, added array element as left value
		mask* mk;
		if (!(i = position(id)))
		{
			error(11); // Undeclared identifier.
		}
		else if (table[i].kind != ID_VARIABLE && table[i].kind != ID_ARRAY)
		{
			error(12); // Illegal assignment.
			i = 0;
		}
		if (table[i].kind == ID_VARIABLE)//variable assignment
		{
			getsym();
			if (sym == SYM_BECOMES)
			{
				getsym();
			}
			else
			{
				error(13); // ':=' expected.
			}

			expression(fsys);
			mk = (mask*)&table[i];
			if (i)
			{
				gen(STO, level - mk->level, mk->address);//change variable's number 
			}
		}
		else if (table[i].kind == ID_ARRAY) {
			//use a recursive function array_visit to read through brackets to calculate address. At the same time, generate neccessary code to calculate expressions between brackets.
			//always use the two value on stack top to calculate accumulated size and store next dimension value
			mk = (mask*)&table[i];
			arr_index = mk->address;//index in array table
			gen(LEA, level - mk->level, array_table[arr_index].address);//in the end, start address - offset
			gen(LIT, 0, 0);//add with 0 first 
			set1 = createset(SYM_RSQUAREBRACKET);
			array_visit(arr_index, 0, set1);//for expression, it can only be followed by ] in this case
			//getsym(); array_visit already get next symbol
			if (sym != SYM_BECOMES) { error(13); }
			gen(OPR, 0, OPR_MIN);//top-1 - top -> top-1
			getsym();
			expression(fsys);//right value, move to top 
			if (i) {//use STA instruction
				gen(STA, 0, 0);//calculated value on stack top, address should be calculated above
			}
		}//array element assignment
	}
	else if (sym == SYM_CALL)
	{ // procedure call
		getsym();
		if (sym != SYM_IDENTIFIER)
		{
			error(14); // There must be an identifier to follow the 'call'.
		}
		else
		{
			if (!(i = position(id)))
			{
				error(11); // Undeclared identifier.
			}
			else if (table[i].kind == ID_PROCEDURE)
			{
				mask* mk;
				mk = (mask*)&table[i];
				gen(CAL, level - mk->level, mk->address);
			}
			else
			{
				error(15); // A constant or variable can not be called. 
			}
			getsym();
		}
	}
	else if (sym == SYM_IF)
	{ // if statement
		getsym();
		set1 = createset(SYM_THEN, SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_THEN)
		{
			getsym();
		}
		else
		{
			error(16); // 'then' expected.
		}
		cx1 = cx;
		gen(JPC, 0, 0);
		statement(fsys);
		code[cx1].a = cx;
	}
	else if (sym == SYM_BEGIN)
	{ // block
		getsym();
		set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
		set = uniteset(set1, fsys);
		statement(set);
		while (sym == SYM_SEMICOLON || inset(sym, statbegsys))
		{
			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(10);
			}
			statement(set);
		} // while
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_END)
		{
			getsym();
		}
		else
		{
			error(17); // ';' or 'end' expected.
		}
	}
	else if (sym == SYM_WHILE)
	{ // while statement
		cx1 = cx;
		getsym();
		set1 = createset(SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);
		destroyset(set1);
		destroyset(set);
		cx2 = cx;
		gen(JPC, 0, 0);
		if (sym == SYM_DO)
		{
			getsym();
		}
		else
		{
			error(18); // 'do' expected.
		}
		statement(fsys);
		gen(JMP, 0, cx1);
		code[cx2].a = cx;
	}
	else if (sym == SYM_PRINT) {
		//print statement
		getsym();
		if (sym != SYM_LPAREN) { error(28); }// wrong format for print
		while (1) {
			getsym();
			if (sym == SYM_RPAREN) { break; }
			else if (sym == SYM_IDENTIFIER) {
				count++;
				if ((i = position(id)) == 0) { error(11); }// Undeclared identifier.
				else if (table[i].kind != ID_VARIABLE && table[i].kind != ID_ARRAY && table[i].kind != ID_CONSTANT)
				{
					error(29);// wrong argument type for print
				}
				else {
					if (table[i].kind == ID_VARIABLE) {//variable
						mask* mk;
						mk = (mask*)&table[i];
						gen(LOD, level - mk->level, mk->address);//load the variable's value onto stack
						getsym();
					}
					else if (table[i].kind == ID_CONSTANT) {
						gen(LIT, 0, table[i].value);//load constant value onto stack
						getsym();
					}
					// array element to be added
					else if (table[i].kind == ID_ARRAY) {
						int arr_index;
						mask* mk;
						mk = (mask*)&table[i];
						arr_index = mk->address;//index in array table
						mk = (mask*)&table[i];
						gen(LEA, level - mk->level, array_table[arr_index].address);//in the end, start address - offset
						gen(LIT, 0, 0);//add with 0 first 
						set1 = createset(SYM_RSQUAREBRACKET);
						array_visit(arr_index, 0, set1);
						//if(sym!= SYM_RPAREN){error(13);}
						//getsym(); array_visit already get next symbol
						gen(OPR, 0, OPR_MIN);//top-1 - top -> top-1
						gen(LDA, 0, 0);//use LDA to load array element to top 
					}
					//getsym();
					if (sym == SYM_COMMA) { ; }
					else if (sym == SYM_RPAREN) { break; }
					else { error(28); }//wrong format
				}
				//destroyset(set1);
			}
			else {
				if (sym == SYM_NUMBER) { error(29); }// wrong argument type for print
				error(28);//wrong format 
			}
		}
		gen(PRT, 0, count); // generate code that print count positions in stack
		getsym();
	}
	test(fsys, phi, 19);
} // statement

//////////////////////////////////////////////////////////////////////
void block(symset fsys)
{
	int cx0; // initial code index
	mask* mk;
	int block_dx;
	int savedTx;
	symset set1, set;

	dx = 3;
	block_dx = dx;
	mk = (mask*)&table[tx];
	mk->address = cx;
	gen(JMP, 0, 0);
	if (level > MAXLEVEL)
	{
		error(32); // There are too many levels.
	}
	do
	{
		if (sym == SYM_CONST)
		{ // constant declarations
			getsym();
			do
			{
				constdeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					constdeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			} while (sym == SYM_IDENTIFIER);
		} // if

		if (sym == SYM_VAR)
		{ // variable declarations
			getsym();
			do
			{
				vardeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					vardeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			} while (sym == SYM_IDENTIFIER);
		} // if
		block_dx = dx; //save dx before handling procedure call!
		while (sym == SYM_PROCEDURE)
		{ // procedure declarations
			getsym();
			if (sym == SYM_IDENTIFIER)
			{
				enter(ID_PROCEDURE);
				getsym();
			}
			else
			{
				error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
			}


			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}

			level++;
			savedTx = tx;
			set1 = createset(SYM_SEMICOLON, SYM_NULL);
			set = uniteset(set1, fsys);
			block(set);
			destroyset(set1);
			destroyset(set);
			tx = savedTx;
			level--;

			if (sym == SYM_SEMICOLON)
			{
				getsym();
				set1 = createset(SYM_IDENTIFIER, SYM_PROCEDURE, SYM_NULL);
				set = uniteset(statbegsys, set1);
				test(set, fsys, 6);
				destroyset(set1);
				destroyset(set);
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}
		} // while
		dx = block_dx; //restore dx after handling procedure call!
		set1 = createset(SYM_IDENTIFIER, SYM_NULL);
		set = uniteset(statbegsys, set1);
		test(set, declbegsys, 7);
		destroyset(set1);
		destroyset(set);
	} while (inset(sym, declbegsys));

	//code[mk->address].a = cx;
	code[mk->address].a = 1;
	mk->address = cx;
	cx0 = cx;
	gen(INT, 0, block_dx);
	set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
	set = uniteset(set1, fsys);

	statement(set);// statement here

	destroyset(set1);
	destroyset(set);
	gen(OPR, 0, OPR_RET); // return
	test(fsys, phi, 8); // test for error: Follow the statement is an incorrect symbol.
	listcode(cx0, cx);
} // block

//////////////////////////////////////////////////////////////////////
// this function goes to the base address of specific AR 
int base(int stack[], int currentLevel, int levelDiff)
{
	int b = currentLevel;

	while (levelDiff--)
		b = stack[b];
	return b;
} // base

//////////////////////////////////////////////////////////////////////
// interprets and executes codes.
void interpret()
{
	int pc;        // program counter
	int stack[STACKSIZE];//initialize a stack
	int top;       // top of stack
	int b;         // program, base, and top-stack register
	instruction i; // instruction register

	printf("Begin executing PL/0 program.\n");

	pc = 0;
	b = 1;
	top = 3;
	stack[1] = stack[2] = stack[3] = 0;
	do
	{
		i = code[pc++];
		switch (i.f)
		{
		case LIT:
			stack[++top] = i.a;
			break;
		case OPR:
			switch (i.a) // operator
			{
			case OPR_RET:
				top = b - 1;
				pc = stack[top + 3];
				b = stack[top + 2];
				break;
			case OPR_NEG:
				stack[top] = -stack[top];
				break;
			case OPR_ADD:
				top--;
				stack[top] += stack[top + 1];
				break;
			case OPR_MIN:
				top--;
				stack[top] -= stack[top + 1];
				break;
			case OPR_MUL:
				top--;
				stack[top] *= stack[top + 1];
				break;
			case OPR_DIV:
				top--;
				if (stack[top + 1] == 0)
				{
					fprintf(stderr, "Runtime Error: Divided by zero.\n");
					fprintf(stderr, "Program terminated.\n");
					continue;
				}
				stack[top] /= stack[top + 1];
				break;
			case OPR_ODD:
				stack[top] %= 2;
				break;
			case OPR_EQU:
				top--;
				stack[top] = stack[top] == stack[top + 1];
				break;
			case OPR_NEQ:
				top--;
				stack[top] = stack[top] != stack[top + 1];
				break;
			case OPR_LES:
				top--;
				stack[top] = stack[top] < stack[top + 1];
				break;
			case OPR_GEQ:
				top--;
				stack[top] = stack[top] >= stack[top + 1];
				break;
			case OPR_GTR:
				top--;
				stack[top] = stack[top] > stack[top + 1];
				break;
			case OPR_LEQ:
				top--;
				stack[top] = stack[top] <= stack[top + 1];
				break;
			} // switch
			break;
		case LEA:
			stack[++top] = base(stack, b, i.l) + i.a;
			break;
		case LOD:
			stack[++top] = stack[base(stack, b, i.l) + i.a];
			break;
		case LDA:
			stack[top] = stack[stack[top]];
			break;
		case STO:
			stack[base(stack, b, i.l) + i.a] = stack[top];//store the number at top to specified address
			//printf("%d\n", stack[top]);
			top--;
			break;
		case STA:
			stack[stack[top - 1]] = stack[top];
			top = top - 2;
			break;
		case STI:
			stack[stack[top - 2] - stack[top - 1]] = stack[top];
			top = top - 2;
			break;
		case CAL:
			stack[top + 1] = base(stack, b, i.l);
			// generate new block mark
			stack[top + 2] = b;
			stack[top + 3] = pc;
			b = top + 1;
			pc = i.a;
			break;
		case INT:
			top += i.a;
			break;
		case RES:
			stack[STACKSIZE - 10] = top;
			top = STACKSIZE - 10;
			break;
		case RET:
			top = stack[top-1];
			break;
		case JMP:
			pc = i.a;
			break;
		case JPC:
			if (stack[top] == 0)
				pc = i.a;
			top--;
			break;
		case PRT:
			if (i.a == 0) { printf("\n"); }
			else {
				for (int k = i.a - 1;k >= 0;k--) {
					printf("%d ", stack[top - k]);//print argument's value one by one
				}
				top = top - i.a;//arguments are collected
			}
			break;
		} // switch
	} while (pc);

	printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
void main()
{
	FILE* hbin;
	char s[80];
	int i;
	symset set, set1, set2;

	printf("Please input source file name: "); // get file name to be compiled
	scanf("%s", s);
	if ((infile = fopen(s, "r")) == NULL)
	{
		printf("File %s can't be opened.\n", s);
		exit(1);
	}

	phi = createset(SYM_NULL);
	relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ, SYM_NULL);

	// create begin symbol sets
	declbegsys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_NULL);
	statbegsys = createset(SYM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE, SYM_NULL);
	facbegsys = createset(SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN, SYM_MINUS, SYM_NULL);

	err = cc = cx = ll = 0; // initialize global variables
	ch = ' ';
	kk = MAXIDLEN;

	getsym();

	set1 = createset(SYM_PERIOD, SYM_NULL);
	set2 = uniteset(declbegsys, statbegsys);
	set = uniteset(set1, set2);
	block(set);
	destroyset(set1);
	destroyset(set2);
	destroyset(set);
	destroyset(phi);
	destroyset(relset);
	destroyset(declbegsys);
	destroyset(statbegsys);
	destroyset(facbegsys);

	if (sym != SYM_PERIOD)
		error(9); // '.' expected.
	if (err == 0)
	{
		hbin = fopen("hbin.txt", "w");
		for (i = 0; i < cx; i++)
			fwrite(&code[i], sizeof(instruction), 1, hbin);
		fclose(hbin);
	}
	if (err == 0)
		interpret();
	else
		printf("There are %d error(s) in PL/0 program.\n", err);
	//listcode(0, cx);



} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c

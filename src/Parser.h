

#if !defined(COCO_PARSER_H__)
#define COCO_PARSER_H__

#include <iostream>
#include <map>
#include <string>
#include <cwchar>
#include <fstream>

using namespace std;

struct Var {
    double val;
    double err;
    Var(double v = 0, double e = 0) : val(v), err(e) {}
};

static map<string, Var> vars;
static double exprValue;
static string idValue;
static bool wasIdent = false;


static string varName;
static double varVal;
static double varErr;


#include "Scanner.h"



class Errors {
public:
	int count;			// number of errors detected

	Errors();
	void SynErr(int line, int col, int n);
	void Error(int line, int col, const wchar_t *s);
	void Warning(int line, int col, const wchar_t *s);
	void Warning(const wchar_t *s);
	void Exception(const wchar_t *s);

}; // Errors

class Parser {
private:
	enum {
		_EOF=0,
		_ident=1,
		_number=2,
		_plus=3,
		_minus=4,
		_times=5,
		_slash=6,
		_assign=7,
		_lparen=8,
		_rparen=9,
		_semicolon=10,
		_print=11,
		_read=12,
		_variable=13,
		_plusminus=14,
		_alias=15,
		_as=16,
		_strToken=17,
		_exportToken=18,
		_to=19
	};
	int maxT;

	Token *dummyToken;
	int errDist;
	int minErrDist;

	void SynErr(int n);
	void Get();
	void Expect(int n);
	bool StartOf(int s);
	void ExpectWeak(int n, int follow);
	bool WeakSeparator(int n, int syFol, int repFol);

public:
	Scanner *scanner;
	Errors  *errors;

	Token *t;			// last recognized token
	Token *la;			// lookahead token



	Parser(Scanner *scanner);
	~Parser();
	void SemErr(const wchar_t* msg);

	void DataParser();
	void Statement();
	void VariableDecl();
	void Print();
	void Read();
	void Assignment();
	void AliasStmt();
	void ExportStmt();
	void Ident();
	void Expression();
	void Term();
	void Factor();

	void Parse();

}; // end Parser



#endif


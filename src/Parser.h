

#if !defined(COCO_PARSER_H__)
#define COCO_PARSER_H__

#include <iostream>
#include <map>
#include <string>
#include <cwchar>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

struct Var {
    double val;
    double err;
    Var(double v = 0, double e = 0) : val(v), err(e) {}
};

static map<string, Var> vars;
static map<string, vector<Var>> arrays;
static double exprValue;
static double exprError;
static string idValue;

static string varName;
static double varVal;
static double varErr;

static Var getArrayElement(const string& name, int index) {
    if (arrays.find(name) == arrays.end()) {
        cout << "Array '" << name << "' not defined" << endl;
        return Var(0,0);
    }
    if (index < 0 || index >= (int)arrays[name].size()) {
        cout << "Index " << index << " out of range for array '" << name << endl;
        return Var(0,0);
    }
    return arrays[name][index];
}

static void setArrayElement(const string& name, int index, const Var& value) {
    if (arrays.find(name) == arrays.end()) {
        cout << "Array '" << name << "' not defined" << endl;
        return;
    }
    if (index < 0 || index >= (int)arrays[name].size()) {
        cout << "Index " << index << " out of range for array '" << name << endl;
        return;
    }
    arrays[name][index] = value;
}

static void printArray(const string& name) {
    auto it = arrays.find(name);
    if (it == arrays.end()) {
        cout << "Array '" << name << "' not defined" << endl;
        return;
    }
    cout << name << " = [";
    for (size_t i = 0; i < it->second.size(); ++i) {
        if (i > 0) cout << ", ";
        cout << it->second[i].val << " +/- " << it->second[i].err;
    }
    cout << "]" << endl;
}

static void exportArrayToFile(const string& name, const string& filename) {
    auto it = arrays.find(name);
    if (it == arrays.end()) {
        cout << "Array '" << name << "' not defined" << endl;
        return;
    }
    ofstream f(filename, ios::app);
    if (!f.is_open()) {
        cout << "Error: could not open file " << filename << endl;
        return;
    }
    f << name << " = [";
    for (size_t i = 0; i < it->second.size(); ++i) {
        if (i > 0) f << ", ";
        f << it->second[i].val << " +/- " << it->second[i].err;
    }
    f << "]" << endl;
    f.close();
    cout << "Successfully exported array " << name << " to " << filename << endl;
}


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
		_lbrack=8,
		_rbrack=9,
		_lsqbr=10,
		_rsqbr=11,
		_semicolon=12,
		_print=13,
		_read=14,
		_variable=15,
		_plusminus=16,
		_alias=17,
		_as=18,
		_strToken=19,
		_exportToken=20,
		_to=21
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
	void Ident();
	void ArrayElement();
	void Expression();
	void Print();
	void Read();
	void Assignment();
	void AliasStmt();
	void ExportStmt();
	void ValueWithError();
	void Term();
	void Factor();

	void Parse();

}; // end Parser



#endif


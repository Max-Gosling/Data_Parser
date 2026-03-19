

#include <wchar.h>
#include "Parser.h"
#include "Scanner.h"




void Parser::SynErr(int n) {
	if (errDist >= minErrDist) errors->SynErr(la->line, la->col, n);
	errDist = 0;
}

void Parser::SemErr(const wchar_t* msg) {
	if (errDist >= minErrDist) errors->Error(t->line, t->col, msg);
	errDist = 0;
}

void Parser::Get() {
	for (;;) {
		t = la;
		la = scanner->Scan();
		if (la->kind <= maxT) { ++errDist; break; }

		if (dummyToken != t) {
			dummyToken->kind = t->kind;
			dummyToken->pos = t->pos;
			dummyToken->col = t->col;
			dummyToken->line = t->line;
			dummyToken->next = NULL;
			coco_string_delete(dummyToken->val);
			dummyToken->val = coco_string_create(t->val);
			t = dummyToken;
		}
		la = t;
	}
}

void Parser::Expect(int n) {
	if (la->kind==n) Get(); else { SynErr(n); }
}

void Parser::ExpectWeak(int n, int follow) {
	if (la->kind == n) Get();
	else {
		SynErr(n);
		while (!StartOf(follow)) Get();
	}
}

bool Parser::WeakSeparator(int n, int syFol, int repFol) {
	if (la->kind == n) {Get(); return true;}
	else if (StartOf(repFol)) {return false;}
	else {
		SynErr(n);
		while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
			Get();
		}
		return StartOf(syFol);
	}
}

void Parser::DataParser() {
		while (StartOf(1)) {
			Statement();
		}
		Expect(_EOF);
}

void Parser::Statement() {
		switch (la->kind) {
		case _variable: {
			VariableDecl();
			break;
		}
		case _print: {
			Print();
			break;
		}
		case _read: {
			Read();
			break;
		}
		case _ident: {
			Assignment();
			break;
		}
		case _alias: {
			AliasStmt();
			break;
		}
		case _exportToken: {
			ExportStmt();
			break;
		}
		default: SynErr(21); break;
		}
		while (!(la->kind == _EOF || la->kind == _semicolon)) {SynErr(22); Get();}
		Expect(_semicolon);
}

void Parser::VariableDecl() {
		Expect(_variable);
		Ident();
		varName = idValue; 
		Expect(_assign);
		Expression();
		varVal = exprValue; varErr = 0; 
		if (la->kind == _plusminus) {
			Get();
			Expression();
			varErr = exprValue; 
		}
		vars[varName] = Var(varVal, varErr);
		
}

void Parser::Print() {
		Expect(_print);
		Expression();
		if (wasIdent) {
		   cout << idValue << " = " << vars[idValue].val << " +/- " << vars[idValue].err << endl;
		} else {
		   cout << exprValue << " +/- " << 0.0 << endl;
		}
		wasIdent = false;
		
}

void Parser::Read() {
		Expect(_read);
		Expect(_variable);
		Ident();
		double val, err; 
		cout << "Enter variable val and err: ";
		cin >> val >> err;
		vars[idValue] = Var(val, err); 
}

void Parser::Assignment() {
		Ident();
		Expect(_assign);
		Expression();
		string name = idValue; vars[name] = Var(exprValue, 0); 
}

void Parser::AliasStmt() {
		Expect(_alias);
		Ident();
		string oldName = idValue; 
		Expect(_as);
		Ident();
		string newName = idValue; 
		if (vars.count(oldName)) {
		   vars[newName] = vars[oldName]; 
		} else {
		   cout << "Error: variable " << oldName << " not found" << endl;
		}
		
}

void Parser::ExportStmt() {
		Expect(_exportToken);
		Ident();
		string name = idValue; 
		Expect(_to);
		Expect(_strToken);
		wstring ws(t->val); 
		string p(ws.begin() + 1, ws.end() - 1); // Убираем кавычки из начала и конца
		
		ofstream f(p, ios::app); // Открываем файл в режиме добавления (append)
		if (f.is_open()) {
		   if (vars.count(name)) {
		       f << name << " = " << vars[name].val << " +/- " << vars[name].err << endl;
		       cout << "Successfully exported " << name << " to " << p << endl;
		   } else {
		       cout << "Error: variable " << name << " not defined." << endl;
		   }
		   f.close();
		} else {
		   cout << "Error: could not open file " << p << endl;
		}
		
}

void Parser::Ident() {
		Expect(_ident);
		wstring wide(t->val);
		idValue = string(wide.begin(), wide.end()); 
}

void Parser::Expression() {
		Term();
		double cur = exprValue; 
		while (la->kind == _plus || la->kind == _minus) {
			if (la->kind == _plus) {
				Get();
				Term();
				cur += exprValue; wasIdent = false; 
			} else {
				Get();
				Term();
				cur -= exprValue; wasIdent = false; 
			}
		}
		exprValue = cur; 
}

void Parser::Term() {
		Factor();
		double cur = exprValue; 
		while (la->kind == _times || la->kind == _slash) {
			if (la->kind == _times) {
				Get();
				Factor();
				cur *= exprValue; wasIdent = false; 
			} else {
				Get();
				Factor();
				cur /= exprValue; wasIdent = false; 
			}
		}
		exprValue = cur; 
}

void Parser::Factor() {
		if (la->kind == _number) {
			Get();
			wstring num(t->val); exprValue = stod(num); wasIdent = false; 
		} else if (la->kind == _ident) {
			Ident();
			exprValue = vars[idValue].val; wasIdent = true; 
		} else if (la->kind == _lparen) {
			Get();
			Expression();
			Expect(_rparen);
		} else SynErr(23);
}




// If the user declared a method Init and a mehtod Destroy they should
// be called in the contructur and the destructor respctively.
//
// The following templates are used to recognize if the user declared
// the methods Init and Destroy.

template<typename T>
struct ParserInitExistsRecognizer {
	template<typename U, void (U::*)() = &U::Init>
	struct ExistsIfInitIsDefinedMarker{};

	struct InitIsMissingType {
		char dummy1;
	};
	
	struct InitExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static InitIsMissingType is_here(...);

	// exist only if ExistsIfInitIsDefinedMarker is defined
	template<typename U>
	static InitExistsType is_here(ExistsIfInitIsDefinedMarker<U>*);

	enum { InitExists = (sizeof(is_here<T>(NULL)) == sizeof(InitExistsType)) };
};

template<typename T>
struct ParserDestroyExistsRecognizer {
	template<typename U, void (U::*)() = &U::Destroy>
	struct ExistsIfDestroyIsDefinedMarker{};

	struct DestroyIsMissingType {
		char dummy1;
	};
	
	struct DestroyExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static DestroyIsMissingType is_here(...);

	// exist only if ExistsIfDestroyIsDefinedMarker is defined
	template<typename U>
	static DestroyExistsType is_here(ExistsIfDestroyIsDefinedMarker<U>*);

	enum { DestroyExists = (sizeof(is_here<T>(NULL)) == sizeof(DestroyExistsType)) };
};

// The folloing templates are used to call the Init and Destroy methods if they exist.

// Generic case of the ParserInitCaller, gets used if the Init method is missing
template<typename T, bool = ParserInitExistsRecognizer<T>::InitExists>
struct ParserInitCaller {
	static void CallInit(T *t) {
		// nothing to do
	}
};

// True case of the ParserInitCaller, gets used if the Init method exists
template<typename T>
struct ParserInitCaller<T, true> {
	static void CallInit(T *t) {
		t->Init();
	}
};

// Generic case of the ParserDestroyCaller, gets used if the Destroy method is missing
template<typename T, bool = ParserDestroyExistsRecognizer<T>::DestroyExists>
struct ParserDestroyCaller {
	static void CallDestroy(T *t) {
		// nothing to do
	}
};

// True case of the ParserDestroyCaller, gets used if the Destroy method exists
template<typename T>
struct ParserDestroyCaller<T, true> {
	static void CallDestroy(T *t) {
		t->Destroy();
	}
};

void Parser::Parse() {
	t = NULL;
	la = dummyToken = new Token();
	la->val = coco_string_create(L"Dummy Token");
	Get();
	DataParser();
	Expect(0);
}

Parser::Parser(Scanner *scanner) {
	maxT = 20;

	ParserInitCaller<Parser>::CallInit(this);
	dummyToken = NULL;
	t = la = NULL;
	minErrDist = 2;
	errDist = minErrDist;
	this->scanner = scanner;
	errors = new Errors();
}

bool Parser::StartOf(int s) {
	const bool T = true;
	const bool x = false;

	static bool set[2][22] = {
		{T,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x},
		{x,T,x,x, x,x,x,x, x,x,x,T, T,T,x,T, x,x,T,x, x,x}
	};



	return set[s][la->kind];
}

Parser::~Parser() {
	ParserDestroyCaller<Parser>::CallDestroy(this);
	delete errors;
	delete dummyToken;
}

Errors::Errors() {
	count = 0;
}

void Errors::SynErr(int line, int col, int n) {
	wchar_t* s;
	switch (n) {
			case 0: s = coco_string_create(L"EOF expected"); break;
			case 1: s = coco_string_create(L"ident expected"); break;
			case 2: s = coco_string_create(L"number expected"); break;
			case 3: s = coco_string_create(L"plus expected"); break;
			case 4: s = coco_string_create(L"minus expected"); break;
			case 5: s = coco_string_create(L"times expected"); break;
			case 6: s = coco_string_create(L"slash expected"); break;
			case 7: s = coco_string_create(L"assign expected"); break;
			case 8: s = coco_string_create(L"lparen expected"); break;
			case 9: s = coco_string_create(L"rparen expected"); break;
			case 10: s = coco_string_create(L"semicolon expected"); break;
			case 11: s = coco_string_create(L"print expected"); break;
			case 12: s = coco_string_create(L"read expected"); break;
			case 13: s = coco_string_create(L"variable expected"); break;
			case 14: s = coco_string_create(L"plusminus expected"); break;
			case 15: s = coco_string_create(L"alias expected"); break;
			case 16: s = coco_string_create(L"as expected"); break;
			case 17: s = coco_string_create(L"strToken expected"); break;
			case 18: s = coco_string_create(L"exportToken expected"); break;
			case 19: s = coco_string_create(L"to expected"); break;
			case 20: s = coco_string_create(L"??? expected"); break;
			case 21: s = coco_string_create(L"invalid Statement"); break;
			case 22: s = coco_string_create(L"this symbol not expected in Statement"); break;
			case 23: s = coco_string_create(L"invalid Factor"); break;

		default:
		{
			wchar_t format[20];
			coco_swprintf(format, 20, L"error %d", n);
			s = coco_string_create(format);
		}
		break;
	}
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	coco_string_delete(s);
	count++;
}

void Errors::Error(int line, int col, const wchar_t *s) {
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	count++;
}

void Errors::Warning(int line, int col, const wchar_t *s) {
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
}

void Errors::Warning(const wchar_t *s) {
	wprintf(L"%ls\n", s);
}

void Errors::Exception(const wchar_t* s) {
	wprintf(L"%ls", s); 
	exit(1);
}



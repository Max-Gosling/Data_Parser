

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
			Get();
			Ident();
			Expect(_assign);
			if (la->kind == _lsqbr) {
				Get();
				string arrName = idValue; vector<Var> elems; 
				ArrayElement();
				elems.push_back(Var(varVal, varErr)); 
				while (la->kind == 22 /* "," */) {
					Get();
					ArrayElement();
					elems.push_back(Var(varVal, varErr)); 
				}
				Expect(_rsqbr);
				arrays[arrName] = elems; 
			} else if (StartOf(2)) {
				varName = idValue; 
				Expression();
				varVal = exprValue; varErr = 0; 
				if (la->kind == _plusminus) {
					Get();
					Expression();
					varErr = exprValue; 
				}
				vars[varName] = Var(varVal, varErr); 
			} else SynErr(24);
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
		default: SynErr(25); break;
		}
		while (!(la->kind == _EOF || la->kind == _semicolon)) {SynErr(26); Get();}
		Expect(_semicolon);
}

void Parser::Ident() {
		Expect(_ident);
		wstring wide(t->val);
		idValue = string(wide.begin(), wide.end()); 
}

void Parser::ArrayElement() {
		if (la->kind == _number) {
			Get();
			wstring num(t->val); varVal = stod(num); varErr = 0; 
			if (la->kind == _plusminus) {
				Get();
				Expect(_number);
				wstring num(t->val); varErr = stod(num); 
			}
		} else if (la->kind == _ident) {
			Ident();
			string name = idValue;
			if (vars.find(name) != vars.end()) {
			   varVal = vars[name].val;
			   varErr = vars[name].err;
			} else {
			   cout << "Variable '" << name << "' not defined" << endl;
			   varVal = 0; varErr = 0;
			}
			
		} else SynErr(27);
}

void Parser::Expression() {
		double v1, e1; 
		Term();
		v1 = exprValue; e1 = exprError; 
		while (la->kind == _plus || la->kind == _minus) {
			if (la->kind == _plus) {
				Get();
				Term();
				exprValue = v1 + exprValue; 
				exprError = sqrt(pow(e1, 2) + pow(exprError, 2));
				v1 = exprValue; e1 = exprError;
				
			} else {
				Get();
				Term();
				exprValue = v1 - exprValue; 
				exprError = sqrt(pow(e1, 2) + pow(exprError, 2));
				v1 = exprValue; e1 = exprError;
				
			}
		}
}

void Parser::Print() {
		Expect(_print);
		if (la->kind == _ident) {
			Ident();
			string name = idValue; bool hasIndex = false; int idx; 
			if (la->kind == _lsqbr) {
				Get();
				Expression();
				Expect(_rsqbr);
				hasIndex = true; idx = (int)exprValue; 
			}
			if (hasIndex) {
			   Var elem = getArrayElement(name, idx);
			   cout << name << "[" << idx << "] = " << elem.val << " +/- " << elem.err << endl;
			} else {
			   if (arrays.find(name) != arrays.end())
			       printArray(name);
			   else if (vars.find(name) != vars.end())
			       cout << name << " = " << vars[name].val << " +/- " << vars[name].err << endl;
			   else
			       cout << "'" << name << "' not defined" << endl;
			}
			
		} else if (StartOf(2)) {
			Expression();
			cout << exprValue << " +/- " << exprError << endl;
			
		} else SynErr(28);
}

void Parser::Read() {
		Expect(_read);
		Expect(_variable);
		Ident();
		double val, err;
		cout << "Enter variable val and err via space: ";
		cin >> val >> err;
		vars[idValue] = Var(val, err);
		
}

void Parser::Assignment() {
		Ident();
		string name = idValue; bool hasIndex = false; int idx; 
		if (la->kind == _lsqbr) {
			Get();
			Expression();
			Expect(_rsqbr);
			hasIndex = true; idx = (int)exprValue; 
		}
		Expect(_assign);
		ValueWithError();
		if (hasIndex)
		   setArrayElement(name, idx, Var(varVal, varErr));
		else
		   vars[name] = Var(varVal, varErr);
		
}

void Parser::AliasStmt() {
		Expect(_alias);
		Ident();
		string old_name = idValue; 
		Expect(_as);
		Ident();
		string new_name = idValue; 
		auto node = vars.extract(old_name);
		if (!node.empty()) {
		   node.key() = new_name;
		   vars.insert(std::move(node));
		} else {
		   cout << "Variable '" << old_name << "' not found" << endl;
		}
		
}

void Parser::ExportStmt() {
		Expect(_exportToken);
		Ident();
		string name = idValue; 
		Expect(_to);
		Expect(_strToken);
		wstring ws(t->val); 
		string p(ws.begin() + 1, ws.end() - 1);
		if (vars.count(name)) {
		   ofstream f(p, ios::app);
		   if (f.is_open()) {
		       f << name << " = " << vars[name].val << " +/- " << vars[name].err << endl;
		       cout << "Successfully exported " << name << " to " << p << endl;
		       f.close();
		   } else {
		       cout << "Error: could not open file " << p << endl;
		   }
		} else if (arrays.count(name)) {
		   exportArrayToFile(name, p);
		} else {
		   cout << "Variable/array " << name << " not defined" << endl;
		}
		
}

void Parser::ValueWithError() {
		if (la->kind == _number) {
			Get();
			wstring num(t->val); varVal = stod(num); varErr = 0; 
			if (la->kind == _plusminus) {
				Get();
				Expect(_number);
				wstring num(t->val); varErr = stod(num); 
			}
		} else if (la->kind == _ident) {
			Ident();
			string name = idValue;
			if (vars.find(name) != vars.end()) {
			   varVal = vars[name].val;
			   varErr = vars[name].err;
			} else {
			   cout << "Variable '" << name << "' not defined" << endl;
			   varVal = 0; varErr = 0;
			}
			
		} else SynErr(29);
}

void Parser::Term() {
		double v1, e1; 
		Factor();
		v1 = exprValue; e1 = exprError; 
		while (la->kind == _times || la->kind == _slash) {
			if (la->kind == _times) {
				Get();
				Factor();
				exprError = sqrt(pow(v1 * exprError, 2) + pow(exprValue * e1, 2)); 
				exprValue = v1 * exprValue; 
				v1 = exprValue; e1 = exprError;
				
			} else {
				Get();
				Factor();
				exprError = sqrt(pow(e1 / exprValue, 2) + pow((v1 * exprError) / pow(exprValue, 2), 2));
				exprValue = v1 / exprValue; 
				v1 = exprValue; e1 = exprError;
				
			}
		}
}

void Parser::Factor() {
		bool negative = false; string name; int idx; bool isArrayAccess = false; 
		if (la->kind == _minus) {
			Get();
			negative = true; 
		}
		if (la->kind == _number) {
			Get();
			wstring num(t->val); 
			exprValue = stod(num);
			exprError = 0.0;
			if (negative) exprValue = -exprValue;
			
		} else if (la->kind == _ident) {
			Ident();
			name = idValue;
			if (la->kind == _lsqbr) {
			   isArrayAccess = true;
			} else {
			   if (vars.find(name) != vars.end()) {
			       exprValue = vars[name].val;
			       exprError = vars[name].err;
			   } else if (arrays.find(name) != arrays.end()) {
			       cout << "Cannot use array '" << name << "' without index" << endl;
			       exprValue = 0; exprError = 0;
			   } else {
			       cout << "Variable '" << name << "' not defined" << endl;
			       exprValue = 0; exprError = 0;
			   }
			   if (negative) exprValue = -exprValue;
			}
			
			if (la->kind == _lsqbr) {
				Get();
				Expression();
				Expect(_rsqbr);
				if (isArrayAccess) {
				  idx = (int)exprValue;
				  Var elem = getArrayElement(name, idx);
				  exprValue = elem.val;
				  exprError = elem.err;
				  if (negative) exprValue = -exprValue;
				}
				
			}
		} else if (la->kind == _lbrack) {
			Get();
			Expression();
			Expect(_rbrack);
			if (negative) exprValue = -exprValue; 
		} else SynErr(30);
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
	maxT = 23;

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

	static bool set[3][25] = {
		{T,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,T,x,x, x,x,x,x, x,x,x,x, x,T,T,T, x,T,x,x, T,x,x,x, x},
		{x,T,T,x, T,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x}
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
			case 8: s = coco_string_create(L"lbrack expected"); break;
			case 9: s = coco_string_create(L"rbrack expected"); break;
			case 10: s = coco_string_create(L"lsqbr expected"); break;
			case 11: s = coco_string_create(L"rsqbr expected"); break;
			case 12: s = coco_string_create(L"semicolon expected"); break;
			case 13: s = coco_string_create(L"print expected"); break;
			case 14: s = coco_string_create(L"read expected"); break;
			case 15: s = coco_string_create(L"variable expected"); break;
			case 16: s = coco_string_create(L"plusminus expected"); break;
			case 17: s = coco_string_create(L"alias expected"); break;
			case 18: s = coco_string_create(L"as expected"); break;
			case 19: s = coco_string_create(L"strToken expected"); break;
			case 20: s = coco_string_create(L"exportToken expected"); break;
			case 21: s = coco_string_create(L"to expected"); break;
			case 22: s = coco_string_create(L"\",\" expected"); break;
			case 23: s = coco_string_create(L"??? expected"); break;
			case 24: s = coco_string_create(L"invalid Statement"); break;
			case 25: s = coco_string_create(L"invalid Statement"); break;
			case 26: s = coco_string_create(L"this symbol not expected in Statement"); break;
			case 27: s = coco_string_create(L"invalid ArrayElement"); break;
			case 28: s = coco_string_create(L"invalid Print"); break;
			case 29: s = coco_string_create(L"invalid ValueWithError"); break;
			case 30: s = coco_string_create(L"invalid Factor"); break;

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



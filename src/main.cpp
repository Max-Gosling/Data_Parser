#include "Scanner.h"
#include "Parser.h"
#include <iostream>

int main()
{

    Scanner scanner(L"..\\src\\input.txt");

    Parser parser(&scanner);

    parser.Parse();

    if (parser.errors->count > 0)
    {
        std::cerr << "Errors found\n";
        return 1;
    }

    std::cout << "Parsing succeeded\n";
    return 0;
}
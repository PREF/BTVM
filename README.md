# BTVM
C++11 implementation of 010 Editor's template language

## Status
BTVM is in early state, lexing and parsing works on simple scripts (BMP and WAV format), DATETIME's related data types aren't implemented yet and more testing is required.
A detailed wiki page about BTVM's status will be available soon

## Requirements
re2c and lemon parser generator are required in order to compile BTVM

## Build
The only required step is to generate a lexer and parser with re2c and lemon and to include generated files in your project

```
cd generator
make
cd ..
```

## Usage

```
#include <iostream>
#include "btvm/btvm.h"
#include "your_custom_io_class.h"

using namespace std;

// Prints file structure to console
void printElements(const BTEntryList& entries, const std::string& prefix)
{
    for(auto it = entries.begin(); it != entries.end(); it++)
    {
        cout << prefix << (*it)->name << " at offset " << (*it)->location.offset << ", size " << (*it)->location.size << endl;
        printElements((*it)->children, prefix + "  ");
    }

    if(!entries.empty())
        cout << endl;
}

int main()
{
   BTVM btvm(new YourCustomIOClass("myfile.bin"));
   btvm.dump("ast.xml"); // Dumps AST to file
   btvm.execute("BMPFormat.bt");
   
   BTEntryList btformat = btvm.format(); // Get format
   printElements(btformat, std::string());

   return 0;
}

```

## License
BTVM is released under GPL3 License

// Chapter 3 toy: what char* argv[] is. Each argv[i] is the address of the first letter of one word.
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: argv_pointer <word>" << std::endl;
        return 1;
    }
    char* word = argv[1];              // a pointer to the first character
    std::cout << "word     = " << word << std::endl;      // << follows it until the end marker
    std::cout << "word[0]  = " << word[0] << std::endl;   // the first character itself
    std::cout << "*word    = " << *word << std::endl;     // same thing, written with *

    std::string copy = argv[1];        // run_simulation.cpp:16 does this: copy the letters into a string
    copy[0] = 'X';
    std::cout << "copy     = " << copy << ", argv[1] = " << argv[1] << std::endl;
    return 0;
}

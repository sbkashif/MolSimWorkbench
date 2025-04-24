#ifndef FILE_READER_H
#define FILE_READER_H

#include <string>
#include <vector>

class FileReader {
public:
    FileReader(const std::string& filename);
    std::vector<std::string> readLines();
    std::string readAll();
    
private:
    std::string filename;
};

#endif // FILE_READER_H
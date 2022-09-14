#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class LineReader {
  public:
    LineReader(std::string path) : path(path) {}

    std::vector<std::string> read() {
        if (done)
            return result;

        std::ifstream in(path);
        std::string line;
        while (in >> line)
            result.push_back(line);

        done = true;

        return result;
    }

  private:
    std::string path;
    bool done = false;
    std::vector<std::string> result;
};

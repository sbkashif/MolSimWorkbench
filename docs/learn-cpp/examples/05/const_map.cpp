// [] on a map inserts a missing key; .at() throws instead.
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

struct AtomParameters {
    double epsilon;
    double sigma;
};

int main() {
    std::map<std::string, AtomParameters> params = {{"LJ", {1.0, 1.0}}};

    // A typo: "Lj" instead of "LJ".
    const double eps = params["Lj"].epsilon;
    std::cout << "params[\"Lj\"].epsilon = " << eps << std::endl;
    std::cout << "map size is now " << params.size() << std::endl;

    try {
        params.at("Xe");
    } catch (const std::out_of_range& error) {
        std::cout << "params.at(\"Xe\") threw: " << error.what() << std::endl;
    }
    return 0;
}

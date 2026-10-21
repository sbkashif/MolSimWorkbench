// Chapter 4 toy: std::map, and the trap in operator[].
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

struct Params {
    double epsilon;
    double sigma;
};

int main() {
    std::map<std::string, Params> params;
    params["LJ"] = {1.0, 1.0};
    std::cout << "size = " << params.size() << std::endl;

    // Reading a missing key with [] does not fail. It INSERTS a default entry.
    const Params& ar = params["Ar"];
    std::cout << "params[\"Ar\"].epsilon = " << ar.epsilon << ", size now = " << params.size() << std::endl;

    // .at() refuses: it throws std::out_of_range.
    try {
        params.at("Ne");
    } catch (const std::out_of_range& e) {
        std::cout << "params.at(\"Ne\") threw std::out_of_range" << std::endl;
    }

    // .find() asks without inserting. end() means "not found".
    if (params.find("Xe") == params.end()) std::cout << "Xe not found, size still " << params.size() << std::endl;

    // Iteration visits keys in sorted order.
    for (const auto& entry : params) std::cout << entry.first << " ";
    std::cout << std::endl;
    return 0;
}

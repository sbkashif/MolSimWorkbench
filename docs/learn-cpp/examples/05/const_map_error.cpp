// A const map cannot use [], because [] might insert.
#include <map>
#include <string>

double firstEpsilon(const std::map<std::string, double>& epsilons) {
    return epsilons["LJ"];
}

int main() {
    return 0;
}

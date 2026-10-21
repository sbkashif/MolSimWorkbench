// Chapter 3 toy: a const reference may be read, never written.
#include <array>

using Vec3 = std::array<double, 3>;

double firstComponent(const Vec3& v) {
    v[0] = 0.0;          // error: v is read-only here
    return v[0];
}

int main() {
    Vec3 p = {1.0, 2.0, 3.0};
    return static_cast<int>(firstComponent(p));
}

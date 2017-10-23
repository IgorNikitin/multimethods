#include <iostream>
#include <multimethods.h>
using namespace std;

struct vehicle : multimethods::unknown {};
struct car : vehicle {};
struct inspector : multimethods::unknown {};
struct state_inspector : inspector {};

define_method(inspect)
    match(vehicle&, inspector&) { cout << "Inspect vehicle.\n"; }
    match(car&, inspector&) { cout << "Inspect seat belts.\n"; next_method; }
    match(car&, state_inspector&) { cout << "Check insurance.\n"; next_method; }
end_method

define_method(join, string)
    match(int x, int y, int z) { return to_string(x) + to_string(y) + to_string(z); }
    match(string s1, string s2) { return s1 + s2; }
    fallback { return "fallback"; }
end_method

int main() {
    inspect(car(), state_inspector());  // Check insurance. Inspect seat belts. Inspect vehicle.

    try {
        inspect(car(), true);
    } catch(multimethods::not_implemented& e) {
        cout << e.what() << endl; // inspect: not_implemented
    }

    cout << join(1, 2, 3) << endl; // 123
    cout << join("Hello,"s, " world."s) << endl; // Hello, world.
    cout << join(false) << endl; // fallback
}

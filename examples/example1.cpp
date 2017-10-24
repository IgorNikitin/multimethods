#include <iostream>
#include <multimethods.h>
using namespace std;

struct vehicle { virtual ~vehicle(){} };
struct car : vehicle {};
struct inspector { virtual ~inspector(){} };
struct state_inspector : inspector {};

void multi_method(inspect, const vehicle&, const inspector&)
    match(const car&, const inspector&) { cout << "Inspect seat belts.\n"; next_method; }
    match(const car&, const state_inspector&) { cout << "Check insurance.\n"; next_method; }
    match(const vehicle&, const inspector&) { cout << "Inspect vehicle.\n"; }
end_method

string multi_method(join, any, any)
    match(int x, int y) { return to_string(x) + to_string(y); }
    match(string s1, string s2) { return s1 + s2; }
end_method

int main() {
    inspect(car(), state_inspector());  // Check insurance. Inspect seat belts. Inspect vehicle.

    cout << join(1, 2) << endl; // 123
    cout << join("Hello,"s, " world."s) << endl; // Hello, world.

    try {
        join(true, false);
    } catch(multimethods::not_implemented& e) {
        cout << e.what() << endl; // join: not_implemented
    }
}

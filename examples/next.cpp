#include <iostream>
#include <multimethods.h>
using namespace std;

struct vehicle { virtual ~vehicle() {} };
struct car : vehicle {};
struct inspector { virtual ~inspector() {} };
struct state_inspector : inspector {};

void multi_method(inspect, const vehicle&, const inspector&)
    match(const vehicle&, const inspector&) { cout << "Inspect vehicle.\n"; }
    match(const car&, const inspector&) { cout << "Inspect seat belts.\n"; next_method; }
    match(const car&, const state_inspector&) { cout << "Check insurance.\n"; next_method; }
end_method

int main() {
    inspect(car(), state_inspector()); // Check insurance. Inspect seat belts. Inspect vehicle.
}

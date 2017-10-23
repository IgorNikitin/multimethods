#include <iostream>
#include <multimethods.h>
using namespace multimethods;
using namespace std;

struct vehicle : unknown {};
struct car : vehicle {};
struct inspector : unknown {};
struct state_inspector : inspector {};

define_method(inspect)
    match(vehicle&, inspector&) { cout << "Inspect vehicle.\n"; }
    match(car&, inspector&) { cout << "Inspect seat belts.\n"; next_method; }
    match(car&, state_inspector&) { cout << "Check insurance.\n"; next_method; }
end_method

int main() {
    inspect(car(), state_inspector()); // Check insurance. Inspect seat belts. Inspect vehicle.
}

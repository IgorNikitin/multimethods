#include <iostream>
#include <multimethods.h>
using namespace multimethods;
using namespace std;

struct Vehicle : unknown {};
struct Car : Vehicle {};
struct Truck : Vehicle {};
struct Inspector : unknown {};
struct StateInspector : Inspector {};

declare_method(inspect)
define_method(inspect, Car&, StateInspector&) { cout << "Check insurance.\n"; skip_method; }
define_method(inspect, Car&, Inspector&) { cout << "Inspect seat belts.\n"; skip_method; }
define_method(inspect, Vehicle&, Inspector&) { cout << "Inspect vehicle.\n"; }

int main() {
  Car car;
  StateInspector inspector;
  inspect(car, inspector); // Check insurance. Inspect seat belts. Inspect vehicle.
}

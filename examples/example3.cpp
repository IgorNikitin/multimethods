#include <iostream>
#include "multimethods.h"
using namespace std;

struct base1 {};
struct derived1 : base1 {};

struct base2 : multimethods::unknown {};
struct derived2 : base2 {};

declare_method(print)
define_method(print, derived1&) { cout << "derived1\n"; }
define_method(print, derived2&) { cout << "derived2\n"; }
define_method(print, base1&) { cout << "base1\n"; }
define_method(print, base2&) { cout << "base2\n"; }

int main() {
    base1 b1;
    base2 b2;
    derived1 d1;
    derived2 d2;

    print(b1); // base1
    print(b2); // base2
    print(d1); // derived1
    print(d2); // derived2

    print(static_cast<base1&>(d1)); // base1
    print(static_cast<base2&>(d2)); // derived2
}

#include <iostream>
#include <multimethods.h>
using namespace std;

struct base1 : multimethods::unknown {};
struct derived1 : base1 {};

struct base2 { virtual ~base2(){} };
struct derived2 : base2 {};

void multi_method(print1, base1)
    match(base1&) { cout << "base1\n"; }
    match(derived1&) { cout << "derived1\n"; }
end_method

void multi_method(print2, base2)
    match(base2&) { cout << "base2\n"; }
    match(derived2&) { cout << "derived2\n"; }
end_method


int main() {
    base1 b1;
    base2 b2;
    derived1 d1;
    derived2 d2;

    print1(b1); // base1
    print1(d1); // derived1

    print2(b2); // base2
    print2(d2); // derived2

    print1(static_cast<base1&>(d1)); // derived1
    print2(static_cast<base2&>(d2)); // derived2
}

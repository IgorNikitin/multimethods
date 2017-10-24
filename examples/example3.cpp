#include <iostream>
#include <multimethods.h>
using namespace std;

struct base { virtual ~base(){} };
struct derived : base {};

void multi_method(print, base)
    match(base&) { cout << "base\n"; }
    match(derived&) { cout << "derived\n"; }
end_method


int main() {
    base b;
    derived d;

    print(b); // base
    print(d); // derived
    print(static_cast<base&>(d)); // derived
}

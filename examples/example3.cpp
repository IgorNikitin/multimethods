#include <iostream>
#include <multimethods.h>
using namespace std;

struct base { virtual ~base(){} };
struct derived : base {};

base& multi_method(print, base&)
    match(base& b) { cout << "base\n"; return b; }
    match(derived& d) { cout << "derived\n"; return d; }
end_method


int main() {
    base b;
    derived d;

    print(b); // base
    print(d); // derived
    print(static_cast<base&>(d)); // derived
}

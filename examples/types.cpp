#include <iostream>
#include <multimethods.h>
using namespace std;

int multi_method(do1, int)
    match(int n) { if(n > 0) next_method; return -n; }
    match(double n) { if(n < 0) next_method; return n; }
end_method

int multi_method(do2, int)
    match(const int n) { if(n > 0) next_method; return -n; }
    match(const double n) { if(n < 0) next_method; return n; }
end_method

int multi_method(do3, int)
    match(const int& n) { if(n > 0) next_method; return -n; }
    match(double n) { if(n < 0) next_method; return n; }
end_method

void multi_method(do4, int)
    match(double n) {}
    match(char n) {}
    match(int n) { cout << "without casting\n"; }
    match(long n) {}
end_method

struct dummy {
};

void multi_method(do5, const dummy&)
    match(const dummy& n) { cout << "const ref\n"; }
    match(dummy& n) { cout << "ref\n"; }
end_method

struct base {};
struct derived : base {};

void multi_method(do6, const base&)
    match(base& n) { cout << "base\n"; }
end_method

int main() {
    cout << do1(-10) << endl; // 10
    cout << do1(10) << endl; // 10

    do4(10); // without casting

    do5(dummy()); // ref
    const dummy d;
    do5(d); // const ref

    do6(derived()); // base
}

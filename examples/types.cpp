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

int main() {
    cout << do1(-10) << endl; // 10
    cout << do1(10) << endl; // 10
}

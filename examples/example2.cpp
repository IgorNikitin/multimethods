#include <iostream>
#include <multimethods.h>
using namespace std;

int multi_method(my_abs, int)
    match(int n) { if(n > 0) next_method; return -n; }
    match(int n) { if(n < 0) next_method; return n; }
end_method

int main() {
    cout << my_abs(-15.) << endl; // 15
    cout << my_abs(10) << endl; // 10
}

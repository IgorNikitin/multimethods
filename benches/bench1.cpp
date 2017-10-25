#include <ctime>
#include <iostream>
#include <multimethods.h>
using namespace std;

struct vehicle { virtual ~vehicle() {} };
struct car : vehicle {};
struct inspector { virtual ~inspector() {} };
struct state_inspector : inspector {};

void multi_method(inspect, const vehicle&, const inspector&)
    match(const vehicle&, const inspector&) {}
    match(const car&, const inspector&) { next_method; }
    match(const car&, const state_inspector&) { next_method; }
end_method

int main() {
   car c;
   state_inspector si;

   clock_t begin = clock();

   for(int i = 0; i < 10000000000; ++i)
       inspect(c, si);

    const double ms = double(clock() - begin) / CLOCKS_PER_SEC;
    printf("%f\n", ms);
}

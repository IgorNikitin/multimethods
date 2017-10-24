#include <ctime>
#include <iostream>
#include <multimethods.h>
using namespace std;

struct base { virtual ~base() {} };
struct asteroid : base {};
struct spaceship : base {};

void multi_method(collide, base&, base&)
    match(asteroid&, asteroid&) {}
    match(asteroid&, spaceship&) {}
    match(spaceship&, asteroid&) {}
    match(spaceship&, spaceship&) {}
end_method

int main() {
   asteroid a;
   spaceship s1, s2;

   clock_t begin = clock();

   for(int i = 0; i < 10000000; ++i)
       collide(s1, s2);

    const double ms = double(clock() - begin) / CLOCKS_PER_SEC;
    printf("%f\n", ms);
}

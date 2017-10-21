#include <ctime>
#include <iostream>
#include <multimethods.h>
using namespace std;

struct base : public multimethods::unknown {};
struct asteroid : base {};
struct spaceship : base {};

declare_method(collide)
define_method(collide, asteroid&, asteroid&) {}
define_method(collide, asteroid&, spaceship&) {}
define_method(collide, spaceship&, asteroid&) {}
define_method(collide, spaceship&, spaceship&) {}

int main() {
   asteroid a;
   spaceship s1, s2;

   clock_t begin = clock();

   for(int i = 0; i < 1000000; ++i)
       collide(static_cast<base&>(s1), static_cast<base&>(s2));

    const double ms = double(clock() - begin) / CLOCKS_PER_SEC;
    printf("%f\n", ms);
}

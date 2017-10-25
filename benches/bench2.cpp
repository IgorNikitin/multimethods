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
    spaceship s;

    clock_t begin = clock();

    for(int i = 0; i < 250000000; ++i) {
        collide(a, a);
        collide(a, s);
        collide(s, a);
        collide(s, s);
    }

    const double ms = double(clock() - begin) / CLOCKS_PER_SEC;
    printf("%f\n", ms);
}

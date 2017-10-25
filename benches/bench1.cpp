#include <ctime>
#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing { MM_CLASS() virtual ~thing() {} };
struct asteroid final : thing { MM_CLASS(thing) };
struct spaceship final : thing { MM_CLASS(thing) };

void multi_method(collide, const thing&, const thing&)
    match(const asteroid&, const asteroid&) {}
    match(const asteroid&, const spaceship&) {}
    match(const spaceship&, const asteroid&) {}
    match(const spaceship&, const spaceship&) {}
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

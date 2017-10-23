#include <iostream>
#include <multimethods.h>
using namespace std;

struct asteroid {};
struct spaceship {};

define_method(collide)
    match(asteroid&, const spaceship&) { cout << "God mode.\n"; }
    match(asteroid&, spaceship&) { cout << "Game over.\n"; }
end_method

int main() {
    asteroid a;
    spaceship s;

    collide(a, const_cast<const spaceship&>(s)); // God mode
    collide(a, s); // Game over
}

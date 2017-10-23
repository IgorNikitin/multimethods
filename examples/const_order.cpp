#include <iostream>
#include <multimethods.h>
using namespace std;

struct asteroid {};
struct spaceship {};

void multi_method(collide)
    match(asteroid&, const spaceship&) { cout << "Just check.\n"; }
    match(asteroid&, spaceship&) { cout << "Game over.\n"; }
end_method

int main() {
    asteroid a;
    spaceship s;

    collide(a, const_cast<const spaceship&>(s)); // Just check
    collide(a, s); // Game over
}

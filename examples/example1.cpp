#include <iostream>
#include <multimethods.h>
using namespace std;

struct asteroid {};
struct spaceship : multimethods::unknown {};
struct spaceship_big : spaceship {};

define_method(collide)
    match(asteroid&, spaceship&) { cout << "Boom!\n"; }
    match(asteroid&, spaceship_big&) { cout << "Big boom!\n"; }
    match(spaceship&, const spaceship& s) { cout << "Knock, knock.\n"; }
end_method

define_method(join, string)
    match(int x, int y, int z) { return to_string(x) + to_string(y) + to_string(z); }
    match(string s1, string s2) { return s1 + s2; }
//    fallback { return "Fallback."; }
end_method

define_method(mm_abs, int)
    match(int n) { if(n > 0) next_method; return -n; }
    match(int n) { return n; }
end_method

// TODO: const example - match to const or not const, sort by const

int main() {
    asteroid a;
    spaceship s1, s2;
    spaceship_big bs;

    collide(a, s1); // Boom!
    collide(a, bs); // Big boom!
    collide(s2, bs); // Knock, knock.

    try {
        collide(a, true);
    } catch(multimethods::not_implemented& e) {
        cout << e.what() << endl; // collide: not_implemented
    }

    cout << join(1, 2, 3) << endl; // 123
    cout << join("Hello,"s, " world."s) << endl; // Hello, world.
    //cout << join(a, s1) << endl; // Fallback.

    cout << mm_abs(-10) << endl; // 10
    cout << mm_abs(10) << endl; // 10
}

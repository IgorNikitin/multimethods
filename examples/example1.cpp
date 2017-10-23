#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing : multimethods::unknown {};
struct asteroid : thing {};
struct bullet : thing {};
struct spaceship : thing {};

define_method(collide)
    match(asteroid&, asteroid&) { cout << "traverse\n"; }
    match(asteroid&, bullet&) { cout << "hit\n"; }
    match(asteroid&, spaceship&) { cout << "boom\n"; }
    match(thing& t, asteroid& a) { collide(a, t); }
    fallback {}
end_method

define_method(join, string)
    match(int x, int y, int z) { return to_string(x) + to_string(y) + to_string(z); }
    match(string s1, string s2) { return s1 + s2; }
    match() { return {}; }
    fallback { return "fallback"; }
end_method

int main() {
    collide(asteroid(), spaceship()); // boom
    collide(bullet(), asteroid()); // hit

    try {
        collide(asteroid(), true);
    } catch(multimethods::not_implemented& e) {
        cout << e.what() << endl; // collide: not_implemented
    }

    cout << join(1, 2, 3) << endl; // 123
    cout << join("Hello,"s, " world."s) << endl; // Hello, world.
    cout << join() << endl; //
    cout << join(false) << endl; // fallback
}

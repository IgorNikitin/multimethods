#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing { virtual ~thing(){} };
struct asteroid : thing {};
struct bullet : thing {};
struct spaceship : thing {};

define_method(collide, string, thing)
    match(asteroid&, asteroid&) { return "Traverse"; }
    match(asteroid&, bullet&) { return "Hit"; }
    match(asteroid&, spaceship&) { return "Boom"; }
    match(thing& t, asteroid& a) { return collide(a, t); }
    fallback { return ""; }
end_method

int main() {
   asteroid a;
   bullet b;
   spaceship s;

   cout << collide(a, a) << endl; // 'Traverse'
   cout << collide(a, b) << endl; // 'Hit'
   cout << collide(b, a) << endl; // 'Hit'
   cout << collide(s, a) << endl; // 'Boom'
   cout << collide(a, s) << endl; // 'Boom'
   cout << collide(b, b) << endl; // fallback
}

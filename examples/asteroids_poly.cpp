#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing { virtual ~thing(){} };
struct asteroid : thing {};
struct bullet : thing {};
struct spaceship : thing {};

declare_method(collide, string, thing)
define_method(collide, asteroid&, asteroid&) { return "Traverse"; }
define_method(collide, asteroid& a, bullet& b) { return collide(b, a); }
define_method(collide, asteroid&, spaceship&) { return "Boom"; }
define_method(collide, bullet&, asteroid&) { return "Hit"; }
define_method(collide, spaceship& s, asteroid& a) { return collide(a, s); }
define_method_fallback(collide) { return {}; }

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

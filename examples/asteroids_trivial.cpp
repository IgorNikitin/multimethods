#include <iostream>
#include <multimethods.h>
using namespace std;

struct asteroid {};
struct bullet {};
struct spaceship {};

declare_method(collide)
define_method(collide, asteroid&, asteroid&) { cout << "Traverse\n"; }
define_method(collide, asteroid& a, bullet& b) { collide(b, a); }
define_method(collide, asteroid&, spaceship&) { cout << "Boom\n"; }
define_method(collide, bullet&, asteroid&) { cout << "Hit\n"; }
define_method(collide, spaceship& s, asteroid& a) { collide(a, s); }
define_method_fallback(collide) {}

int main() {
   asteroid a;
   bullet b;
   spaceship s;

   collide(a, a); // 'Traverse'
   collide(a, b); // 'Hit'
   collide(b, a); // 'Hit'
   collide(s, a); // 'Boom'
   collide(a, s); // 'Boom'
   collide(b, b); // fallback
}

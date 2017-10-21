#include <iostream>
#include "multimethods.h"
using namespace std;

struct asteroid {};
struct spaceship : multimethods::unknown {};
struct spaceship_big : spaceship {};

declare_method(collide)
define_method(collide, asteroid&, spaceship_big&) { cout << "Big boom!\n"; }
define_method(collide, asteroid&, spaceship&) { cout << "Boom!\n"; }
define_method(collide, spaceship&, const spaceship& s) { cout << "Knock, knock.\n"; }

declare_method(join, string)
define_method(join, int x, int y, int z) { return to_string(x) + to_string(y) + to_string(z); }
define_method(join, string s1, string s2) { return s1 + s2; }
define_method_fallback(join) { return "Fallback."; }

declare_method(mm_abs, int)
define_method(mm_abs, int n) { if(n > 0) skip_method; return -n; }
define_method(mm_abs, int n) { return n; }

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
   cout << join(a, s1) << endl; // Fallback.

   cout << mm_abs(-10) << endl; // 10
   cout << mm_abs(10) << endl; // 10
}

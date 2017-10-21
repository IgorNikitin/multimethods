#include <iostream>
#include <multimethods.h>
using namespace std;

struct asteroid {};
struct spaceship {};

declare_method(collide)
define_method(collide, asteroid&, asteroid&) { cout << "A - A\n"; }
define_method(collide, asteroid&, spaceship&) { cout << "A - S\n"; }
define_method(collide, spaceship&, asteroid&) { cout << "S - A\n"; }
define_method(collide, spaceship&, spaceship&) { cout << "S - S\n"; }

int main() {
   asteroid a;
   spaceship s;

   collide(a, s);
   collide(a, a);
   collide(s, a);
   collide(s, s);
}

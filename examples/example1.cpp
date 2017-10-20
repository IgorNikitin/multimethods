#include <iostream>
#include "multimethods.h"
using namespace std;

struct asteroid {};
struct spaceship : multimethods::base {};
struct big_spaceship : spaceship {};

defgeneric(collide)
defmethod(collide, asteroid&, spaceship&)        { cout << "BOOM!!!" << endl; }
defmethod(collide, spaceship&, const spaceship&) { cout << "Knock, knock." << endl; }
defmethod(collide, int x, int y, int z)          { cout << x + y + z << endl; }
defmethod(collide, string s, big_spaceship&)     { cout << s << endl; }

int main() {
   asteroid a;
   spaceship s1, s2;
   big_spaceship bs;

   collide(a, s1);
   collide(a, bs);
   collide(s2, s1);
   collide(1, 2, 3);
   collide(string("Hello!"), static_cast<spaceship&>(bs));
}

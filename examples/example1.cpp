#include <iostream>
#include "multimethods.h"
using namespace std;

struct asteroid {};

struct spaceship : multimethods::unknown {
    virtual bool is_big() const { return false; }
};

struct spaceship_big : spaceship {
    bool is_big() const override { return true; }
};

declare_method(collide)
define_method(collide, asteroid&, spaceship&)            { cout << "BOOM!" << endl; }
define_method(collide, spaceship&, const spaceship& s)   { cout << "Knock, knock." << endl; if(s.is_big()) skip_method; }
define_method(collide, spaceship&, const spaceship_big&) { cout << "KNOCK, KNOCK." << endl; }
define_method(collide, int x, int y, int z)              { cout << x + y + z << endl; }
define_method(collide, string s, spaceship_big&)         { cout << s << endl; }

int main() {
   asteroid a;
   spaceship s1, s2;
   spaceship_big bs;

   collide(a, s1);
   collide(s2, bs);
   collide(1, 2, 3);
   collide("Hello."s, bs);
}

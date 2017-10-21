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

declare_method(collide, void)
define_method(collide, asteroid&, spaceship&)            { cout << "BOOM!" << endl; }
define_method(collide, spaceship&, const spaceship& s)   { cout << "Knock, knock." << endl; if(s.is_big()) skip_method; }
define_method(collide, spaceship&, const spaceship_big&) { cout << "KNOCK, KNOCK." << endl; }

declare_method(concat, string)
define_method(concat, int x, int y, int z)  { return to_string(x + y + z); }
define_method(concat, string s1, string s2) { return s1 + s2; }

int main() {
   asteroid a;
   spaceship s1, s2;
   spaceship_big bs;

   collide(a, s1);
   collide(s2, bs);

   cout << concat(1, 2, 3) << endl;
   cout << concat("Hello,"s, " world."s) << endl;
}

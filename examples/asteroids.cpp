#include <multimethods.h>
#include <iostream>
using namespace std;

struct thing { virtual ~thing(){} };
struct asteroid : thing {};
struct bullet : thing {};
struct ship : thing {};
struct saucer : thing {};

declare_method(collide, string, thing);
define_method(collide, ship&, thing&) { return "kaboom!"; }
define_method(collide, thing& a, ship& b) { return collide(b, a); }
define_method(collide, asteroid&, asteroid&) { return "traverse"; }
define_method_fallback(collide) { return {}; }

int main() {
  ship player;
  asteroid as;
  saucer small;
  bullet b;

  cout << collide(player, as) << endl; // kaboom!
  cout << collide(as, as) << endl; // traverse
}

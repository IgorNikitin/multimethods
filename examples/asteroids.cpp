#include <multimethods.h>
#include <iostream>
using namespace std;
using namespace multimethods;

struct asteroid : unknown {};
struct bullet : unknown {};
struct ship : unknown {};
struct saucer : unknown {};

declare_method(collide, string);
define_method(collide, ship&, unknown&) { return "kaboom!"; }
define_method(collide, unknown& a, ship& b) { return collide(b, a); }
define_method(collide, asteroid&, asteroid&) { return "traverse"; }

int main() {
  ship player;
  asteroid as;
  saucer small;
  bullet b;

  cout << collide(player, as) << endl; // kaboom!
  cout << collide(as, as) << endl; // traverse
}

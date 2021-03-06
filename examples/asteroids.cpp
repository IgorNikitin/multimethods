#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing { virtual ~thing() {} };
struct asteroid : thing {};
struct bullet : thing {};
struct spaceship : thing {};

void multi_method(collide, thing&, thing&)
    match(asteroid&, asteroid&) { cout << "Traverse\n"; }
    match(asteroid&, bullet&) { cout << "Hit\n"; }
    match(asteroid&, spaceship&) { cout << "Boom\n"; }
    match(thing& t, asteroid& a) { collide(a, t); }
    fallback {}
end_method

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

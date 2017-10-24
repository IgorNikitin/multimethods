#include <iostream>
#include <multimethods.h>
using namespace std;

struct asteroid {};
struct bullet {};
struct spaceship {};

void multi_method(collide, any, any)
    match(asteroid&, asteroid&) { cout << "Traverse\n"; }
    match(asteroid&, bullet&) { cout << "Hit\n"; }
    match(asteroid&, spaceship&) { cout << "Boom\n"; }
    match(bullet& b, asteroid& a) { collide(a, b); }
    match(spaceship& s, asteroid& a) { collide(a, s); }
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

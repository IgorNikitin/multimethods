#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing { virtual ~thing() {} };
struct asteroid : thing {};

void multi_method(collide, const thing&)
    match(const thing&) { cout << "base const\n"; }
    match(thing&) { cout << "base\n"; }
    match(const asteroid&) { cout << "derived const\n"; next_method; }
    match(asteroid&) { cout << "derived\n"; next_method; }
end_method

int main() {
    asteroid a;
    collide(a);
    cout << "-------------\n";
    collide((const thing&) a);
}

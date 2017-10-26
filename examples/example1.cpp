#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing { virtual ~thing() {} };
struct lizard : thing {};
struct paper : thing {};
struct rock : thing {};
struct scissors : thing {};
struct spock : thing {};

void multi_method(rpsls, const thing&, const thing&)
    match(const lizard&, const paper&)      { cout << "Lizard eats Paper\n"; }
    match(const lizard&, const spock&)      { cout << "Lizard poisons Spock\n"; }
    match(const lizard&, const lizard&)     { cout << "Try again\n"; }
    match(const rock&, const lizard&)       { cout << "Rock crushes Lizard\n"; }
    match(const rock&, const scissors&)     { cout << "Rock crushes Scissors\n"; }
    match(const rock&, const rock&)         { cout << "Try again\n"; }
    match(const paper&, const rock&)        { cout << "Paper covers Rock\n"; }
    match(const paper&, const spock&)       { cout << "Paper disproves Spock\n"; }
    match(const paper&, const paper&)       { cout << "Try again\n"; }
    match(const scissors&, const lizard&)   { cout << "Scissors decapitates Lizard\n"; }
    match(const scissors&, const paper&)    { cout << "Scissors cuts Paper\n"; }
    match(const scissors&, const scissors&) { cout << "Try again\n"; }
    match(const spock&, const rock&)        { cout << "Spock vaporizes Rock\n"; }
    match(const spock&, const scissors&)    { cout << "Spock smashes Scissors\n"; }
    match(const spock&, const spock&)       { cout << "Try again\n"; }
    match(const thing& t1, const thing& t2) { rpsls(t2, t1); }
end_method

struct shape { virtual ~shape() {} };
struct rectangle : shape {};
struct triangle : shape {};

shape multi_method(overlap, const shape&, const shape&)
    match(const rectangle&, const rectangle&) { return rectangle(); }
end_method

int main() {
    rpsls(paper(), scissors());
    rpsls(lizard(), rock());
    rpsls(spock(), spock());
    // --------------
    overlap(rectangle(), rectangle());

    try {
        overlap(rectangle(), triangle());
    } catch(multimethods::not_implemented& e) {
        cout << e.what() << endl; // overlap: not_implemented
    }
}

#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing { virtual ~thing() {} };
struct lizard : thing {};
struct paper : thing {};
struct rock : thing {};
struct scissors : thing {};
struct spock : thing {};

void multi_method(rpsls, thing, thing)
    match(lizard, paper)        { cout << "Lizard eats Paper\n"; }
    match(lizard, spock)        { cout << "Lizard poisons Spock\n"; }
    match(lizard, lizard)       { cout << "Try again\n"; }
    match(rock, lizard)         { cout << "Rock crushes Lizard\n"; }
    match(rock, scissors)       { cout << "Rock crushes Scissors\n"; }
    match(rock, rock)           { cout << "Try again\n"; }
    match(paper, rock)          { cout << "Paper covers Rock\n"; }
    match(paper, spock)         { cout << "Paper disproves Spock\n"; }
    match(paper, paper)         { cout << "Try again\n"; }
    match(scissors, lizard)     { cout << "Scissors decapitates Lizard\n"; }
    match(scissors, paper)      { cout << "Scissors cuts Paper\n"; }
    match(scissors, scissors)   { cout << "Try again\n"; }
    match(spock, rock)          { cout << "Spock vaporizes Rock\n"; }
    match(spock, scissors)      { cout << "Spock smashes Scissors\n"; }
    match(spock, spock)         { cout << "Try again\n"; }
    match(thing& t1, thing& t2) { rpsls(t2, t1); }
end_method

struct shape { virtual ~shape() {} };
struct rectangle : shape {};
struct triangle : shape {};

shape multi_method(overlap, shape, shape)
    match(rectangle, rectangle) { return rectangle(); }
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

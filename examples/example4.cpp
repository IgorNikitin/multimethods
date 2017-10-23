#include <multimethods.h>
#include <iostream>
using namespace multimethods;
using namespace std;

struct role : unknown {};
struct manager : role {};
struct ceo : role {};

struct expense : unknown {};
struct plane : expense {};
struct cab : expense {};

struct reason : unknown {};
struct business : reason {};
struct comfort : reason {};

define_method(approve, bool)
    match(expense&, ceo&, reason&) { return true; }
    match(cab&, manager&, business&) { return true; }
    fallback { return false; }
end_method

int main() {
    cout << approve(plane(), ceo(), comfort()) << endl; // true
    cout << approve(cab(), ceo(), business()) << endl; // true

    cout << approve(cab(), manager(), business()) << endl; // true
    cout << approve(cab(), manager(), comfort()) << endl; // false
    cout << approve(plane(), manager(), business()) << endl; // false
}

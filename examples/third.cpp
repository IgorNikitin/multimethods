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
    match(const expense&, const ceo&, const reason&) { return true; }
    match(const cab&, const manager&, const business&) { return true; }
    fallback { return false; }
end_method

int main() {
    cout << boolalpha;

    cout << approve(plane(), ceo(), comfort()) << endl; // true
    cout << approve(cab(), ceo(), business()) << endl; // true

    cout << approve(cab(), manager(), business()) << endl; // true
    cout << approve(cab(), manager(), comfort()) << endl; // false
    cout << approve(plane(), manager(), comfort()) << endl; // false
}

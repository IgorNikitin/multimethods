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

declare_method(approve, bool)
define_method(approve, const expense&, const ceo&, const reason&) { return true; }
define_method(approve, const cab&, const manager&, const business&) { return true; }
define_method_fallback(approve) { return false; }

int main() {
    cout << boolalpha;

    cout << approve(plane(), ceo(), comfort()) << endl; // true
    cout << approve(cab(), ceo(), business()) << endl; // true

    cout << approve(cab(), manager(), business()) << endl; // true
    cout << approve(cab(), manager(), comfort()) << endl; // false

  return 0;
}

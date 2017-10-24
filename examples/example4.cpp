#include <multimethods.h>
#include <iostream>
using namespace std;

struct role { virtual ~role() {} };
struct manager : role {};
struct ceo : role {};

struct expense { virtual ~expense() {} };
struct plane : expense {};
struct cab : expense {};

struct reason { virtual ~reason() {} };
struct business : reason {};
struct comfort : reason {};

bool multi_method(approve, const expense&, const role&, const reason&)
    match(const expense&, const ceo&, const reason&) { return true; }
    match(const cab&, const manager&, const business&) { return true; }
    fallback { return false; }
end_method

int main() {
    cout << approve(plane(), ceo(), comfort()) << endl; // true
    cout << approve(cab(), ceo(), business()) << endl; // true

    cout << approve(cab(), manager(), business()) << endl; // true
    cout << approve(cab(), manager(), comfort()) << endl; // false
    cout << approve(plane(), manager(), business()) << endl; // false
}

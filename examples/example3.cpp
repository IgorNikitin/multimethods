#include <iostream>
#include <multimethods.h>
using namespace std;

struct base1 {
    virtual string foo() const { return "base1"; }
    int m1_;
};

struct base2 : virtual base1 {
    MM_CLASS()
    string foo() const { return "base2"; }
    virtual ~base2(){}
    int m2;
};

struct base3 {
    virtual void foo2() {};
    int m3_;
};

struct derived1 : virtual base1, virtual base2, base3 {
    MM_CLASS(base2)
    string foo() const { return name_; }
    void foo2() {}
    string name_ = "derived1";
};

struct derived2 : virtual base1, virtual base3, base2 {
    MM_CLASS(base2)
    string foo() const { return name_; }
    void foo2() {}
    string name_ = "derived2";
};

void multi_method(print, const base2&)
    match(const base2& b) { cout << b.foo() << " via base2" << endl; }
    match(const derived1& d) { cout << d.foo() << endl; }
end_method

int main() {
    base2 b2;
    derived1 d1;
    derived2 d2;

    print(b2); // base2 via base2
    print(d1); // derived1
    print(dynamic_cast<base2&>(d1)); // derived1
    print(dynamic_cast<base2&>(d2)); // derived2 via base2
}

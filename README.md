# multimethods
Multimethods for C++17

Особливості:

* можливість розділення оголошення методу та його реализацій, вони можуть бути у окремих .cpp-файлах;
* підтримка всіх типів, а не тільки поліморфних;
* довільна кількість параметрів;
* можливість отримати результат;
* можливість обирати методи, використовуючи передані аргументи, наприклад:
```C++
    declare_method(abs, int)
    define_method(abs, int n) { if(n > 0) skip_method; return -n; }
    define_method(abs, int n) { return n; }
```
* мінімум кода для використання;
* бібліотека дуже мала (кількасот рядків) і міститься у одному заголовному файлі.

Приклад використання:

```C++
#include <iostream>
#include "multimethods.h"
using namespace std;

struct asteroid {};
struct spaceship : multimethods::unknown {};
struct spaceship_big : spaceship {};

declare_method(collide, void)
define_method(collide, asteroid&, spaceship_big&) { cout << "Big boom!.\n"; }
define_method(collide, asteroid&, spaceship&) { cout << "Boom!\n"; }
define_method(collide, spaceship&, const spaceship& s) { cout << "Knock, knock.\n"; }

declare_method(concat, string)
define_method(concat) { return ""s; }
define_method(concat, int x, int y, int z) { return to_string(x) + to_string(y) + to_string(z); }
define_method(concat, string s1, string s2) { return s1 + s2; }

declare_method(mm_abs, int)
define_method(mm_abs, int n) { if(n > 0) skip_method; return -n; }
define_method(mm_abs, int n) { return n; }

int main() {
   asteroid a;
   spaceship s1, s2;
   spaceship_big bs;

   collide(a, s1);
   collide(a, bs);
   collide(s2, bs);

   cout << concat() << endl;
   cout << concat(1, 2, 3) << endl;
   cout << concat("Hello,"s, " world."s) << endl;

   cout << mm_abs( -10 ) << endl;
   cout << mm_abs( 10 ) << endl;
}
```

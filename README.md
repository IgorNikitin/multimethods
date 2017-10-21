# multimethods
Multimethods for C++17

(якщо хтось може допомогти перекласти англійською, буду вдячний)

## Особливості

* Можливість розділення оголошення методу та його реализацій, вони можуть бути у окремих .cpp-файлах;
* підтримка всіх типів, а не тільки поліморфних;
* довільна кількість параметрів у реалізаціях;
* можливість отримати результат, а не просто викликати функцію;
* можливість обирати методи, використовуючи передані аргументи, наприклад:
```C++
    declare_method(abs, int)
    define_method(abs, int n) { if(n > 0) skip_method; return -n; }
    define_method(abs, int n) { return n; }
```
* мінімум кода для використання;
* можна вказати fallback-функцію, що буде викликатися, якщо не знайдена відповідна реалізація;
* бібліотека дуже мала (кількасот рядків) і міститься у одному заголовному файлі.

## Важлива інформація

* Реалізація, що розміщена вище по коду, має більший пріорітет, тому, якщо використовується диспетчеризація за базовими класами, то спочатку треба вказати реалізацію для успадкованих класів;
* якщо треба диспетчеризація за базовим класом, то наслідуйте його від multimethods::unknown (див. examples/example3.cpp);
* для максимальної швидкодії краще обрати інший інструмент, ця бібліотека використовує dynamic_cast для пошуку відповідної реалізації, хоча у більшості випадків її швидкодії буде цілком достатньо.
* чим більше реалізацій у одного з методів, тим більше часу може знадобиться для його виклику. Наприклад, для такого коду:
```C++
    struct base : public multimethods::unknown {};
    struct asteroid : public base {};
    struct spaceship : public base {};

    declare_method(collide)
    define_method(collide, asteroid&, asteroid&) {}
    define_method(collide, asteroid&, spaceship&) {}
    define_method(collide, spaceship&, asteroid&) {}
    define_method(collide, spaceship&, spaceship&) {}
    ...
    spaceship s1, s2;
    for(int i = 0; i < 1000000; ++i)
       collide(static_cast<base&>(s1), static_cast<base&>(s2));
```
де використовується поліморфізм, та обран найневдаліший варіант (останній по пріоритету), час виконання на 2600K 3.4GHz складає близько 0.15сек (150нс на виклик). Без поліморфізму і з першим варіантом час скадає 0.01сек (10нс на виклик). Для порівняння виклик простої функції склав 0.0015сек (1.5нс на виклик). Ці числа не є фіксованими і можуть бути змінені з часом через оптимізації, або, навпаки, через додавання нової функціональності.

## Приклад використання

```C++
#include <iostream>
#include "multimethods.h"
using namespace std;

struct asteroid {};
struct spaceship : multimethods::unknown {};
struct spaceship_big : spaceship {};

declare_method(collide)
define_method(collide, asteroid&, spaceship_big&) { cout << "Big boom!\n"; }
define_method(collide, asteroid&, spaceship&) { cout << "Boom!\n"; }
define_method(collide, spaceship&, const spaceship& s) { cout << "Knock, knock.\n"; }

declare_method(join, string)
define_method(join, int x, int y, int z) { return to_string(x) + to_string(y) + to_string(z); }
define_method(join, string s1, string s2) { return s1 + s2; }
define_method_fallback(join) { return "Fallback."; }

declare_method(mm_abs, int)
define_method(mm_abs, int n) { if(n > 0) skip_method; return -n; }
define_method(mm_abs, int n) { return n; }

int main() {
   asteroid a;
   spaceship s1, s2;
   spaceship_big bs;

   collide(a, s1); // Boom!
   collide(a, bs); // Big boom!
   collide(s2, bs); // Knock, knock.

   try {
       collide(a, true);
   } catch(multimethods::not_implemented& e) {
       cout << e.what() << endl; // collide: not_implemented
   }

   cout << join(1, 2, 3) << endl; // 123
   cout << join("Hello,"s, " world."s) << endl; // Hello, world.
   cout << join(a, s1) << endl; // Fallback.

   cout << mm_abs(-10) << endl; // 10
   cout << mm_abs(10) << endl; // 10
}
```

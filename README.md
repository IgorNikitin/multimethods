# multimethods
Multimethods for C++17

## Особливості

* Підтримка неполіморфних типів;
* бібліотека сортує реалізації, використувачи спадкування та константність, спочатку обираються успадковані класи, якщо ж типи співпадають для всіх параметрів, то реалізація без const (типи перевіряються зліва-направо як для спадкування, так і для наявності const в них) має вищій пріоритет, наприклад:
```C++
    void multi_method(collide, const thing&, const thing&)
        match(const thing&, spaceship&) { cout << "base const\n"; }
        match(thing&, spaceship&) { cout << "base\n"; }
        match(const asteroid&, spaceship&) { cout << "derived const\n"; next_method; }
        match(asteroid&, spaceship&) { cout << "derived\n"; next_method; }
    end_method
    ...
    asteroid a;
    spaceship s;
    collide(a, s);
    cout << "-------------\n";
    collide((const thing&) a, s);
``` 
виведе на екран:
```
    derived
    derived const
    base
    -------------
    derived const
    base const
```
* можливість обирати методи, використовуючи передані аргументи, наприклад:
```C++
    int multi_method(my_abs, int)
        match(int n) { if(n > 0) next_method; return -n; }
        match(int n) { if(n < 0) next_method; return n; }
    end_method
```
* мінімум кода для використання;
* можна вказати fallback-функцію, що буде викликатися, якщо не знайдена відповідна реалізація;
```C++
    void multi_method(dump, any)
        match(int n) { cout << n << endl; }
        match(string s) { cout << s << endl; }
        fallback { cout << "unknown\n"; }
    end_method
```
* бібліотека дуже мала (кількасот рядків) і міститься у одному заголовному файлі.

## Важлива інформація (обмеження)

* Для максимальної швидкодії краще обрати інший інструмент, ця бібліотека використовує dynamic_cast для пошуку відповідної реалізації, хоча у більшості випадків її швидкодії буде цілком достатньо;
* чим більше реалізацій у одного з методів, тим більше часу може знадобиться для його виклику. Наприклад, для такого коду:
```C++
    struct thing { virtial ~thing() {} };
    struct asteroid : thing {};
    struct spaceship : thing {};

    void multi_method(collide, thing&, thing&)
        match(asteroid&, asteroid&) {}
        match(asteroid&, spaceship&) {}
        match(spaceship&, asteroid&) {}
        match(spaceship&, spaceship&) {}
    end_method
    ...
    spaceship s1, s2;
    for(int i = 0; i < 1000000000; ++i)
       collide(s1, s2);
```
де використовується поліморфізм, час виконання на 2600K 3.4GHz складає близько 88.6сек (88.6нс на виклик). Без поліморфізму час складає 6.78сек (6.78нс на виклик). Ці числа не є фіксованими і можуть бути змінені з часом через оптимізації, або, навпаки, через додавання нової функціональності.

## Приклад використання

```C++
#include <iostream>
#include <multimethods.h>
using namespace std;

struct vehicle { virtual ~vehicle(){} };
struct car : vehicle {};
struct inspector { virtual ~inspector(){} };
struct state_inspector : inspector {};

void multi_method(inspect, const vehicle&, const inspector&)
    match(const car&, const inspector&) { cout << "Inspect seat belts.\n"; next_method; }
    match(const car&, const state_inspector&) { cout << "Check insurance.\n"; next_method; }
    match(const vehicle&, const inspector&) { cout << "Inspect vehicle.\n"; }
end_method

string multi_method(join, any, any)
    match(int x, int y) { return to_string(x) + to_string(y); }
    match(string s1, string s2) { return s1 + s2; }
end_method

int main() {
    inspect(car(), state_inspector());  // Check insurance. Inspect seat belts. Inspect vehicle.

    cout << join(1, 2) << endl; // 123
    cout << join("Hello,"s, " world."s) << endl; // Hello, world.

    try {
        join(true, false);
    } catch(multimethods::not_implemented& e) {
        cout << e.what() << endl; // join: not_implemented
    }
}
```

# multimethods
Multimethods for C++17

## Особливості

* Підтримка неполіморфних типів;
* довільна кількість параметрів у реалізаціях;
* бібліотека сортує реалізації, використувачи спадкування та константність, спочатку обираються успадковані класи, якщо ж типи співпадають для всіх параметрів, то реалізація без const (типи перевіряються зліва-направо як для спадкування, так і для наявності const в них) має вищій пріоритет, наприклад:
```C++
    void multi_method(collide)
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
    int multi_method(my_abs)
        match(int n) { if(n > 0) next_method; return -n; }
        match(int n) { if(n < 0) next_method; return n; }
    end_method
```
* мінімум кода для використання;
* можна вказати fallback-функцію, що буде викликатися, якщо не знайдена відповідна реалізація;
```C++
    void multi_method(dump)
        match(int n) { cout << n << endl; }
        match(string s) { cout << s << endl; }
        fallback { cout << "unknown\n"; }
    end_method
```
* бібліотека дуже мала (кількасот рядків) і міститься у одному заголовному файлі.

## Важлива інформація (обмеження)

* Якщо треба диспетчеризація за базовим класом, то або вкажіть його другим аргументом у multi_method, або наслідуйте його (їх) від multimethods::unknown, див. [example3.cpp](examples/example3.cpp);
* для максимальної швидкодії краще обрати інший інструмент, ця бібліотека використовує dynamic_cast для пошуку відповідної реалізації, хоча у більшості випадків її швидкодії буде цілком достатньо;
* про неможливість виклику функції з неправильними аргументами можна буде дізнатися тільки у рантаймі (виключення multimethods::not_implemented);
* чим більше реалізацій у одного з методів, тим більше часу може знадобиться для його виклику. Наприклад, для такого коду:
```C++
    using thing = multimethods::unknown;
    struct asteroid : thing {};
    struct spaceship : thing {};

    void multi_method(collide)
        match(asteroid&, asteroid&) {}
        match(asteroid&, spaceship&) {}
        match(spaceship&, asteroid&) {}
        match(spaceship&, spaceship&) {}
    end_method
    ...
    spaceship s1, s2;
    for(int i = 0; i < 1000000; ++i)
       collide(s1, s2);
```
де використовується поліморфізм, час виконання на 2600K 3.4GHz складає близько 0.17сек (170нс на виклик). Без поліморфізму час складає 0.012сек (12нс на виклик). Ці числа не є фіксованими і можуть бути змінені з часом через оптимізації, або, навпаки, через додавання нової функціональності.

## Приклад використання

```C++
#include <iostream>
#include <multimethods.h>
using namespace std;

struct vehicle : multimethods::unknown {};
struct car : vehicle {};
struct inspector : multimethods::unknown {};
struct state_inspector : inspector {};

void multi_method(inspect)
    match(vehicle&, inspector&) { cout << "Inspect vehicle.\n"; }
    match(car&, inspector&) { cout << "Inspect seat belts.\n"; next_method; }
    match(car&, state_inspector&) { cout << "Check insurance.\n"; next_method; }
end_method

string multi_method(join)
    match(int x, int y, int z) { return to_string(x) + to_string(y) + to_string(z); }
    match(string s1, string s2) { return s1 + s2; }
    fallback { return "fallback"; }
end_method

int main() {
    inspect(car(), state_inspector());  // Check insurance. Inspect seat belts. Inspect vehicle.

    try {
        inspect(car(), true);
    } catch(multimethods::not_implemented& e) {
        cout << e.what() << endl; // inspect: not_implemented
    }

    cout << join(1, 2, 3) << endl; // 123
    cout << join("Hello,"s, " world."s) << endl; // Hello, world.
    cout << join(false) << endl; // fallback
}
```

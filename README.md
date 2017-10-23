# multimethods
Multimethods for C++17

## Особливості

* Підтримка неполіморфних типів;
* довільна кількість параметрів у реалізаціях;
* можливість отримати результат від функції;
* бібліотека сортує реалізації, використувачи спадкування та константність, спочатку обираються успадковані класи, якщо ж типи співпадають для всіх параметрів, то реалізація без const (типи перевіряються зліва-направо як для спадкування, так і для наявності const в них) має вищій пріоритет, наприклад:
```C++
    define_method(collide, void, thing)
        match(const thing&, spaceship&) { cout << "base const\n"; }
        match(thing&, spaceship&) { cout << "base\n"; }
        match(const asteroid&, spaceship&) { cout << "derived const\n"; next_method; }
        match(asteroid&, spaceship&) { cout << "derived\n"; next_method; }
    end_method
    ...
    asteroid a;
    spaceship s;
    collide(static_cast<thing&>(a), s);
    cout << "-------------\n";
    collide(static_cast<const thing&>(a), s);
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
    define_method(my_abs, int)
        match(int n) { if(n > 0) next_method; return -n; }
        match(int n) { if(n < 0) next_method; return n; }
    end_method
```
* мінімум кода для використання;
* можна вказати fallback-функцію, що буде викликатися, якщо не знайдена відповідна реалізація;
```C++
    define_method(dump)
        match(int n) { cout << n << endl; }
        match(string s) { cout << s << endl; }
        fallback { cout << "unknown\n"; }
    end_method
```
* бібліотека дуже мала (кількасот рядків) і міститься у одному заголовному файлі.

## Важлива інформація (обмеження)

* Якщо треба диспетчеризація за базовим класом, то або вкажіть його третім аргументом у declare_methods, або наслідуйте його (їх) від multimethods::unknown, див. [example3.cpp](examples/example3.cpp);
* для максимальної швидкодії краще обрати інший інструмент, ця бібліотека використовує dynamic_cast для пошуку відповідної реалізації, хоча у більшості випадків її швидкодії буде цілком достатньо.
* чим більше реалізацій у одного з методів, тим більше часу може знадобиться для його виклику. Наприклад, для такого коду:
```C++
    struct base : multimethods::unknown {};
    struct asteroid : base {};
    struct spaceship : base {};

    define_method(collide)
        match(asteroid&, asteroid&) {}
        match(asteroid&, spaceship&) {}
        match(spaceship&, asteroid&) {}
        match(spaceship&, spaceship&) {}
    end_method
    ...
    spaceship s1, s2;
    for(int i = 0; i < 1000000; ++i)
       collide(static_cast<base&>(s1), static_cast<base&>(s2));
```
де використовується поліморфізм, та обран найневдаліший варіант (останній по пріоритету), час виконання на 2600K 3.4GHz складає близько 0.17сек (170нс на виклик). Без поліморфізму і з першим варіантом час складає 0.012сек (12нс на виклик). Ці числа не є фіксованими і можуть бути змінені з часом через оптимізації, або, навпаки, через додавання нової функціональності.

## Приклад використання

```C++
#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing : multimethods::unknown {};
struct asteroid : thing {};
struct bullet : thing {};
struct spaceship : thing {};

define_method(collide)
    match(asteroid&, asteroid&) { cout << "traverse\n"; }
    match(asteroid&, bullet&) { cout << "hit\n"; }
    match(asteroid&, spaceship&) { cout << "boom\n"; }
    match(thing& t, asteroid& a) { collide(a, t); }
    fallback {}
end_method

define_method(join, string)
    match(int x, int y, int z) { return to_string(x) + to_string(y) + to_string(z); }
    match(string s1, string s2) { return s1 + s2; }
    match() { return {}; }
    fallback { return "fallback"; }
end_method

int main() {
    collide(asteroid(), spaceship()); // boom
    collide(bullet(), asteroid()); // hit

    try {
        collide(asteroid(), true);
    } catch(multimethods::not_implemented& e) {
        cout << e.what() << endl; // collide: not_implemented
    }

    cout << join(1, 2, 3) << endl; // 123
    cout << join("Hello,"s, " world."s) << endl; // Hello, world.
    cout << join() << endl; //
    cout << join(false) << endl; // fallback
}
```

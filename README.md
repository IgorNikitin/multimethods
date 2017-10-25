# multimethods
Multimethods for C++17

## Особливості

* Бібліотека автоматичо сортує реалізації, використувачи спадкування та константність (типи порівнюються зліва-направо), наприклад:
```C++
    void multi_method(collide, const thing&)
        match(const thing&) { cout << "base const\n"; }
        match(thing&) { cout << "base\n"; }
        match(const asteroid&) { cout << "derived const\n"; next_method; }
        match(asteroid&) { cout << "derived\n"; next_method; }
    end_method
    ...
    asteroid a;
    collide(a);
    cout << "-------------\n";
    collide((const thing&) a);
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
    void multi_method(dump, thing&)
        match(asteroid&) { cout << "asteroid\n"; }
        fallback { cout << "unknown\n"; }
    end_method
```
* бібліотека мала (близько тисячі рядків) і міститься у одному заголовному файлі.

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
    asteroid a;
    spaceship s;
    
    for(int i = 0; i < 250000000; ++i) {
        collide(a, a);
        collide(a, s);
        collide(s, a);
        collide(s, s);
    }
```
час виконання на 2600K 3.4GHz з компілятором clang++ склав близько 57.5сек (57.5нс на виклик). Це число не є фіксованими і може бути змінено з часом через оптимізації, або, навпаки, через додавання нової функціональності.

## Приклад використання

```C++
#include <iostream>
#include <multimethods.h>
using namespace std;

struct thing { virtual ~thing() {} };
struct lizard : thing {};
struct paper : thing {};
struct rock : thing {};
struct scissors : thing {};
struct spock : thing {};

void multi_method(rpsls, const thing&, const thing&)
    match(const lizard&, const paper&)      { cout << "Lizard eats Paper\n"; }
    match(const lizard&, const spock&)      { cout << "Lizard poisons Spock\n"; }
    match(const lizard&, const lizard&)     { cout << "Try again\n"; }
    match(const rock&, const lizard&)       { cout << "Rock crushes Lizard\n"; }
    match(const rock&, const scissors&)     { cout << "Rock crushes Scissors\n"; }
    match(const rock&, const rock&)         { cout << "Try again\n"; }
    match(const paper&, const rock&)        { cout << "Paper covers Rock\n"; }
    match(const paper&, const spock&)       { cout << "Paper disproves Spock\n"; }
    match(const paper&, const paper&)       { cout << "Try again\n"; }
    match(const scissors&, const lizard&)   { cout << "Scissors decapitates Lizard\n"; }
    match(const scissors&, const paper&)    { cout << "Scissors cuts Paper\n"; }
    match(const scissors&, const scissors&) { cout << "Try again\n"; }
    match(const spock&, const rock&)        { cout << "Spock vaporizes Rock\n"; }
    match(const spock&, const scissors&)    { cout << "Spock smashes Scissors\n"; }
    match(const spock&, const spock&)       { cout << "Try again\n"; }
    match(const thing& t1, const thing& t2) { rpsls(t2, t1); }
end_method

struct shape { virtual ~shape() {} };
struct rectangle : shape {};
struct triangle : shape {};

shape multi_method(overlap, shape, shape)
    match(rectangle, rectangle) { return rectangle(); }
end_method

int main() {
    rpsls(paper(), scissors());
    rpsls(lizard(), rock());
    rpsls(spock(), spock());
    // --------------
    overlap(rectangle(), rectangle());

    try {
        overlap(rectangle(), triangle());
    } catch(multimethods::not_implemented& e) {
        cout << e.what() << endl; // overlap: not_implemented
    }
}
```

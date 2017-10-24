# multimethods
Multimethods for C++17

## Особливості

* Підтримка неполіморфних типів;
* бібліотека автоматичо сортує реалізації, використувачи спадкування, тип параметру та константність (типи порівнюються зліва-направо), наприклад:
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
де використовується поліморфізм, час виконання на 2600K 3.4GHz з компілятором clang++ склав близько 86.8сек (86.8нс на виклик). Без поліморфізму час склав 6.78сек (6.78нс на виклик). Ці числа не є фіксованими і можуть бути змінені з часом через оптимізації, або, навпаки, через додавання нової функціональності.

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

void multi_method(rpsls, thing, thing)
    match(lizard, paper)        { cout << "Lizard eats Paper\n"; }
    match(lizard, spock)        { cout << "Lizard poisons Spock\n"; }
    match(lizard, lizard)       { cout << "Try again\n"; }
    match(rock, lizard)         { cout << "Rock crushes Lizard\n"; }
    match(rock, scissors)       { cout << "Rock crushes Scissors\n"; }
    match(rock, rock)           { cout << "Try again\n"; }
    match(paper, rock)          { cout << "Paper covers Rock\n"; }
    match(paper, spock)         { cout << "Paper disproves Spock\n"; }
    match(paper, paper)         { cout << "Try again\n"; }
    match(scissors, lizard)     { cout << "Scissors decapitates Lizard\n"; }
    match(scissors, paper)      { cout << "Scissors cuts Paper\n"; }
    match(scissors, scissors)   { cout << "Try again\n"; }
    match(spock, rock)          { cout << "Spock vaporizes Rock\n"; }
    match(spock, scissors)      { cout << "Spock smashes Scissors\n"; }
    match(spock, spock)         { cout << "Try again\n"; }
    match(thing& t1, thing& t2) { rpsls(t2, t1); }
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

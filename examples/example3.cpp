#include <iostream>
#include <multimethods.h>
using namespace std;

struct base1 : multimethods::unknown {};
struct derived1 : base1 {};

struct base2 { virtual ~base2(){} };
struct derived2 : base2 {};


/**********************************************************************************************/
// Defines a new multimethod and optionally specifies it's result type and base class
// for polymorphic types.
//
//   declare_method(collide)
//   declare_method(concat, string)
//   declare_method(concat, string, I_Unknown)
//
#define define_method1(name, ...) \
    using g_mm_b_ ## name = ::multimethods::detail::multimethod_parameters<__VA_ARGS__>::base; \
    using g_mm_r_ ## name = ::multimethods::detail::multimethod_parameters<__VA_ARGS__>::type; \
    \
    struct g_mm_f_ ## name { \
        static inline std::vector<::multimethods::detail::abstract_method<g_mm_r_ ## name, g_mm_b_ ## name>*> funcs_; \
        static inline g_mm_r_ ## name(*fallback_)() { nullptr }; \
    }; \
    \
    template<class... Args> inline \
    g_mm_r_ ## name name(Args&&... args) { \
        for( auto m : g_mm_f_ ## name ::funcs_ ) \
            try { \
                if(auto r = m->call(args...)) \
                    return ::multimethods::detail::method_result<g_mm_r_ ## name>::unwrap(r); \
            } catch(::multimethods::not_match&) { \
            } \
        return g_mm_f_ ## name ::fallback_ \
            ? (*g_mm_f_ ## name ::fallback_)() \
            : throw ::multimethods::not_implemented(#name ": not_implemented."); \
    } \
    \
    static bool MM_JOIN(_mm_init_, __LINE__) = []{ const std::tuple funcs { true


/**********************************************************************************************/
// Adds implementation of a multimethod.
//
//   declare_method(collide)
//   define_method(collide, asteroid&, spaceship&) { cout << "Boom!\n"; }
//
#define match ,+[]

#define end_method(name) \
    }; \
    g_mm_f_ ## name::funcs_ = ::multimethods::detail::sort_functions(funcs).sort<g_mm_r_ ## name, g_mm_b_ ## name>(); \
    return true; }();

define_method1(print1)
    match(base1&) { cout << "base1\n"; }
    match(derived1&) { cout << "derived1\n"; }
end_method(print1)

define_method1(print2, void, base2)
    match(base2&) { cout << "base2\n"; }
    match(derived2&) { cout << "derived2\n"; }
end_method(print1)

int main() {
    base1 b1;
    base2 b2;
    derived1 d1;
    derived2 d2;

    print1(); // base1
    print1(d1); // derived1

    print2(b2); // base2
    print2(d2); // derived2

    print1(static_cast<base1&>(d1)); // derived1
    print2(static_cast<base2&>(d2)); // derived2
}

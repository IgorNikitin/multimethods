/**********************************************************************************************/
// multimethods.h
//
// Igor Nikitin, 2017.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
/**********************************************************************************************/

#include <algorithm>
#include <optional>
#include <string>
#include <tuple>
#include <typeindex>
#include <vector>


/**********************************************************************************************/
// Defines a new multimethod and optionally specifies it's result type and base class
// for polymorphic types.
//
//   declare_method(collide)
//   declare_method(concat, string)
//   declare_method(concat, string, I_Unknown)
//
#define define_method(name, ...) \
    namespace mm_namespace_ ## name { \
        using mm_base_t     = ::multimethods::detail::multimethod_parameters<__VA_ARGS__>::base; \
        using mm_ret_type_t = ::multimethods::detail::multimethod_parameters<__VA_ARGS__>::type; \
        \
        static inline std::vector<::multimethods::detail::abstract_method<mm_ret_type_t, mm_base_t>*> g_funcs; \
        static inline mm_ret_type_t(*fallback_)() { nullptr }; \
    }; \
    \
    template<class... Args> inline \
    mm_namespace_ ## name::mm_ret_type_t name(Args&&... args) { \
        for( auto m : mm_namespace_ ## name ::g_funcs ) \
            try { \
                if(auto r = m->call(args...)) \
                    return ::multimethods::detail::method_result<mm_namespace_ ## name::mm_ret_type_t>::unwrap(r); \
            } catch(::multimethods::try_next&) { \
            } \
        return mm_namespace_ ## name ::fallback_ \
            ? (*mm_namespace_ ## name ::fallback_)() \
            : throw ::multimethods::not_implemented(#name ": not_implemented."); \
    } \
    \
    namespace mm_namespace_ ## name { \
        const bool g_init = [] { const std::tuple funcs { true

/**********************************************************************************************/
// Adds implementation of a multimethod.
//
//   declare_method(collide)
//   define_method(collide, asteroid&, spaceship&) { cout << "Boom!\n"; }
//
#define match(...) , +[](__VA_ARGS__) -> mm_ret_type_t

#define end_method \
            }; \
            ::multimethods::detail::sort_functions sorter(funcs); \
            g_funcs = sorter.sort<mm_ret_type_t, mm_base_t>(); \
            return true; \
        }(); \
    }

/**********************************************************************************************/
// Adds fallback handler for a multimethod.
//
//   define_method(collide)
//   fallback(collide) { cout << "All is fine.\n"; }
//
#define fallback , +[](::multimethods::detail::fallback_t) -> mm_ret_type_t

/**********************************************************************************************/
// Skip current method in runtime to search for more suitable implementation.
//
//   define_method(mm_abs, int n) { if(n > 0) skip_method; return -n; }
//
#define next_method throw ::multimethods::try_next();


/**********************************************************************************************/
namespace multimethods {

/**********************************************************************************************/
// Exception to notify about an unimplemented method.
//
struct not_implemented final : std::exception {
    std::string name_;
    not_implemented(const char* name) : name_(name) {}

    virtual const char* what() const noexcept { return name_.c_str(); }
};

/**********************************************************************************************/
// Exception to skip a method and try next one.
//
struct try_next final : std::exception {
    virtual const char* what() const noexcept { return "next_method"; }
};

/**********************************************************************************************/
// Base class to use with user's classes to dispatch over base classes.
//
// struct spaceship : multimethods::unknown {};
// struct spaceship_big : spaceship {};
// ...
// define_method(collide, asteroid&, spaceship&) { cout << "Boom!\n"; }
//
struct unknown {
    virtual ~unknown() {}
};


/**********************************************************************************************/
namespace detail {

/**********************************************************************************************/
using namespace std;

/**********************************************************************************************/
static inline const std::type_index g_dummy_type_index( typeid(int) );

/**********************************************************************************************/
struct fallback_t {};

/**********************************************************************************************/
// Class to store reference to an argument and cast it on call an implementation.
//
template<class B>
struct arg final {
    B* base_;
    bool const_;
    void* p_;
    const std::type_index type_;

    // Constructs from polymorphic value - we can try to cast it to the base class,
    // and cast to a destination class later.
    template<class T, class = enable_if_t<is_polymorphic_v<decay_t<T>> && !is_base_of_v<B, T>>>
    arg<B>(T& v)
    : base_(const_cast<B*>(dynamic_cast<const B*>(&v)))
    , const_(is_const_v<remove_reference_t<T>>)
    , p_(const_cast<void*>(reinterpret_cast<const void*>(&v)))
    , type_(typeid(v)) {
    }

    // Constructs from polymorphic value - we can try to cast it to the base class,
    // and cast to a destination class later.
    template<class T, class = typename std::enable_if<std::is_polymorphic<typename std::decay<T>::type>::value && std::is_base_of<B, T>::value>::type, class = void>
    arg<B>(T& v)
    : base_(const_cast<B*>(static_cast<const B*>(&v)))
    , const_(is_const_v<remove_reference_t<T>>)
    , type_(g_dummy_type_index) {
    }

    // Constructs from non-polymorphic value.
    template<class T, class = typename std::enable_if<!std::is_polymorphic_v<std::decay_t<T>>>::type, class = void, class = void>
    arg<B>(T& v)
    : base_(nullptr)
    , const_(is_const_v<remove_reference_t<T>>)
    , p_(const_cast<void*>(reinterpret_cast<const void*>(&v)))
    , type_(typeid(v)) {
    }

    // Cast to a polymorphic type
    template<class T>
    auto cast() -> typename std::enable_if<std::is_polymorphic<std::decay_t<T>>::value, std::remove_reference_t<T>*>::type {
        if(is_const_v<remove_reference_t<T>> || !const_ ) {
            if(auto p = dynamic_cast<typename std::decay<T>::type*>(base_))
                return p;
            if(!base_ && type_ == typeid(T))
                return reinterpret_cast<typename std::remove_reference<T>::type*>(p_);
        }
        return nullptr;
    }

    // Cast to a non-polymorphic type
    template<class T>
    auto cast() -> typename std::enable_if_t<!std::is_polymorphic_v<std::decay_t<T>>, std::remove_reference_t<T>*> {
        if(std::is_const_v<std::remove_reference_t<T>> || !const_ ) {
            if(!base_ && type_ == typeid(T))
                return reinterpret_cast<std::remove_reference_t<T>*>(p_);
        }
        return nullptr;
    }
};

/**********************************************************************************************/
// Base class for methods' implementations.
//
template<class T, class B>
struct abstract_method {
    using ret_t = typename std::conditional<std::is_same<T, void>::value, bool, std::optional<T>>::type;
    virtual ~abstract_method() {}
    virtual ret_t call() { return {}; }
    virtual ret_t call(arg<B>) { return {}; }
    virtual ret_t call(arg<B>, arg<B>) { return {}; }
    virtual ret_t call(arg<B>, arg<B>, arg<B>) { return {}; }
    virtual ret_t call(arg<B>, arg<B>, arg<B>, arg<B>) { return {}; }
    virtual ret_t call(arg<B>, arg<B>, arg<B>, arg<B>, arg<B>) { return {}; }
};


/**********************************************************************************************/
template<class B>
constexpr bool is_match_to_base() {
    return true;
}

/**********************************************************************************************/
template<class B, class T, class... Args>
constexpr bool is_match_to_base() {
    if(std::is_polymorphic<typename std::decay<T>::type>::value && !std::is_base_of<B, typename std::decay<T>::type>::value)
        return false;
    return is_match_to_base<B, Args...>();
}

/**********************************************************************************************/
template<class B, class F>
struct check_base_class_impl;

/**********************************************************************************************/
template<class B, class R, class... Args>
struct check_base_class_impl<B, R(*)(Args...)> {
    static constexpr bool value = is_match_to_base<B, Args...>();
};

/**********************************************************************************************/
template<class B, class F>
struct check_base_class : public check_base_class_impl<B, typename std::add_pointer<F>::type> {
};



/**********************************************************************************************/
template<class... Args> constexpr enable_if_t<sizeof...(Args) == 0, int> compare_const() { return 0; }
template<class T1> constexpr int compare_const() { return 0; }
template<class T1, class T2, class T3> constexpr int compare_const() { return 0; }
template<class T1, class T2, class T3, class T4, class T5> constexpr int compare_const() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7> constexpr int compare_const() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> constexpr int compare_const() { return 0; }

/**********************************************************************************************/
template<class T, class U>
constexpr int compare_const() {
    if constexpr(is_same_v<decay_t<T>, decay_t<U>>) {
        if constexpr(is_const_v<remove_reference_t<T>>)
            return is_const_v<remove_reference_t<U>> ? 0 : 1;
        else if constexpr(is_const_v<remove_reference_t<U>>) {
            return -1;
        }
    }
    return 0;
}

/**********************************************************************************************/
template<class T1, class T2, class U1, class U2>
constexpr int compare_const() {
    if(constexpr int r = compare_const<T1, U1>() )
        return r;
    return compare_const<T2, U2>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class U1, class U2, class U3>
constexpr int compare_const() {
    if(constexpr int r = compare_const<T1, U1>() )
        return r;
    return compare_const<T2, T3, U2, U3>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class T4, class U1, class U2, class U3, class U4>
constexpr int compare_const() {
    if(constexpr int r = compare_const<T1, U1>() )
        return r;
    return compare_const<T2, T3, T4, U2, U3, U4>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class T4, class T5, class U1, class U2, class U3, class U4, class U5>
constexpr int compare_const() {
    if(constexpr int r = compare_const<T1, U1>() )
        return r;
    return compare_const<T2, T3, T4, T5, U2, U3, U4, U5>();
}


/**********************************************************************************************/
template<class... Args> constexpr enable_if_t<sizeof...(Args) == 0, int> compare_types() { return 0; }
template<class T1> constexpr int compare_types() { return 0; }
template<class T1, class T2, class T3> constexpr int compare_types() { return 0; }
template<class T1, class T2, class T3, class T4, class T5> constexpr int compare_types() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7> constexpr int compare_types() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> constexpr int compare_types() { return 0; }

/**********************************************************************************************/
template<class T, class U>
constexpr int compare_types() {
    if(std::is_same<typename std::decay<T>::type, typename std::decay<U>::type>::value)
        return 0;
    if(std::is_base_of<typename std::decay<T>::type, typename std::decay<U>::type>::value)
        return 1;
    if(std::is_base_of<typename std::decay<U>::type, typename std::decay<T>::type>::value)
        return -1;
    return 0;
}

/**********************************************************************************************/
template<class T1, class T2, class U1, class U2>
constexpr int compare_types() {
    if(constexpr int r = compare_types<T1, U1>() )
        return r;
    return compare_types<T2, U2>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class U1, class U2, class U3>
constexpr int compare_types() {
    if(constexpr int r = compare_types<T1, U1>() )
        return r;
    return compare_types<T2, T3, U2, U3>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class T4, class U1, class U2, class U3, class U4>
constexpr int compare_types() {
    if(constexpr int r = compare_types<T1, U1>() )
        return r;
    return compare_types<T2, T3, T4, U2, U3, U4>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class T4, class T5, class U1, class U2, class U3, class U4, class U5>
constexpr int compare_types() {
    if(constexpr int r = compare_types<T1, U1>() )
        return r;
    return compare_types<T2, T3, T4, T5, U2, U3, U4, U5>();
}

/**********************************************************************************************/
template<class F1, class F2>
struct compare_methods_impl;

/**********************************************************************************************/
template<class R, class... Args1, class... Args2>
struct compare_methods_impl<R(*)(Args1...), R(*)(Args2...)> {
    static constexpr int value_types = sizeof...(Args1) < sizeof...(Args2)
        ? -1
        : sizeof...(Args1) > sizeof...(Args2)
          ? 1
          : compare_types<Args1..., Args2...>();

    static constexpr int value = value_types
          ? value_types
          : compare_const<Args1..., Args2...>();
};

/**********************************************************************************************/
template<class F1, class F2>
struct compare_methods : public compare_methods_impl<F1, F2> {
};


/**********************************************************************************************/
template<class F>
struct function_traits_impl;

/**********************************************************************************************/
template<class R>
struct function_traits_impl<R(*)()> {
    enum { arity = 0 };
    using ret_type = R;
};

/**********************************************************************************************/
template<class R, class T1>
struct function_traits_impl<R(*)(T1)> {
    enum { arity = 1 };
    using ret_type = R;
    using arg1_type = T1;
};

/**********************************************************************************************/
template<class R, class T1, class T2>
struct function_traits_impl<R(*)(T1, T2)> {
    enum { arity = 2 };
    using ret_type = R;
    using arg1_type = T1;
    using arg2_type = T2;
};

/**********************************************************************************************/
template<class R, class T1, class T2, class T3>
struct function_traits_impl<R(*)(T1, T2, T3)> {
    enum { arity = 3 };
    using ret_type = R;
    using arg1_type = T1;
    using arg2_type = T2;
    using arg3_type = T3;
};

/**********************************************************************************************/
template<class R, class T1, class T2, class T3, class T4>
struct function_traits_impl<R(*)(T1, T2, T3, T4)> {
    enum { arity = 4 };
    using ret_type = R;
    using arg1_type = T1;
    using arg2_type = T2;
    using arg3_type = T3;
    using arg4_type = T4;
};

template<class R, class T1, class T2, class T3, class T4, class T5>
struct function_traits_impl<R(*)(T1, T2, T3, T4, T5)> {
    enum { arity = 5 };
    using ret_type = R;
    using arg1_type = T1;
    using arg2_type = T2;
    using arg3_type = T3;
    using arg4_type = T4;
    using arg5_type = T5;
};

/**********************************************************************************************/
template<class F>
struct function_traits : public function_traits_impl<F> {
};


/**********************************************************************************************/
template<class T, class B, class F>
struct method_0 final : abstract_method<T, B> {
    F f_;
    method_0(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call() { return f_(); }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_0_void final : abstract_method<T, B> {
    F f_;
    method_0_void(F f) : f_(f) {}

    bool call() { f_(); return true; }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_1 final : abstract_method<T, B> {
    F f_;
    method_1(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p) {
        if(auto u1 = p.template cast<typename function_traits<F>::arg1_type>())
            return f_(*u1);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_1_void final : abstract_method<T, B> {
    F f_;
    method_1_void(F f) : f_(f) {}

    bool call(arg<B> p) {
        if(auto u1 = p.template cast<typename function_traits<F>::arg1_type>())
            { f_(*u1); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_2 final : abstract_method<T, B> {
    F f_;
    method_2(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2) {
        if(auto u1 = p1.template cast<typename function_traits<F>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<F>::arg2_type>())
                return f_(*u1, *u2);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_2_void final : abstract_method<T, B> {
    F f_;
    method_2_void(F f) : f_(f) {}

    bool call(arg<B> p1, arg<B> p2) {
        if(auto u1 = p1.template cast<typename function_traits<F>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<F>::arg2_type>())
                { f_(*u1, *u2); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_3 final : abstract_method<T, B> {
    F f_;
    method_3(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3) {
        if(auto u1 = p1.template cast<typename function_traits<F>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<F>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<F>::arg3_type>())
                        return f_(*u1, *u2, *u3);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_3_void final : abstract_method<T, B> {
    F f_;
    method_3_void(F f) : f_(f) {}

    bool call(arg<B> p1, arg<B> p2, arg<B> p3) {
        if(auto u1 = p1.template cast<typename function_traits<F>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<F>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<F>::arg3_type>())
                    { f_(*u1, *u2, *u3); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_4 final : abstract_method<T, B> {
    F f_;
    method_4(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4) {
        if(auto u1 = p1.template cast<typename function_traits<F>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<F>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<F>::arg3_type>())
                    if(auto u4 = p4.template cast<typename function_traits<F>::arg4_type>())
                        return f_(*u1, *u2, *u3, *u4);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_4_void final : abstract_method<T, B> {
    F f_;
    method_4_void(F f) : f_(f) {}

    bool call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4) {
        if(auto u1 = p1.template cast<typename function_traits<F>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<F>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<F>::arg3_type>())
                    if(auto u4 = p4.template cast<typename function_traits<F>::arg4_type>())
                        { f_(*u1, *u2, *u3, *u4); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_5 final : abstract_method<T, B> {
    F f_;
    method_5(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4, arg<B> p5) {
        if(auto u1 = p1.template cast<typename function_traits<F>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<F>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<F>::arg3_type>())
                    if(auto u4 = p4.template cast<typename function_traits<F>::arg4_type>())
                        if(auto u5 = p5.template cast<typename function_traits<F>::arg5_type>())
                            return f_(*u1, *u2, *u3, *u4, *u5);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_5_void final : abstract_method<T, B> {
    F f_;
    method_5_void(F f) : f_(f) {}

    bool call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4) {
        if(auto u1 = p1.template cast<typename function_traits<F>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<F>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<F>::arg3_type>())
                    if(auto u4 = p4.template cast<typename function_traits<F>::arg4_type>())
                        if(auto u5 = p4.template cast<typename function_traits<F>::arg5_type>())
                            { f_(*u1, *u2, *u3, *u4, *u5); return true; }
        return false;
    }
};


/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 0 && !std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_0<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 0 && std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_0_void<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 1 && !std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_1<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 1 && std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_1_void<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 2 && !std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_2<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 2 && std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_2_void<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 3 && !std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_3<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 3 && std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_3_void<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 4 && !std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_4<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 4 && std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_4_void<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 5 && !std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_5<T, B, F>(f);
}

/**********************************************************************************************/
template<class T, class B, class F> inline
auto make_method(F f) -> typename std::enable_if<function_traits<F>::arity == 5 && std::is_same<void, typename function_traits<F>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_5_void<T, B, F>(f);
}

/**********************************************************************************************/
template<class T>
struct method_result_impl {
    static T& unwrap(std::optional<T>& o) {return *o;}
};

/**********************************************************************************************/
template<>
struct method_result_impl<void> {
    static void unwrap(bool) {}
};

/**********************************************************************************************/
template<class T>
struct method_result : public method_result_impl<T> {
};


/**********************************************************************************************/
template<class T=void, class U=::multimethods::unknown>
struct multimethod_parameters {
    using type = T;
    using base = U;
};

template<std::size_t N, class T, class... types>
struct get_Nth_type {
    using type = typename get_Nth_type<N - 1, types...>::type;
};

template<class T, class... types>
struct get_Nth_type<0, T, types...> {
    using type = T;
};

/**********************************************************************************************/
template<class... Funcs>
struct sort_functions {
    sort_functions( const std::tuple<bool, Funcs...>& funcs ) : funcs_(funcs) {}

    const std::tuple<bool, Funcs...>& funcs_;

    static constexpr int N = sizeof...(Funcs);

    template<std::size_t N>
    using getF = typename get_Nth_type<N, Funcs...>::type;

#define MM_FUNC_TYPE(I) \
    using F ## I = getF<(I<N?I:0)>

    MM_FUNC_TYPE(0);
    MM_FUNC_TYPE(1);
    MM_FUNC_TYPE(2);
    MM_FUNC_TYPE(3);
    MM_FUNC_TYPE(4);
    MM_FUNC_TYPE(5);
    MM_FUNC_TYPE(6);
    MM_FUNC_TYPE(7);
    MM_FUNC_TYPE(8);
    MM_FUNC_TYPE(9);
    MM_FUNC_TYPE(10);
    MM_FUNC_TYPE(11);
    MM_FUNC_TYPE(12);
    MM_FUNC_TYPE(13);
    MM_FUNC_TYPE(14);
    MM_FUNC_TYPE(15);
    MM_FUNC_TYPE(16);
    MM_FUNC_TYPE(17);
    MM_FUNC_TYPE(18);
    MM_FUNC_TYPE(19);
    MM_FUNC_TYPE(20);
    MM_FUNC_TYPE(21);
    MM_FUNC_TYPE(22);
    MM_FUNC_TYPE(23);
    MM_FUNC_TYPE(24);
    MM_FUNC_TYPE(25);
    MM_FUNC_TYPE(26);
    MM_FUNC_TYPE(27);
    MM_FUNC_TYPE(28);
    MM_FUNC_TYPE(29);
    MM_FUNC_TYPE(30);
    MM_FUNC_TYPE(31);
    MM_FUNC_TYPE(32);

#undef MM_FUNC_TYPE

    template<class A>
    static constexpr bool pred(int b) {
        #define MM_CASE_B(I) \
            if(b == I) return compare_methods<A, F ## I>::value < 0;

        MM_CASE_B(0); MM_CASE_B(1); MM_CASE_B(2); MM_CASE_B(3);
        MM_CASE_B(4); MM_CASE_B(5); MM_CASE_B(6); MM_CASE_B(7);
        MM_CASE_B(8); MM_CASE_B(9); MM_CASE_B(10); MM_CASE_B(11);
        MM_CASE_B(12); MM_CASE_B(13); MM_CASE_B(14); MM_CASE_B(15);
        MM_CASE_B(16); MM_CASE_B(17); MM_CASE_B(18); MM_CASE_B(19);
        MM_CASE_B(20); MM_CASE_B(21); MM_CASE_B(22); MM_CASE_B(23);
        MM_CASE_B(24); MM_CASE_B(25); MM_CASE_B(26); MM_CASE_B(27);
        MM_CASE_B(28); MM_CASE_B(29); MM_CASE_B(30); MM_CASE_B(31);
        MM_CASE_B(32);

        #undef MM_CASE_B

        return false;
    }

    template<class TR, class BR>
    auto sort() {
        int indexes[N];
        for( int i = 0 ; i < N ; ++i )
            indexes[i] = i;

        std::sort(indexes, indexes + N, [](int a, int b) {
            #define MM_CASE_A(I) \
                if(a == I) return sort_functions::pred<F ## I>(b)

            MM_CASE_A(0); MM_CASE_A(1); MM_CASE_A(2); MM_CASE_A(3);
            MM_CASE_A(4); MM_CASE_A(5); MM_CASE_A(6); MM_CASE_A(7);
            MM_CASE_A(8); MM_CASE_A(9); MM_CASE_A(10); MM_CASE_A(11);
            MM_CASE_A(12); MM_CASE_A(13); MM_CASE_A(14); MM_CASE_A(15);
            MM_CASE_A(16); MM_CASE_A(17); MM_CASE_A(18); MM_CASE_A(19);
            MM_CASE_A(20); MM_CASE_A(21); MM_CASE_A(22); MM_CASE_A(23);
            MM_CASE_A(24); MM_CASE_A(25); MM_CASE_A(26); MM_CASE_A(27);
            MM_CASE_A(28); MM_CASE_A(29); MM_CASE_A(30); MM_CASE_A(31);
            MM_CASE_A(32);

            #undef MM_CASE_A

            return a<b;
        } );

        std::vector<abstract_method<TR, BR>*> r(N);
        for( int i = 0 ; i < N ; ++i )
        {
            switch(indexes[i]) {

                #define MM_MAKE_METHOD(I) \
                    case I: r[i] = make_method<TR, BR, F ## I>(std::get<I<N?I+1:1>(funcs_)); break

                MM_MAKE_METHOD(0); MM_MAKE_METHOD(1); MM_MAKE_METHOD(2); MM_MAKE_METHOD(3);
                MM_MAKE_METHOD(4); MM_MAKE_METHOD(5); MM_MAKE_METHOD(6); MM_MAKE_METHOD(7);
                MM_MAKE_METHOD(8); MM_MAKE_METHOD(9); MM_MAKE_METHOD(10); MM_MAKE_METHOD(11);
                MM_MAKE_METHOD(12); MM_MAKE_METHOD(13); MM_MAKE_METHOD(14); MM_MAKE_METHOD(15);
                MM_MAKE_METHOD(16); MM_MAKE_METHOD(17); MM_MAKE_METHOD(18); MM_MAKE_METHOD(19);
                MM_MAKE_METHOD(20); MM_MAKE_METHOD(21); MM_MAKE_METHOD(22); MM_MAKE_METHOD(23);
                MM_MAKE_METHOD(24); MM_MAKE_METHOD(25); MM_MAKE_METHOD(26); MM_MAKE_METHOD(27);
                MM_MAKE_METHOD(28); MM_MAKE_METHOD(29); MM_MAKE_METHOD(30); MM_MAKE_METHOD(31);
                MM_MAKE_METHOD(32);

                #undef MM_MAKE_METHOD

                default:;
            }
        }

        return r;
    }
};


/**********************************************************************************************/
} }

/**********************************************************************************************/
// Helper macros for 'define_method' macro.
//
#define MM_JOIN(x, y) MM_JOIN_AGAIN(x, y)
#define MM_JOIN_AGAIN(x, y) x ## y

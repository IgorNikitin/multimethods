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

/**********************************************************************************************/
// Adds fallback handler for a multimethod.
//
//   declare_method(collide)
//   define_method_fallback(collide) { cout << "All is fine.\n"; }
//
#define define_method_fallback(name) \
   static g_mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)(); \
   static bool MM_JOIN(_mm_init_, __LINE__) = []{ g_mm_f_ ## name ::fallback_ = MM_JOIN(_mm_impl_, __LINE__); return true; }(); \
   static g_mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)()

/**********************************************************************************************/
// Skip current method in runtime to search for more suitable implementation.
//
//   define_method(mm_abs, int n) { if(n > 0) skip_method; return -n; }
//
#define skip_method throw ::multimethods::not_match();


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
struct not_match final : std::exception {
    virtual const char* what() const noexcept { return "multimethods::not_match"; }
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
static inline const std::type_index g_dummy_type_index( typeid(int) );

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
    template<class T, class = typename std::enable_if<std::is_polymorphic<typename std::decay<T>::type>::value && !std::is_base_of<B, T>::value>::type>
    arg<B>(T& v)
    : base_(const_cast<B*>(dynamic_cast<const B*>(&v)))
    , const_(std::is_const<T>::value)
    , p_(const_cast<void*>(reinterpret_cast<const void*>(&v)))
    , type_(typeid(v)) {
    }

    // Constructs from polymorphic value - we can try to cast it to the base class,
    // and cast to a destination class later.
    template<class T, class = typename std::enable_if<std::is_polymorphic<typename std::decay<T>::type>::value && std::is_base_of<B, T>::value>::type, class = void>
    arg<B>(T& v)
    : base_(const_cast<B*>(static_cast<const B*>(&v)))
    , const_(std::is_const<T>::value)
    , type_(g_dummy_type_index) {
    }

    // Constructs from non-polymorphic value.
    template<class T, class = typename std::enable_if<!std::is_polymorphic<typename std::decay<T>::type>::value>::type, class = void, class = void>
    arg<B>(T& v)
    : base_(nullptr)
    , const_(std::is_const<T>::value)
    , p_(const_cast<void*>(reinterpret_cast<const void*>(&v)))
    , type_(typeid(v)) {
    }

    // Cast to a polymorphic type
    template<class T>
    auto cast() -> typename std::enable_if<std::is_polymorphic<typename std::decay<T>::type>::value, typename std::remove_reference<T>::type*>::type {
        if(std::is_const<typename std::remove_reference<T>::type>::value || !const_ ) {
            if(auto p = dynamic_cast<typename std::decay<T>::type*>(base_))
                return p;
            if(!base_ && type_ == typeid(T))
                return reinterpret_cast<typename std::remove_reference<T>::type*>(p_);
        }
        return nullptr;
    }

    // Cast to a non-polymorphic type
    template<class T>
    auto cast() -> typename std::enable_if<!std::is_polymorphic<typename std::decay<T>::type>::value, typename std::remove_reference<T>::type*>::type {
        if(std::is_const<typename std::remove_reference<T>::type>::value || !const_ ) {
            if(!base_ && type_ == typeid(T))
                return reinterpret_cast<typename std::remove_reference<T>::type*>(p_);
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
constexpr int compare_types() { return 0; }
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
template<class R1>
struct compare_methods_impl<R1(*const)(), R1(*const)()> {
    static constexpr int value = 0;
};

/**********************************************************************************************/
template<class R1, class... Args1, class... Args2>
struct compare_methods_impl<R1(*)(Args1...), R1(*)(Args2...)> {
    static constexpr int value = sizeof...(Args1) < sizeof...(Args2)
        ? -1
        : sizeof...(Args1) > sizeof...(Args2)
          ? 1
          : compare_types<Args1..., Args2...>();
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
    using type=T;
    using base=U;
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

#undef MM_FUNC_TYPE

    template<class A>
    static constexpr bool pred(int b) {
        if(b == 0) return compare_methods<A, F0>::value < 0;
        if(b == 1) return compare_methods<A, F1>::value < 0;
        if(b == 2) return compare_methods<A, F2>::value < 0;
        if(b == 3) return compare_methods<A, F3>::value < 0;
        if(b == 4) return compare_methods<A, F4>::value < 0;
        if(b == 5) return compare_methods<A, F5>::value < 0;
        if(b == 6) return compare_methods<A, F6>::value < 0;
        if(b == 7) return compare_methods<A, F7>::value < 0;
        if(b == 8) return compare_methods<A, F8>::value < 0;
        if(b == 9) return compare_methods<A, F9>::value < 0;
        if(b == 10) return compare_methods<A, F10>::value < 0;
        if(b == 11) return compare_methods<A, F11>::value < 0;
        if(b == 12) return compare_methods<A, F12>::value < 0;
        if(b == 13) return compare_methods<A, F13>::value < 0;
        if(b == 14) return compare_methods<A, F14>::value < 0;
        if(b == 15) return compare_methods<A, F15>::value < 0;
        if(b == 16) return compare_methods<A, F16>::value < 0;
        if(b == 17) return compare_methods<A, F17>::value < 0;
        if(b == 18) return compare_methods<A, F18>::value < 0;
        if(b == 19) return compare_methods<A, F19>::value < 0;
        if(b == 20) return compare_methods<A, F20>::value < 0;

        return false;
    }

    template<class TR, class BR>
    auto sort() {
        int indexes[N];
        for( int i = 0 ; i < N ; ++i )
            indexes[i] = i;

        std::sort(indexes, indexes + N, [](int a, int b) {
            if(a == 0) return sort_functions::pred<F0>(b);
            if(a == 1) return sort_functions::pred<F1>(b);
            if(a == 2) return sort_functions::pred<F2>(b);
            if(a == 3) return sort_functions::pred<F3>(b);
            if(a == 4) return sort_functions::pred<F4>(b);
            if(a == 5) return sort_functions::pred<F5>(b);
            if(a == 6) return sort_functions::pred<F6>(b);
            if(a == 7) return sort_functions::pred<F7>(b);
            if(a == 8) return sort_functions::pred<F8>(b);
            if(a == 9) return sort_functions::pred<F9>(b);
            if(a == 10) return sort_functions::pred<F10>(b);
            if(a == 11) return sort_functions::pred<F11>(b);
            if(a == 12) return sort_functions::pred<F12>(b);
            if(a == 13) return sort_functions::pred<F13>(b);
            if(a == 14) return sort_functions::pred<F14>(b);
            if(a == 15) return sort_functions::pred<F15>(b);
            if(a == 16) return sort_functions::pred<F16>(b);
            if(a == 17) return sort_functions::pred<F17>(b);
            if(a == 18) return sort_functions::pred<F18>(b);
            if(a == 19) return sort_functions::pred<F19>(b);
            if(a == 20) return sort_functions::pred<F20>(b);

            return a<b;
        } );

        std::vector<abstract_method<TR, BR>*> r(N);
        for( int i = 0 ; i < N ; ++i )
        {
            #define MM_MAKE_METHOD(I) \
                case I: r[i] = make_method<TR, BR, F ## I>(std::get<I<N?I+1:1>(funcs_)); break;

            switch(indexes[i]) {
                MM_MAKE_METHOD(0);
                MM_MAKE_METHOD(1);
                MM_MAKE_METHOD(2);
                MM_MAKE_METHOD(3);
                MM_MAKE_METHOD(4);
                MM_MAKE_METHOD(5);
                MM_MAKE_METHOD(6);
                MM_MAKE_METHOD(7);
                MM_MAKE_METHOD(8);
                MM_MAKE_METHOD(9);
                MM_MAKE_METHOD(10);
                MM_MAKE_METHOD(11);
                MM_MAKE_METHOD(12);
                MM_MAKE_METHOD(13);
                MM_MAKE_METHOD(14);
                MM_MAKE_METHOD(15);
                MM_MAKE_METHOD(16);
                MM_MAKE_METHOD(17);
                MM_MAKE_METHOD(18);
                MM_MAKE_METHOD(19);
                MM_MAKE_METHOD(20);

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

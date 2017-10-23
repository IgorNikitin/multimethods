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
//   define_method(collide)
//       match(asteroid&, asteroid&) {}
//       match(asteroid&, spaceship&) {}
//   end_method
//
#define multi_method(name, ...) \
    mm_proto_ret_ ## name(){}; \
    namespace mm_namespace_ ## name { \
        using namespace ::multimethods::detail; \
        \
        using base_t     = multimethod_parameters<__VA_ARGS__>::base; \
        using ret_type_t = std::result_of_t<decltype(&mm_proto_ret_ ## name)()>; \
        using method_t   = abstract_method<ret_type_t, base_t>; \
        \
        static inline method_t* g_fallback { nullptr }; \
        static inline std::vector<method_t*> g_funcs; \
    }; \
    \
    template<class... Args> inline \
    mm_namespace_ ## name::ret_type_t name(Args&&... args) { \
        for( auto m : mm_namespace_ ## name ::g_funcs ) \
            try { \
                if(auto r = m->call(args...)) \
                    return ::multimethods::detail::method_result<mm_namespace_ ## name::ret_type_t>::unwrap(r); \
            } catch(::multimethods::try_next&) { \
            } \
        if(mm_namespace_ ## name ::g_fallback) { \
            ::multimethods::detail::fallback_t fb; \
            if(auto r = mm_namespace_ ## name ::g_fallback->call(fb)) \
                return ::multimethods::detail::method_result<mm_namespace_ ## name::ret_type_t>::unwrap(r); \
        } \
        throw ::multimethods::not_implemented(#name ": not_implemented."); \
    } \
    \
    namespace mm_namespace_ ## name { \
        const bool g_init = [] { const std::tuple funcs { true

/**********************************************************************************************/
// Adds implementation of a method.
//
//   match(asteroid&, asteroid&) { cout << "Traverse.\n"; }
//   match(asteroid&, spaceship&) { cout << "Boom!\n"; }
//
#define match(...) , +[](__VA_ARGS__) -> ret_type_t

/**********************************************************************************************/
// Finishes definition of a multimethod.
//
//   define_method(collide)
//       match(asteroid&, asteroid&) {}
//       match(asteroid&, spaceship&) {}
//   end_method
//
#define end_method \
            }; \
            ::multimethods::detail::sort_functions sorter(funcs); \
            g_funcs = sorter.sort_methods<ret_type_t, base_t>(); \
            for(auto it = g_funcs.begin() ; it != g_funcs.end() ; ++it) \
                if((*it)->is_fallback()) { \
                    g_fallback = *it; \
                    g_funcs.erase(it); \
                    break; \
                } \
            return true; \
        }(); \
    }

/**********************************************************************************************/
// Adds fallback handler for a multimethod.
//
//   define_method(collide)
//   fallback { cout << "All is fine.\n"; }
//
#define fallback , +[](::multimethods::detail::fallback_t) -> ret_type_t

/**********************************************************************************************/
// Skips current method in runtime to search for more suitable implementation.
//
//   match(int n) { if(n > 0) next_method; return -n; }
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
    template<class T, class = enable_if_t<is_polymorphic_v<decay_t<T>> && is_base_of_v<B, T>>, class = void>
    arg<B>(T& v)
    : base_(const_cast<B*>(static_cast<const B*>(&v)))
    , const_(is_const_v<remove_reference_t<T>>)
    , type_(g_dummy_type_index) {
    }

    // Constructs from non-polymorphic value.
    template<class T, class = enable_if_t<!is_polymorphic_v<decay_t<T>>>, class = void, class = void>
    arg<B>(T& v)
    : base_(nullptr)
    , const_(is_const_v<remove_reference_t<T>>)
    , p_(const_cast<void*>(reinterpret_cast<const void*>(&v)))
    , type_(typeid(v)) {
    }

    // Cast to a polymorphic type
    template<class T>
    auto cast() -> enable_if_t<is_polymorphic_v<decay_t<T>>, remove_reference_t<T>*> {
        if(is_const_v<remove_reference_t<T>> || !const_ ) {
            if(auto p = dynamic_cast<decay_t<T>*>(base_))
                return p;
            if(!base_ && type_ == typeid(T))
                return reinterpret_cast<remove_reference_t<T>*>(p_);
        }
        return nullptr;
    }

    // Cast to a non-polymorphic type
    template<class T>
    auto cast() -> enable_if_t<!is_polymorphic_v<decay_t<T>>, remove_reference_t<T>*> {
        if(std::is_const_v<std::remove_reference_t<T>> || !const_ ) {
            if(!base_ && type_ == typeid(T))
                return reinterpret_cast<remove_reference_t<T>*>(p_);
        }
        return nullptr;
    }
};

/**********************************************************************************************/
// Base class for methods' implementations.
//
template<class T, class B>
struct abstract_method {
    using ret_t = conditional_t<is_same_v<T, void>, bool, optional<T>>;
    virtual ~abstract_method() {}
    virtual ret_t call() { return {}; }
    virtual ret_t call(arg<B>) { return {}; }
    virtual ret_t call(arg<B>, arg<B>) { return {}; }
    virtual ret_t call(arg<B>, arg<B>, arg<B>) { return {}; }
    virtual ret_t call(arg<B>, arg<B>, arg<B>, arg<B>) { return {}; }
    virtual ret_t call(arg<B>, arg<B>, arg<B>, arg<B>, arg<B>) { return {}; }
    virtual ret_t call(arg<B>, arg<B>, arg<B>, arg<B>, arg<B>, arg<B>) { return {}; }
    virtual bool is_fallback() const { return false; }
};


/**********************************************************************************************/
template<class B>
constexpr bool is_match_to_base() {
    return true;
}

/**********************************************************************************************/
template<class B, class T, class... Args>
constexpr bool is_match_to_base() {
    if(is_polymorphic_v<decay_t<T>> && !is_base_of_v<B, decay_t<T>>)
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
struct check_base_class : public check_base_class_impl<B, F> {
};


/**********************************************************************************************/
template<class... Args> constexpr enable_if_t<sizeof...(Args) == 0, int> compare_const() { return 0; }
template<class T1> constexpr int compare_const() { return 0; }
template<class T1, class T2, class T3> constexpr int compare_const() { return 0; }
template<class T1, class T2, class T3, class T4, class T5> constexpr int compare_const() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7> constexpr int compare_const() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> constexpr int compare_const() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11> constexpr int compare_const() { return 0; }

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
template<class T1, class T2, class T3, class T4, class T5, class T6, class U1, class U2, class U3, class U4, class U5, class U6>
constexpr int compare_const() {
    if(constexpr int r = compare_const<T1, U1>() )
        return r;
    return compare_const<T2, T3, T4, T5, T6, U2, U3, U4, U5, U6>();
}


/**********************************************************************************************/
template<class... Args> constexpr enable_if_t<sizeof...(Args) == 0, int> compare_types() { return 0; }
template<class T1> constexpr int compare_types() { return 0; }
template<class T1, class T2, class T3> constexpr int compare_types() { return 0; }
template<class T1, class T2, class T3, class T4, class T5> constexpr int compare_types() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7> constexpr int compare_types() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> constexpr int compare_types() { return 0; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11> constexpr int compare_types() { return 0; }

/**********************************************************************************************/
template<class T, class U>
constexpr int compare_types() {
    if(is_same_v<decay_t<T>, decay_t<U>>)
        return 0;
    if(is_base_of_v<decay_t<T>, decay_t<U>>)
        return 1;
    if(is_base_of_v<decay_t<U>, decay_t<T>>)
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
template<class T1, class T2, class T3, class T4, class T5, class T6, class U1, class U2, class U3, class U4, class U5, class U6>
constexpr int compare_types() {
    if(constexpr int r = compare_types<T1, U1>() )
        return r;
    return compare_types<T2, T3, T4, T5, T6, U2, U3, U4, U5, U6>();
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

template<class R, class T1, class T2, class T3, class T4, class T5, class T6>
struct function_traits_impl<R(*)(T1, T2, T3, T4, T5, T6)> {
    enum { arity = 6 };
    using ret_type = R;
    using arg1_type = T1;
    using arg2_type = T2;
    using arg3_type = T3;
    using arg4_type = T4;
    using arg5_type = T5;
    using arg6_type = T6;
};

/**********************************************************************************************/
template<class F>
struct function_traits : public function_traits_impl<F> {
};


/**********************************************************************************************/
#define MM_CAST_1 if(auto u1 = p1.template cast<typename function_traits<F>::arg1_type>())
#define MM_CAST_2 MM_CAST_1 if(auto u2 = p2.template cast<typename function_traits<F>::arg2_type>())
#define MM_CAST_3 MM_CAST_2 if(auto u3 = p3.template cast<typename function_traits<F>::arg3_type>())
#define MM_CAST_4 MM_CAST_3 if(auto u4 = p4.template cast<typename function_traits<F>::arg4_type>())
#define MM_CAST_5 MM_CAST_4 if(auto u5 = p5.template cast<typename function_traits<F>::arg5_type>())
#define MM_CAST_6 MM_CAST_5 if(auto u6 = p6.template cast<typename function_traits<F>::arg6_type>())

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

    typename abstract_method<T, B>::ret_t call(arg<B> p1) {
        MM_CAST_1 return f_(*u1);
        return {};
    }

    virtual bool is_fallback() const { return is_same_v<fallback_t, typename function_traits<F>::arg1_type>; }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_1_void final : abstract_method<T, B> {
    F f_;
    method_1_void(F f) : f_(f) {}

    bool call(arg<B> p1) {
        MM_CAST_1 { f_(*u1); return true; }
        return false;
    }

    virtual bool is_fallback() const { return is_same_v<fallback_t, typename function_traits<F>::arg1_type>; }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_2 final : abstract_method<T, B> {
    F f_;
    method_2(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2) {
        MM_CAST_2 return f_(*u1, *u2);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_2_void final : abstract_method<T, B> {
    F f_;
    method_2_void(F f) : f_(f) {}

    bool call(arg<B> p1, arg<B> p2) {
        MM_CAST_2 { f_(*u1, *u2); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_3 final : abstract_method<T, B> {
    F f_;
    method_3(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3) {
        MM_CAST_3 return f_(*u1, *u2, *u3);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_3_void final : abstract_method<T, B> {
    F f_;
    method_3_void(F f) : f_(f) {}

    bool call(arg<B> p1, arg<B> p2, arg<B> p3) {
        MM_CAST_3 { f_(*u1, *u2, *u3); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_4 final : abstract_method<T, B> {
    F f_;
    method_4(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4) {
        MM_CAST_4 return f_(*u1, *u2, *u3, *u4);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_4_void final : abstract_method<T, B> {
    F f_;
    method_4_void(F f) : f_(f) {}

    bool call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4) {
        MM_CAST_4 { f_(*u1, *u2, *u3, *u4); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_5 final : abstract_method<T, B> {
    F f_;
    method_5(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4, arg<B> p5) {
        MM_CAST_5 return f_(*u1, *u2, *u3, *u4, *u5);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_5_void final : abstract_method<T, B> {
    F f_;
    method_5_void(F f) : f_(f) {}

    bool call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4, arg<B> p5) {
        MM_CAST_5 { f_(*u1, *u2, *u3, *u4, *u5); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_6 final : abstract_method<T, B> {
    F f_;
    method_6(F f) : f_(f) {}

    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4, arg<B> p5, arg<B> p6) {
        MM_CAST_6 return f_(*u1, *u2, *u3, *u4, *u5, *u6);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, class F>
struct method_6_void final : abstract_method<T, B> {
    F f_;
    method_6_void(F f) : f_(f) {}

    bool call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4, arg<B> p5, arg<B> p6) {
        MM_CAST_6 { f_(*u1, *u2, *u3, *u4, *u5, *u6); return true; }
        return false;
    }
};


/**********************************************************************************************/
#define MM_MAKE_METHOD(N) \
    template<class T, class B, class F> inline \
    auto make_method(F f) -> std::enable_if_t<function_traits<F>::arity == N && !std::is_same_v<void, typename function_traits<F>::ret_type>, abstract_method<T, B>*> { \
        static_assert(check_base_class<B, F>::value, "Please specify common base class for polymorphic types in 'define_method' statement, or use 'multimethods::unknown' as base class."); \
        return new method_ ## N<T, B, F>(f); \
    } \
    \
    template<class T, class B, class F> inline \
    auto make_method(F f) -> std::enable_if_t<function_traits<F>::arity == N && std::is_same_v<void, typename function_traits<F>::ret_type>, abstract_method<T, B>*> { \
        static_assert(check_base_class<B, F>::value, "Please specify common base class for polymorphic types in 'define_method' statement, or use 'multimethods::unknown' as base class."); \
        return new method_ ## N ## _void<T, B, F>(f); \
    }

/**********************************************************************************************/
MM_MAKE_METHOD(0)
MM_MAKE_METHOD(1)
MM_MAKE_METHOD(2)
MM_MAKE_METHOD(3)
MM_MAKE_METHOD(4)
MM_MAKE_METHOD(5)
MM_MAKE_METHOD(6)


/**********************************************************************************************/
template<class T>
struct method_result_impl {
    static T& unwrap(std::optional<T>& o) { return *o; }
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
template<class U=::multimethods::unknown>
struct multimethod_parameters {
    using base = U;
};


/**********************************************************************************************/
template<std::size_t N, class T, class... Args>
struct get_type_by_index {
    using type = typename get_type_by_index<N - 1, Args...>::type;
};

/**********************************************************************************************/
template<class T, class... Args>
struct get_type_by_index<0, T, Args...> {
    using type = T;
};


/**********************************************************************************************/
template<class... Funcs>
struct sort_functions {
    const std::tuple<bool, Funcs...>& funcs_;
    sort_functions( const std::tuple<bool, Funcs...>& funcs ) : funcs_(funcs) {}

    static constexpr int N = sizeof...(Funcs);

    template<std::size_t N>
    using func_type_t = typename get_type_by_index<N, Funcs...>::type;

    #define MM_FUNC_TYPE(I) \
        using F ## I = func_type_t<(I < N ? I : 0)>

    MM_FUNC_TYPE(0); MM_FUNC_TYPE(1); MM_FUNC_TYPE(2); MM_FUNC_TYPE(3);
    MM_FUNC_TYPE(4); MM_FUNC_TYPE(5); MM_FUNC_TYPE(6); MM_FUNC_TYPE(7);
    MM_FUNC_TYPE(8); MM_FUNC_TYPE(9); MM_FUNC_TYPE(10); MM_FUNC_TYPE(11);
    MM_FUNC_TYPE(12); MM_FUNC_TYPE(13); MM_FUNC_TYPE(14); MM_FUNC_TYPE(15);
    MM_FUNC_TYPE(16); MM_FUNC_TYPE(17); MM_FUNC_TYPE(18); MM_FUNC_TYPE(19);
    MM_FUNC_TYPE(20); MM_FUNC_TYPE(21); MM_FUNC_TYPE(22); MM_FUNC_TYPE(23);
    MM_FUNC_TYPE(24); MM_FUNC_TYPE(25); MM_FUNC_TYPE(26); MM_FUNC_TYPE(27);
    MM_FUNC_TYPE(28); MM_FUNC_TYPE(29); MM_FUNC_TYPE(30); MM_FUNC_TYPE(31);
    MM_FUNC_TYPE(32);

    #undef MM_FUNC_TYPE

    // Predicate function to sort methods
    template<class A>
    static constexpr bool pred_b(int b) {
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

    // Sorts methods and return vector with instances of 'abstract_method'
    template<class TR, class BR>
    auto sort_methods() {
        // Use indexes cause we cannot sort tuple itself
        int indexes[N];
        for(int i = 0 ; i < N ; ++i)
            indexes[i] = i;

        // Use quick-sort to sort methods by types and it's constness
        std::sort(indexes, indexes + N, [](int a, int b) {
            #define MM_CASE_A(I) \
                if(a == I) return sort_functions::pred_b<F ## I>(b)

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

        // Create and fill vector with methods
        std::vector<abstract_method<TR, BR>*> r(N);

        for(int i = 0 ; i < N ; ++i) {
            switch(indexes[i]) {
                #define MM_FILL_VECTOR(I) \
                    case I: r[i] = make_method<TR, BR, F ## I>(std::get<I < N ? I + 1 : 1>(funcs_)); break

                MM_FILL_VECTOR(0); MM_FILL_VECTOR(1); MM_FILL_VECTOR(2); MM_FILL_VECTOR(3);
                MM_FILL_VECTOR(4); MM_FILL_VECTOR(5); MM_FILL_VECTOR(6); MM_FILL_VECTOR(7);
                MM_FILL_VECTOR(8); MM_FILL_VECTOR(9); MM_FILL_VECTOR(10); MM_FILL_VECTOR(11);
                MM_FILL_VECTOR(12); MM_FILL_VECTOR(13); MM_FILL_VECTOR(14); MM_FILL_VECTOR(15);
                MM_FILL_VECTOR(16); MM_FILL_VECTOR(17); MM_FILL_VECTOR(18); MM_FILL_VECTOR(19);
                MM_FILL_VECTOR(20); MM_FILL_VECTOR(21); MM_FILL_VECTOR(22); MM_FILL_VECTOR(23);
                MM_FILL_VECTOR(24); MM_FILL_VECTOR(25); MM_FILL_VECTOR(26); MM_FILL_VECTOR(27);
                MM_FILL_VECTOR(28); MM_FILL_VECTOR(29); MM_FILL_VECTOR(30); MM_FILL_VECTOR(31);
                MM_FILL_VECTOR(32);

                #undef MM_FILL_VECTOR

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

/**********************************************************************************************/
// multimethods.h
//
// Igor Nikitin, 2017.
// igor_nikitin@valentina-db.com
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
/**********************************************************************************************/

#include <algorithm>
#include <any>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

/**********************************************************************************************/
// Defines a new multimethod and specifies it's result type and parameters.
//
//   void define_method(collide, thing&, thing&)
//       match(asteroid&, asteroid&) {}
//       match(asteroid&, spaceship&) {}
//   end_method
//
#define multi_method(name, ...) \
    mm_proto_ ## name(__VA_ARGS__); \
    namespace mm_namespace_ ## name { \
        using namespace ::multimethods::detail; \
        \
        using proto_t        = decltype(&mm_proto_ ## name); \
        using proto_traits_t = function_traits<proto_t>; \
        using base1_t        = proto_traits_t::arg1_type; \
        using base2_t        = proto_traits_t::arg2_type; \
        using base3_t        = proto_traits_t::arg3_type; \
        using base4_t        = proto_traits_t::arg4_type; \
        using base5_t        = proto_traits_t::arg5_type; \
        using base6_t        = proto_traits_t::arg6_type; \
        using ret_type_t     = proto_traits_t::ret_type; \
        using method_t       = abstract_method<ret_type_t, base1_t, base2_t, base3_t, base4_t, base5_t, base6_t>; \
        \
        static constexpr int MM_MAX_IMPLEMENTATIONS = 64; \
        \
        static inline method_t* g_fallback { nullptr }; \
        static inline method_t* g_impls[MM_MAX_IMPLEMENTATIONS]; \
        static inline method_t** g_impls_end { g_impls }; \
    }; \
    \
    template<class... Args> inline \
    enable_if_t<std::is_invocable_v<mm_namespace_ ## name::proto_t, Args...>, mm_namespace_ ## name::ret_type_t> \
    name(Args&&... args) { \
        using namespace ::multimethods::detail; \
        using namespace mm_namespace_ ## name; \
        \
        for(auto m = g_impls ; m != g_impls_end ; ++m) \
            try { \
                if(auto r = (*m)->call(args...)) \
                    return method_result<ret_type_t>::unwrap(r); \
            } catch(try_next&) { \
            } \
        \
        if(g_fallback) { \
            if(auto r = g_fallback->call(g_dummy_fallback)) \
                return method_result<ret_type_t>::unwrap(r); \
        } \
        \
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
// Finishes definition of a method.
//
//   define_method(collide)
//       match(asteroid&, asteroid&) {}
//       match(asteroid&, spaceship&) {}
//   end_method
//
#define end_method \
            }; \
            ::multimethods::detail::sort_functions sorter(funcs); \
            const auto methods = sorter.sort_methods<proto_t, ret_type_t, base1_t, base2_t, base3_t, base4_t, base5_t, base6_t>(); \
            \
            for(auto it: methods) \
                if(it->is_fallback()) { \
                    g_fallback = it; \
                } else { \
                    *g_impls_end++ = it; \
                } \
            \
            return true; \
        }(); \
    }

/**********************************************************************************************/
// Adds fallback handler for a method.
//
//   define_method(collide, thing&, thing&)
//      fallback {}
//
#define fallback , +[](::multimethods::detail::fallback_t) -> ret_type_t

/**********************************************************************************************/
// Skips current method in runtime to search for more suitable implementation.
//
//   match(int n) { if(n > 0) next_method; return -n; }
//
#define next_method throw ::multimethods::detail::try_next();


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
namespace detail {

/**********************************************************************************************/
using namespace std;

/**********************************************************************************************/
struct fallback_t {}; // Special type to use as parameter of a fallback function (to detect it in a list with implementations).

/**********************************************************************************************/
static inline fallback_t g_dummy_fallback;


/**********************************************************************************************/
// An argument for polymorphic and non-const type.
//
template<class B>
struct arg_poly_non_const {
    B* const base_;

    // Dummy constructor for fallback.
    arg_poly_non_const(fallback_t)
    : base_(nullptr) {
    }

    // Derived class - can use static_cast
    template<class T, class = enable_if_t<is_base_of_v<B, decay_t<T>>>>
    arg_poly_non_const(T&& v)
    : base_(const_cast<B*>(static_cast<const B*>(&v))) {
    }

    // Non-related type - need to use dynamic_cast
    template<class T, class = enable_if_t<!is_base_of_v<B, decay_t<T>>>, class = void>
    arg_poly_non_const(T&& v)
    : base_(const_cast<B*>(dynamic_cast<const B*>(&v))) {
    }

    template<class T>
    remove_reference_t<T>* cast() {
        if constexpr(!is_same_v<decay_t<T>, fallback_t>)
            return dynamic_cast<decay_t<T>*>(base_);
        else
            return &g_dummy_fallback;
    }
};

/**********************************************************************************************/
// An argument for polymorphic and const type.
//
template<class B>
struct arg_poly_const {
    B* base_;
    bool const_;

    // Dummy constructor for fallback.
    arg_poly_const(fallback_t)
    : base_(nullptr)
    , const_(false) {
    }

    // Derived class - can use static_cast
    template<class T, class = enable_if_t<is_base_of_v<B, decay_t<T>>>>
    arg_poly_const(T&& v)
    : base_(static_cast<B*>(&v))
    , const_(is_const_v<remove_reference_t<T>>) {
    }

    // Non-related type - need to use dynamic_cast
    template<class T, class = enable_if_t<!is_base_of_v<B, decay_t<T>>>, class = void>
    arg_poly_const(T&& v)
    : base_(dynamic_cast<B*>(&v))
    , const_(is_const_v<remove_reference_t<T>>) {
    }

    template<class T>
    remove_reference_t<T>* cast() {
        if constexpr(!is_same_v<decay_t<T>, fallback_t>) {
            if(is_const_v<remove_reference_t<T>> || !const_ )
                return const_cast<remove_reference_t<T>*>(dynamic_cast<const remove_reference_t<T>*>(base_));
        } else
            return &g_dummy_fallback;

        return nullptr;
    }
};

/**********************************************************************************************/
// An argument for non-polymorphic and non-const type.
//
template<class B>
struct arg_non_poly_non_const {
    B* p_;

    // Dummy constructor for fallback.
    arg_non_poly_non_const(fallback_t)
    : p_(nullptr) {
    }

    arg_non_poly_non_const(const B& v)
    : p_(const_cast<B*>(&v)) {
    }

    arg_non_poly_non_const(B& v)
    : p_(&v) {
    }

    arg_non_poly_non_const(B&& v)
    : p_(&v) {
    }

    template<class T>
    remove_reference_t<T>* cast() {
        if constexpr(!is_same_v<decay_t<T>, fallback_t>)
            return p_;
        else
            return &g_dummy_fallback;
    }
};

/**********************************************************************************************/
// An argument for non-polymorphic and const type.
//
template<class U, class B = remove_const_t<U>>
struct arg_non_poly_const {
    bool const_;
    B* p_;

    // Dummy constructor for fallback.
    arg_non_poly_const(const fallback_t&)
    : p_(nullptr) {
    }

    arg_non_poly_const(B& v)
    : const_(false)
    , p_(const_cast<B*>(&v)) {
    }

    arg_non_poly_const(B&& v)
    : const_(false)
    , p_(&v) {
    }

    arg_non_poly_const(const B& v)
    : const_(true)
    , p_(const_cast<B*>(&v)) {
    }

    template<class T>
    remove_reference_t<T>* cast() {
        if constexpr(!is_same_v<decay_t<T>, fallback_t>) {
            if(is_const_v<remove_reference_t<T>> || !const_ )
                return const_cast<remove_reference_t<T>*>(p_);
        } else
            return &g_dummy_fallback;

        return nullptr;
    }
};

/**********************************************************************************************/
// Dummy void argument.
//
struct arg_void {
    template<class T>
    remove_reference_t<T>* cast() { return nullptr; }
};

/**********************************************************************************************/
// An argument for an implementation.
//
template<
    class B,
    class U = remove_reference_t<B>,
    class S = conditional_t<
                is_same_v<B, void>,
                    arg_void,
                    conditional_t<is_polymorphic_v<decay_t<U>>,
                        conditional_t<is_const_v<remove_reference_t<U>>, arg_poly_const<U>, arg_poly_non_const<U>>,
                        conditional_t<is_const_v<remove_reference_t<U>>, arg_non_poly_const<U>, arg_non_poly_non_const<U>>>>
>
struct arg final : S {
    arg(const fallback_t&) {}

    template<class T>
    arg(T&& v) : S(std::forward<T>(v)) {}
};

/**********************************************************************************************/
// Exception to skip an implementation and try next one.
//
struct try_next final : std::exception {
    virtual const char* what() const noexcept { return "next_method"; }
};


/**********************************************************************************************/
// Helper class to wrap a reference.
//
template<class U>
struct optional_ref {
    optional_ref() : p_(nullptr) {}
    optional_ref(U v) : p_(&v) {}

    operator bool() const { return p_; }
    U operator *() const { return static_cast<U>(*p_); }

    remove_reference_t<U>* p_;
};

/**********************************************************************************************/
template<class T, class B = conditional_t<
    is_reference_v<T>,
    optional_ref<T>,
    optional<remove_reference_t<T>>>
>
struct method_ret_type : B {
    method_ret_type() {}

    template<class U>
    method_ret_type(U&& v) : B(std::forward<U>(v)) {}
};


/**********************************************************************************************/
// Base class for methods' implementations.
//
template<class T, class B1, class B2, class B3, class B4, class B5, class B6>
struct abstract_method {
    using ret_t = conditional_t<is_same_v<T, void>, bool, method_ret_type<T>>;
    virtual ~abstract_method() {}
    virtual ret_t call() { return {}; }
    virtual ret_t call(arg<B1>) { return {}; }
    virtual ret_t call(arg<B1>, arg<B2>) { return {}; }
    virtual ret_t call(arg<B1>, arg<B2>, arg<B3>) { return {}; }
    virtual ret_t call(arg<B1>, arg<B2>, arg<B3>, arg<B4>) { return {}; }
    virtual ret_t call(arg<B1>, arg<B2>, arg<B3>, arg<B4>, arg<B5>) { return {}; }
    virtual ret_t call(arg<B1>, arg<B2>, arg<B3>, arg<B4>, arg<B5>, arg<B6>) { return {}; }
    virtual bool is_fallback() const { return false; }
};


/**********************************************************************************************/
template<class... Args> constexpr enable_if_t<sizeof...(Args) == 0, bool> check_types() { return false; }
template<class T1> constexpr bool check_types() { return false; }
template<class T1, class T2, class T3> constexpr bool check_types() { return false; }
template<class T1, class T2, class T3, class T4, class T5> constexpr bool check_types() { return false; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7> constexpr bool check_types() { return false; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> constexpr bool check_types() { return false; }
template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11> constexpr bool check_types() { return false; }

/**********************************************************************************************/
template<class T, class U>
constexpr bool check_types() {
    if constexpr(is_same_v<decay_t<T>, decay_t<U>>)
        return true;
    else
        static_assert(is_polymorphic_v<decay_t<T>> && is_base_of_v<decay_t<T>, decay_t<U>>, "Implementation can extend parameter's type, but not replace it.");

    return true;
}

/**********************************************************************************************/
template<class T1, class T2, class U1, class U2>
constexpr int check_types() {
    return check_types<T1, U1>() &&
           check_types<T2, U2>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class U1, class U2, class U3>
constexpr int check_types() {
    return check_types<T1, U1>() &&
           check_types<T2, T3, U2, U3>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class T4, class U1, class U2, class U3, class U4>
constexpr int check_types() {
    return check_types<T1, U1>() &&
           check_types<T2, T3, T4, U2, U3, U4>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class T4, class T5, class U1, class U2, class U3, class U4, class U5>
constexpr int check_types() {
    return check_types<T1, U1>() &&
           check_types<T2, T3, T4, T5, U2, U3, U4, U5>();
}

/**********************************************************************************************/
template<class T1, class T2, class T3, class T4, class T5, class T6, class U1, class U2, class U3, class U4, class U5, class U6>
constexpr int check_types() {
    return check_types<T1, U1>() &&
           check_types<T2, T3, T4, T5, T6, U2, U3, U4, U5, U6>();
}

/**********************************************************************************************/
template<class F1, class F2>
struct check_parameters_impl;

/**********************************************************************************************/
template<class R, class... Args1, class... Args2>
struct check_parameters_impl<R(*)(Args1...), R(*)(Args2...)> {
    static constexpr bool value = check_types<Args1..., Args2...>();
};

/**********************************************************************************************/
template<class R, class... Args>
struct check_parameters_impl<R(*)(Args...), R(*)(fallback_t)> {
    static constexpr bool value = true;
};

/**********************************************************************************************/
template<class F1, class F2>
struct check_parameters : public check_parameters_impl<F1, F2> {
};


/**********************************************************************************************/
template<class... Args> constexpr enable_if_t<sizeof...(Args) == 0, int> compare_types() { return 0; }
template<class> constexpr int compare_types() { return 0; }
template<class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class, class, class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class, class, class, class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class, class, class, class, class, class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class, class, class, class, class, class, class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class, class, class, class, class, class, class, class, class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class, class, class, class, class, class, class, class, class, class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> constexpr int compare_types() { return 0; }
template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> constexpr int compare_types() { return 0; }

/**********************************************************************************************/
template<class B, class T, class U>
constexpr int compare_types() {
    using BD = decay_t<B>;
    using TD = decay_t<T>;
    using UD = decay_t<U>;

    // The same types - just check constness
    if constexpr(is_same_v<TD, UD>) {
        if constexpr(is_const_v<remove_reference_t<T>>)
            return is_const_v<remove_reference_t<U>> ? 0 : 1;
        if constexpr(is_const_v<remove_reference_t<U>>)
            return -1;

        return 0;
    } else {
        // Prefer derived classes
        if constexpr (is_base_of_v<decay_t<T>, decay_t<U>>)
            return 1;
        else if constexpr(is_base_of_v<decay_t<U>, decay_t<T>>)
            return -1;
    }

    return 0;
}

/**********************************************************************************************/
template<class B1, class B2, class T1, class T2, class U1, class U2>
constexpr int compare_types() {
    if(constexpr int r = compare_types<B1, T1, U1>() )
        return r;
    return compare_types<B2, T2, U2>();
}

/**********************************************************************************************/
template<class B1, class B2, class B3, class T1, class T2, class T3, class U1, class U2, class U3>
constexpr int compare_types() {
    if(constexpr int r = compare_types<B1, T1, U1>() )
        return r;
    return compare_types<B2, B3, T2, T3, U2, U3>();
}

/**********************************************************************************************/
template<class B1, class B2, class B3, class B4, class T1, class T2, class T3, class T4, class U1, class U2, class U3, class U4>
constexpr int compare_types() {
    if(constexpr int r = compare_types<T1, U1>() )
        return r;
    return compare_types<B2, B3, B4, T2, T3, T4, U2, U3, U4>();
}

/**********************************************************************************************/
template<class B1, class B2, class B3, class B4, class B5, class T1, class T2, class T3, class T4, class T5, class U1, class U2, class U3, class U4, class U5>
constexpr int compare_types() {
    if(constexpr int r = compare_types<T1, U1>() )
        return r;
    return compare_types<B2, B3, B4, B5, T2, T3, T4, T5, U2, U3, U4, U5>();
}

/**********************************************************************************************/
template<class B1, class B2, class B3, class B4, class B5, class B6, class T1, class T2, class T3, class T4, class T5, class T6, class U1, class U2, class U3, class U4, class U5, class U6>
constexpr int compare_types() {
    if(constexpr int r = compare_types<T1, U1>() )
        return r;
    return compare_types<B2, B3, B4, B5, B6, T2, T3, T4, T5, T6, U2, U3, U4, U5, U6>();
}

/**********************************************************************************************/
template<class P, class F1, class F2>
struct compare_methods_impl;

/**********************************************************************************************/
template<class R, class... PArgs, class... Args1, class... Args2>
struct compare_methods_impl<R(*)(PArgs...), R(*)(Args1...), R(*)(Args2...)> {
    static constexpr int value = compare_types<PArgs..., Args1..., Args2...>();
};

/**********************************************************************************************/
template<class P, class F1, class F2>
struct compare_methods : public compare_methods_impl<P, F1, F2> {
};


/**********************************************************************************************/
template<class F>
struct function_traits_impl;

/**********************************************************************************************/
template<class R>
struct function_traits_impl<R(*)()> {
    enum { arity = 0 };
    using ret_type = R;
    using arg1_type = void;
    using arg2_type = void;
    using arg3_type = void;
    using arg4_type = void;
    using arg5_type = void;
    using arg6_type = void;
};

/**********************************************************************************************/
template<class R, class T1>
struct function_traits_impl<R(*)(T1)> {
    enum { arity = 1 };
    using ret_type = R;
    using arg1_type = T1;
    using arg2_type = void;
    using arg3_type = void;
    using arg4_type = void;
    using arg5_type = void;
    using arg6_type = void;
};

/**********************************************************************************************/
template<class R, class T1, class T2>
struct function_traits_impl<R(*)(T1, T2)> {
    enum { arity = 2 };
    using ret_type = R;
    using arg1_type = T1;
    using arg2_type = T2;
    using arg3_type = void;
    using arg4_type = void;
    using arg5_type = void;
    using arg6_type = void;
};

/**********************************************************************************************/
template<class R, class T1, class T2, class T3>
struct function_traits_impl<R(*)(T1, T2, T3)> {
    enum { arity = 3 };
    using ret_type = R;
    using arg1_type = T1;
    using arg2_type = T2;
    using arg3_type = T3;
    using arg4_type = void;
    using arg5_type = void;
    using arg6_type = void;
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
    using arg5_type = void;
    using arg6_type = void;
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
    using arg6_type = void;
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
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_0 final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_0(F f) : f_(f) {}

    typename abstract_method<T, B1, B2, B3, B4, B5, B6>::ret_t call() { return f_(); }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_0_void final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_0_void(F f) : f_(f) {}

    bool call() { f_(); return true; }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_1 final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_1(F f) : f_(f) {}

    typename abstract_method<T, B1, B2, B3, B4, B5, B6>::ret_t call(arg<B1> p1) {
        MM_CAST_1 return f_(*u1);
        return {};
    }

    virtual bool is_fallback() const { return is_same_v<fallback_t, typename function_traits<F>::arg1_type>; }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_1_void final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_1_void(F f) : f_(f) {}

    bool call(arg<B1> p1) {
        MM_CAST_1 { f_(*u1); return true; }
        return false;
    }

    virtual bool is_fallback() const { return is_same_v<fallback_t, typename function_traits<F>::arg1_type>; }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_2 final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_2(F f) : f_(f) {}

    typename abstract_method<T, B1, B2, B3, B4, B5, B6>::ret_t call(arg<B1> p1, arg<B2> p2) {
        MM_CAST_2 return f_(*u1, *u2);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_2_void final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_2_void(F f) : f_(f) {}

    bool call(arg<B1> p1, arg<B2> p2) {
        MM_CAST_2 { f_(*u1, *u2); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_3 final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_3(F f) : f_(f) {}

    typename abstract_method<T, B1, B2, B3, B4, B5, B6>::ret_t call(arg<B1> p1, arg<B2> p2, arg<B3> p3) {
        MM_CAST_3 return f_(*u1, *u2, *u3);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_3_void final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_3_void(F f) : f_(f) {}

    bool call(arg<B1> p1, arg<B2> p2, arg<B3> p3) {
        MM_CAST_3 { f_(*u1, *u2, *u3); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_4 final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_4(F f) : f_(f) {}

    typename abstract_method<T, B1, B2, B3, B4, B5, B6>::ret_t call(arg<B1> p1, arg<B2> p2, arg<B3> p3, arg<B4> p4) {
        MM_CAST_4 return f_(*u1, *u2, *u3, *u4);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_4_void final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_4_void(F f) : f_(f) {}

    bool call(arg<B1> p1, arg<B2> p2, arg<B3> p3, arg<B4> p4) {
        MM_CAST_4 { f_(*u1, *u2, *u3, *u4); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_5 final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_5(F f) : f_(f) {}

    typename abstract_method<T, B1, B2, B3, B4, B5, B6>::ret_t call(arg<B1> p1, arg<B2> p2, arg<B3> p3, arg<B4> p4, arg<B5> p5) {
        MM_CAST_5 return f_(*u1, *u2, *u3, *u4, *u5);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_5_void final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_5_void(F f) : f_(f) {}

    bool call(arg<B1> p1, arg<B2> p2, arg<B3> p3, arg<B4> p4, arg<B5> p5) {
        MM_CAST_5 { f_(*u1, *u2, *u3, *u4, *u5); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_6 final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_6(F f) : f_(f) {}

    typename abstract_method<T, B1, B2, B3, B4, B5, B6>::ret_t call(arg<B1> p1, arg<B2> p2, arg<B3> p3, arg<B4> p4, arg<B5> p5, arg<B6> p6) {
        MM_CAST_6 return f_(*u1, *u2, *u3, *u4, *u5, *u6);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B1, class B2, class B3, class B4, class B5, class B6, class F>
struct method_6_void final : abstract_method<T, B1, B2, B3, B4, B5, B6> {
    F f_;
    method_6_void(F f) : f_(f) {}

    bool call(arg<B1> p1, arg<B2> p2, arg<B3> p3, arg<B4> p4, arg<B5> p5, arg<B6> p6) {
        MM_CAST_6 { f_(*u1, *u2, *u3, *u4, *u5, *u6); return true; }
        return false;
    }
};


/**********************************************************************************************/
#define MM_MAKE_METHOD(N) \
    template<class P, class T, class B1, class B2, class B3, class B4, class B5, class B6, class F> inline \
    auto make_method(F f) -> enable_if_t<function_traits<F>::arity == N && !is_same_v<void, typename function_traits<F>::ret_type>, abstract_method<T, B1, B2, B3, B4, B5, B6>*> { \
        static_assert(function_traits<F>::arity == function_traits<P>::arity || is_same_v<typename function_traits<F>::arg1_type, fallback_t>, "Implementation's parameters count mismatch."); \
        static_assert(check_parameters<P, F>::value, "Incompatible implementation's parameters types."); \
        return new method_ ## N<T, B1, B2, B3, B4, B5, B6, F>(f); \
    } \
    \
    template<class P, class T, class B1, class B2, class B3, class B4, class B5, class B6, class F> inline \
    auto make_method(F f) -> enable_if_t<function_traits<F>::arity == N && is_same_v<void, typename function_traits<F>::ret_type>, abstract_method<T, B1, B2, B3, B4, B5, B6>*> { \
        static_assert(function_traits<F>::arity == function_traits<P>::arity || is_same_v<typename function_traits<F>::arg1_type, fallback_t>, "Implementation's parameters count mismatch."); \
        static_assert(check_parameters<P, F>::value, "Incompatible implementation's parameters types."); \
        return new method_ ## N ## _void<T, B1, B2, B3, B4, B5, B6, F>(f); \
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
    static T unwrap(method_ret_type<T>& o) { return *o; }
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
    static_assert(N>0, "Expected atleasy one implementation.");

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
    MM_FUNC_TYPE(32); MM_FUNC_TYPE(33); MM_FUNC_TYPE(34); MM_FUNC_TYPE(35);
    MM_FUNC_TYPE(36); MM_FUNC_TYPE(37); MM_FUNC_TYPE(38); MM_FUNC_TYPE(39);
    MM_FUNC_TYPE(40); MM_FUNC_TYPE(41); MM_FUNC_TYPE(42); MM_FUNC_TYPE(43);
    MM_FUNC_TYPE(44); MM_FUNC_TYPE(45); MM_FUNC_TYPE(46); MM_FUNC_TYPE(47);
    MM_FUNC_TYPE(48); MM_FUNC_TYPE(49); MM_FUNC_TYPE(50); MM_FUNC_TYPE(51);
    MM_FUNC_TYPE(52); MM_FUNC_TYPE(53); MM_FUNC_TYPE(54); MM_FUNC_TYPE(55);
    MM_FUNC_TYPE(56); MM_FUNC_TYPE(57); MM_FUNC_TYPE(58); MM_FUNC_TYPE(59);
    MM_FUNC_TYPE(60); MM_FUNC_TYPE(61); MM_FUNC_TYPE(62); MM_FUNC_TYPE(63);
    MM_FUNC_TYPE(64);

    #undef MM_FUNC_TYPE

    // Predicate function to sort methods
    template<class P, class A>
    static constexpr bool pred_b(int b) {
        #define MM_CASE_B(I) \
            if(b == I) return compare_methods<P, A, F ## I>::value < 0;

        MM_CASE_B(0); MM_CASE_B(1); MM_CASE_B(2); MM_CASE_B(3);
        MM_CASE_B(4); MM_CASE_B(5); MM_CASE_B(6); MM_CASE_B(7);
        MM_CASE_B(8); MM_CASE_B(9); MM_CASE_B(10); MM_CASE_B(11);
        MM_CASE_B(12); MM_CASE_B(13); MM_CASE_B(14); MM_CASE_B(15);
        MM_CASE_B(16); MM_CASE_B(17); MM_CASE_B(18); MM_CASE_B(19);
        MM_CASE_B(20); MM_CASE_B(21); MM_CASE_B(22); MM_CASE_B(23);
        MM_CASE_B(24); MM_CASE_B(25); MM_CASE_B(26); MM_CASE_B(27);
        MM_CASE_B(28); MM_CASE_B(29); MM_CASE_B(30); MM_CASE_B(31);
        MM_CASE_B(32); MM_CASE_B(33); MM_CASE_B(34); MM_CASE_B(35);
        MM_CASE_B(36); MM_CASE_B(37); MM_CASE_B(38); MM_CASE_B(39);
        MM_CASE_B(40); MM_CASE_B(41); MM_CASE_B(42); MM_CASE_B(43);
        MM_CASE_B(44); MM_CASE_B(45); MM_CASE_B(46); MM_CASE_B(47);
        MM_CASE_B(48); MM_CASE_B(49); MM_CASE_B(50); MM_CASE_B(51);
        MM_CASE_B(52); MM_CASE_B(53); MM_CASE_B(54); MM_CASE_B(55);
        MM_CASE_B(56); MM_CASE_B(57); MM_CASE_B(58); MM_CASE_B(59);
        MM_CASE_B(60); MM_CASE_B(61); MM_CASE_B(62); MM_CASE_B(63);
        MM_CASE_B(64);

        #undef MM_CASE_B

        return false;
    }

    // Sorts methods and return vector with instances of 'abstract_method'
    template<class TP, class TR, class BR1, class BR2, class BR3, class BR4, class BR5, class BR6>
    auto sort_methods() {
        // Use indexes cause we cannot sort tuple itself
        int indexes[N];
        for(int i = 0 ; i < N ; ++i)
            indexes[i] = i;

        // Use quick-sort to sort methods by types and it's constness
        std::sort(indexes, indexes + N, [](int a, int b) {
            #define MM_CASE_A(I) \
                if(a == I) return sort_functions::pred_b<TP, F ## I>(b)

            MM_CASE_A(0); MM_CASE_A(1); MM_CASE_A(2); MM_CASE_A(3);
            MM_CASE_A(4); MM_CASE_A(5); MM_CASE_A(6); MM_CASE_A(7);
            MM_CASE_A(8); MM_CASE_A(9); MM_CASE_A(10); MM_CASE_A(11);
            MM_CASE_A(12); MM_CASE_A(13); MM_CASE_A(14); MM_CASE_A(15);
            MM_CASE_A(16); MM_CASE_A(17); MM_CASE_A(18); MM_CASE_A(19);
            MM_CASE_A(20); MM_CASE_A(21); MM_CASE_A(22); MM_CASE_A(23);
            MM_CASE_A(24); MM_CASE_A(25); MM_CASE_A(26); MM_CASE_A(27);
            MM_CASE_A(28); MM_CASE_A(29); MM_CASE_A(30); MM_CASE_A(31);
            MM_CASE_A(32); MM_CASE_A(33); MM_CASE_A(34); MM_CASE_A(35);
            MM_CASE_A(36); MM_CASE_A(37); MM_CASE_A(38); MM_CASE_A(39);
            MM_CASE_A(40); MM_CASE_A(41); MM_CASE_A(42); MM_CASE_A(43);
            MM_CASE_A(44); MM_CASE_A(45); MM_CASE_A(46); MM_CASE_A(47);
            MM_CASE_A(48); MM_CASE_A(49); MM_CASE_A(50); MM_CASE_A(51);
            MM_CASE_A(52); MM_CASE_A(53); MM_CASE_A(54); MM_CASE_A(55);
            MM_CASE_A(56); MM_CASE_A(57); MM_CASE_A(58); MM_CASE_A(59);
            MM_CASE_A(60); MM_CASE_A(61); MM_CASE_A(62); MM_CASE_A(63);
            MM_CASE_A(64);

            #undef MM_CASE_A

            return a < b;
        } );

        // Create and fill vector with methods
        std::vector<abstract_method<TR, BR1, BR2, BR3, BR4, BR5, BR6>*> r(N);

        for(int i = 0 ; i < N ; ++i) {
            switch(indexes[i]) {
                #define MM_FILL_VECTOR(I) \
                    case I: r[i] = make_method<TP, TR, BR1, BR2, BR3, BR4, BR5, BR6, F ## I>(std::get<I < N ? I + 1 : 1>(funcs_)); break

                MM_FILL_VECTOR(0); MM_FILL_VECTOR(1); MM_FILL_VECTOR(2); MM_FILL_VECTOR(3);
                MM_FILL_VECTOR(4); MM_FILL_VECTOR(5); MM_FILL_VECTOR(6); MM_FILL_VECTOR(7);
                MM_FILL_VECTOR(8); MM_FILL_VECTOR(9); MM_FILL_VECTOR(10); MM_FILL_VECTOR(11);
                MM_FILL_VECTOR(12); MM_FILL_VECTOR(13); MM_FILL_VECTOR(14); MM_FILL_VECTOR(15);
                MM_FILL_VECTOR(16); MM_FILL_VECTOR(17); MM_FILL_VECTOR(18); MM_FILL_VECTOR(19);
                MM_FILL_VECTOR(20); MM_FILL_VECTOR(21); MM_FILL_VECTOR(22); MM_FILL_VECTOR(23);
                MM_FILL_VECTOR(24); MM_FILL_VECTOR(25); MM_FILL_VECTOR(26); MM_FILL_VECTOR(27);
                MM_FILL_VECTOR(28); MM_FILL_VECTOR(29); MM_FILL_VECTOR(30); MM_FILL_VECTOR(31);
                MM_FILL_VECTOR(32); MM_FILL_VECTOR(33); MM_FILL_VECTOR(34); MM_FILL_VECTOR(35);
                MM_FILL_VECTOR(36); MM_FILL_VECTOR(37); MM_FILL_VECTOR(38); MM_FILL_VECTOR(39);
                MM_FILL_VECTOR(40); MM_FILL_VECTOR(41); MM_FILL_VECTOR(42); MM_FILL_VECTOR(43);
                MM_FILL_VECTOR(44); MM_FILL_VECTOR(45); MM_FILL_VECTOR(46); MM_FILL_VECTOR(47);
                MM_FILL_VECTOR(48); MM_FILL_VECTOR(49); MM_FILL_VECTOR(50); MM_FILL_VECTOR(51);
                MM_FILL_VECTOR(52); MM_FILL_VECTOR(53); MM_FILL_VECTOR(54); MM_FILL_VECTOR(55);
                MM_FILL_VECTOR(56); MM_FILL_VECTOR(57); MM_FILL_VECTOR(58); MM_FILL_VECTOR(59);
                MM_FILL_VECTOR(60); MM_FILL_VECTOR(61); MM_FILL_VECTOR(62); MM_FILL_VECTOR(63);
                MM_FILL_VECTOR(64);

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

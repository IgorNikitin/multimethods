/**********************************************************************************************/
// multimethods.h
//
// Igor Nikitin, 2017.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
/**********************************************************************************************/

#include <optional>
#include <string>
#include <typeindex>
#include <vector>


/**********************************************************************************************/
// Declares a new multimethod and optionally specifies it's result type and base class
// for polymorphic types.
//
//   declare_method(collide)
//   declare_method(concat, string)
//   declare_method(concat, string, I_Unknown)
//
#define declare_method(name, ...) \
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

/**********************************************************************************************/
// Adds implementation of a multimethod.
//
//   declare_method(collide)
//   define_method(collide, asteroid&, spaceship&) { cout << "Boom!\n"; }
//
#define define_method(name, ...) \
   static g_mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)(__VA_ARGS__); \
   static_assert(::multimethods::detail::check_base_class<g_mm_b_ ## name, decltype(*MM_JOIN(_mm_impl_, __LINE__))>::value, \
   "For polymorphic types in a parameters list need to specify common base class on call 'declare_method' function, or use 'multimethods::unknown' as base class."); \
   static bool MM_JOIN(_mm_init_, __LINE__) = []{ g_mm_f_ ## name ::funcs_.push_back(::multimethods::detail::make_method<g_mm_r_ ## name, g_mm_b_ ## name, MM_JOIN(_mm_impl_, __LINE__)>()); return true; }(); \
   static g_mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)(__VA_ARGS__)

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
struct function_traits : public function_traits_impl<typename std::add_pointer<F>::type> {
};


/**********************************************************************************************/
template<class T, class B, auto F>
struct method_0 final : abstract_method<T, B> {
    typename abstract_method<T, B>::ret_t call() { return F(); }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_0_void final : abstract_method<T, B> {
    bool call() { F(); return true; }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_1 final : abstract_method<T, B> {
    typename abstract_method<T, B>::ret_t call(arg<B> p) {
        if(auto u1 = p.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            return F(*u1);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_1_void final : abstract_method<T, B> {
    bool call(arg<B> p) {
        if(auto u1 = p.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            { F(*u1); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_2 final : abstract_method<T, B> {
    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2) {
        if(auto u1 = p1.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<decltype(*F)>::arg2_type>())
                return F(*u1, *u2);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_2_void final : abstract_method<T, B> {
    bool call(arg<B> p1, arg<B> p2) {
        if(auto u1 = p1.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<decltype(*F)>::arg2_type>())
                { F(*u1, *u2); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_3 final : abstract_method<T, B> {
    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3) {
        if(auto u1 = p1.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<decltype(*F)>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<decltype(*F)>::arg3_type>())
                        return F(*u1, *u2, *u3);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_3_void final : abstract_method<T, B> {
    bool call(arg<B> p1, arg<B> p2, arg<B> p3) {
        if(auto u1 = p1.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<decltype(*F)>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<decltype(*F)>::arg3_type>())
                    { F(*u1, *u2, *u3); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_4 final : abstract_method<T, B> {
    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4) {
        if(auto u1 = p1.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<decltype(*F)>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<decltype(*F)>::arg3_type>())
                    if(auto u4 = p4.template cast<typename function_traits<decltype(*F)>::arg4_type>())
                        return F(*u1, *u2, *u3, *u4);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_4_void final : abstract_method<T, B> {
    bool call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4) {
        if(auto u1 = p1.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<decltype(*F)>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<decltype(*F)>::arg3_type>())
                    if(auto u4 = p4.template cast<typename function_traits<decltype(*F)>::arg4_type>())
                        { F(*u1, *u2, *u3, *u4); return true; }
        return false;
    }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_5 final : abstract_method<T, B> {
    typename abstract_method<T, B>::ret_t call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4, arg<B> p5) {
        if(auto u1 = p1.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<decltype(*F)>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<decltype(*F)>::arg3_type>())
                    if(auto u4 = p4.template cast<typename function_traits<decltype(*F)>::arg4_type>())
                        if(auto u5 = p5.template cast<typename function_traits<decltype(*F)>::arg5_type>())
                            return F(*u1, *u2, *u3, *u4, *u5);
        return {};
    }
};

/**********************************************************************************************/
template<class T, class B, auto F>
struct method_5_void final : abstract_method<T, B> {
    bool call(arg<B> p1, arg<B> p2, arg<B> p3, arg<B> p4) {
        if(auto u1 = p1.template cast<typename function_traits<decltype(*F)>::arg1_type>())
            if(auto u2 = p2.template cast<typename function_traits<decltype(*F)>::arg2_type>())
                if(auto u3 = p3.template cast<typename function_traits<decltype(*F)>::arg3_type>())
                    if(auto u4 = p4.template cast<typename function_traits<decltype(*F)>::arg4_type>())
                        if(auto u5 = p4.template cast<typename function_traits<decltype(*F)>::arg5_type>())
                            { F(*u1, *u2, *u3, *u4, *u5); return true; }
        return false;
    }
};


/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 0 && !std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_0<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 0 && std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_0_void<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 1 && !std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_1<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 1 && std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_1_void<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 2 && !std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_2<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 2 && std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_2_void<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 3 && !std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_3<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 3 && std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_3_void<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 4 && !std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_4<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 4 && std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_4_void<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 5 && !std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_5<T, B, F>();
}

/**********************************************************************************************/
template<class T, class B, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 5 && std::is_same<void, typename function_traits<decltype(*F)>::ret_type>::value, abstract_method<T, B>*>::type {
    return new method_5_void<T, B, F>();
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

/**********************************************************************************************/
} }

/**********************************************************************************************/
// Helper macros for 'define_method' macro.
//
#define MM_JOIN(x, y) MM_JOIN_AGAIN(x, y)
#define MM_JOIN_AGAIN(x, y) x ## y

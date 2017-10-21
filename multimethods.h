/**********************************************************************************************/
// multimethods.h
//
// Igor Nikitin, 2017.
// Public domain.
/**********************************************************************************************/

#include <string>
#include <typeindex>
#include <vector>


/**********************************************************************************************/
// Declares a new multimethod and specifies it's result type (void by default).
//
//   declare_method(collide)
//   declare_method(concat, string)
//
#define declare_method(name, ...) \
    using _mm_r_ ## name = ::multimethods::detail::method_type<__VA_ARGS__>::type; \
    struct _mm_f_ ## name { \
        static inline std::vector<::multimethods::detail::abstract_method<_mm_r_ ## name>*> funcs_; \
        static inline _mm_r_ ## name(*fallback_)() { nullptr }; \
    }; \
    template<class... Args> inline \
    _mm_r_ ## name name(Args&&... args) { \
        for( auto m : _mm_f_ ## name ::funcs_ ) \
            try { \
                return m->call(args...); \
            } catch(::multimethods::not_match&) { \
            } \
        return _mm_f_ ## name ::fallback_ \
            ? (*_mm_f_ ## name ::fallback_)() \
            : throw ::multimethods::not_implemented(#name ": not_implemented."); \
    } \

/**********************************************************************************************/
// Adds implementation of a multimethod.
//
//   declare_method(collide)
//   define_method(collide, asteroid&, spaceship&) { cout << "Boom!\n"; }
//
#define define_method(name, ...) \
   static _mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)(__VA_ARGS__); \
   static bool MM_JOIN(_mm_init_, __LINE__) = []{ _mm_f_ ## name ::funcs_.push_back(::multimethods::detail::make_method<_mm_r_ ## name, MM_JOIN(_mm_impl_, __LINE__)>()); return true; }(); \
   static _mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)(__VA_ARGS__)

/**********************************************************************************************/
// Adds fallback handler for a multimethod.
//
//   declare_method(collide)
//   define_method_fallback(collide) { cout << "All is fine.\n"; }
//
#define define_method_fallback(name) \
   static _mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)(); \
   static bool MM_JOIN(_mm_init_, __LINE__) = []{ _mm_f_ ## name ::fallback_ = MM_JOIN(_mm_impl_, __LINE__); return true; }(); \
   static _mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)()

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
// Class to store reference to an argument and cast it on call an implementation.
//
struct arg final {
    unknown* base_;
    bool const_;
    void* p_;
    std::type_index type_;

    // Constructs from polymorphic value - we can try to cast it to the 'unknown' now,
    // and cast to a destination class later.
    template<class T, class = typename std::enable_if<std::is_polymorphic<typename std::decay<T>::type>::value>::type>
    arg(T& v)
    : base_(dynamic_cast<unknown*>(&v))
    , const_(std::is_const<T>::value)
    , p_(const_cast<void*>(reinterpret_cast<const void*>(&v)))
    , type_(typeid(v)) {
    }

    // Constructs from non-polymorphic value.
    template<class T, class = typename std::enable_if<!std::is_polymorphic<typename std::decay<T>::type>::value>::type, class U = T>
    arg(T& v)
    : base_(nullptr)
    , const_(std::is_const<T>::value)
    , p_(const_cast<void*>(reinterpret_cast<const void*>(&v)))
    , type_(typeid(v)) {
    }

    // Cast to a polymorphic type
    template<class T>
    auto cast() -> typename std::enable_if<std::is_polymorphic<typename std::decay<T>::type>::value, T&>::type {
        if(std::is_const<T>::value || !const_ ) {
            if(type_ == typeid(T))
                return *reinterpret_cast<typename std::remove_reference<T>::type*>(p_);
            if(auto p = dynamic_cast<typename std::decay<T>::type*>(base_))
                return *p;
        }
        throw not_match();
    }

    // Cast to a non-polymorphic type
    template<class T>
    auto cast() -> typename std::enable_if<!std::is_polymorphic<typename std::decay<T>::type>::value, T&>::type {
        if(std::is_const<T>::value || !const_ ) {
            if(type_ == typeid(T))
                return *reinterpret_cast<typename std::remove_reference<T>::type*>(p_);
        }
        throw not_match();
    }
};

/**********************************************************************************************/
// Base class for methods' implementations.
//
template<class T>
struct abstract_method {
    virtual ~abstract_method() {}
    virtual T call() { throw not_match(); }
    virtual T call(arg) { throw not_match(); }
    virtual T call(arg, arg) { throw not_match(); }
    virtual T call(arg, arg, arg) { throw not_match(); }
    virtual T call(arg, arg, arg, arg) { throw not_match(); }
};


/**********************************************************************************************/
template<typename F>
struct function_traits_impl;

/**********************************************************************************************/
template<class R>
struct function_traits_impl<R(*)()> {
    enum { arity = 0 };
};

/**********************************************************************************************/
template<class R, class T1>
struct function_traits_impl<R(*)(T1)> {
    enum { arity = 1 };
    typedef T1 arg1_type;
};

/**********************************************************************************************/
template<class R, class T1, class T2>
struct function_traits_impl<R(*)(T1, T2)> {
    enum { arity = 2 };
    typedef T1 arg1_type;
    typedef T2 arg2_type;
};

/**********************************************************************************************/
template<class R, class T1, class T2, class T3>
struct function_traits_impl<R(*)(T1, T2, T3)> {
    enum { arity = 3 };
    typedef T1 arg1_type;
    typedef T2 arg2_type;
    typedef T3 arg3_type;
};

/**********************************************************************************************/
template<class R, class T1, class T2, class T3, class T4>
struct function_traits_impl<R(*)(T1, T2, T3, T4)> {
    enum { arity = 4 };
    typedef T1 arg1_type;
    typedef T2 arg2_type;
    typedef T3 arg3_type;
    typedef T4 arg4_type;
};

/**********************************************************************************************/
template<typename F>
struct function_traits : public function_traits_impl<typename std::add_pointer<F>::type> {
};


/**********************************************************************************************/
template<class T, auto F>
struct method_0 final : abstract_method<T> {
    T call() { return F(); }
};

/**********************************************************************************************/
template<class T, auto F>
struct method_1 final : abstract_method<T> {
    T call(arg p) {
        return F(p.cast<typename function_traits<decltype(*F)>::arg1_type>());
    }
};

/**********************************************************************************************/
template<class T, auto F>
struct method_2 final : abstract_method<T> {
    T call(arg p1, arg p2) {
        return F(p1.cast<typename function_traits<decltype(*F)>::arg1_type>(),
                 p2.cast<typename function_traits<decltype(*F)>::arg2_type>());
    }
};

/**********************************************************************************************/
template<class T, auto F>
struct method_3 final : abstract_method<T> {
    T call(arg p1, arg p2, arg p3) {
        return F(p1.cast<typename function_traits<decltype(*F)>::arg1_type>(),
                 p2.cast<typename function_traits<decltype(*F)>::arg2_type>(),
                 p3.cast<typename function_traits<decltype(*F)>::arg3_type>());
    }
};

/**********************************************************************************************/
template<class T, auto F>
struct method_4 final : abstract_method<T> {
    T call(arg p1, arg p2, arg p3, arg p4) {
        return F(p1.cast<typename function_traits<decltype(*F)>::arg1_type>(),
                 p2.cast<typename function_traits<decltype(*F)>::arg2_type>(),
                 p3.cast<typename function_traits<decltype(*F)>::arg3_type>(),
                 p4.cast<typename function_traits<decltype(*F)>::arg4_type>());
    }
};


/**********************************************************************************************/
template<class T, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 0, abstract_method<T>*>::type {
    return new method_0<T, F>();
}

/**********************************************************************************************/
template<class T, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 1, abstract_method<T>*>::type {
    return new method_1<T, F>();
}

/**********************************************************************************************/
template<class T, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 2, abstract_method<T>*>::type {
    return new method_2<T, F>();
}

/**********************************************************************************************/
template<class T, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 3, abstract_method<T>*>::type {
    return new method_3<T, F>();
}

/**********************************************************************************************/
template<class T, auto F> inline
auto make_method() -> typename std::enable_if<function_traits<decltype(*F)>::arity == 4, abstract_method<T>*>::type {
    return new method_4<T, F>();
}


/**********************************************************************************************/
template<class T=void>
struct method_type {
    using type=T;
};

/**********************************************************************************************/
} }

/**********************************************************************************************/
// Helper macros for 'define_method' macro.
//
#define MM_JOIN(x, y) MM_JOIN_AGAIN(x, y)
#define MM_JOIN_AGAIN(x, y) x ## y

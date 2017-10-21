/**********************************************************************************************/
// multimethods.h
//
// Igor Nikitin, 2017.
// Public domain.
/**********************************************************************************************/

#include <typeindex>
#include <vector>

/**********************************************************************************************/
#define declare_method(name, type) \
    using _mm_r_ ## name = type; \
    struct _mm_f_ ## name { \
        static inline std::vector<::multimethods::detail::i_method<type>*> funcs_; \
    }; \
    template<class... Args> inline \
    type name(Args&&... args) { \
        for( auto m : _mm_f_ ## name ::funcs_ ) \
            try { \
                return m->call(args...); \
            } catch(::multimethods::not_match&) { \
            } \
        throw std::logic_error( #name ": not implemented."); \
    } \

/**********************************************************************************************/
#define define_method(name, ...) \
   static _mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)(__VA_ARGS__); \
   static bool MM_JOIN(_mm_init_, __LINE__) = []{ _mm_f_ ## name ::funcs_.push_back(::multimethods::detail::make_method<_mm_r_ ## name>(MM_JOIN(_mm_impl_, __LINE__))); return true; }(); \
   static _mm_r_ ## name MM_JOIN(_mm_impl_, __LINE__)(__VA_ARGS__)

#define skip_method throw ::multimethods::not_match();

/**********************************************************************************************/
#define MM_JOIN(x, y) MM_JOIN_AGAIN(x, y)
#define MM_JOIN_AGAIN(x, y) x ## y


/**********************************************************************************************/
namespace multimethods {

/**********************************************************************************************/
struct not_match : std::exception {
    virtual const char* what() const noexcept { return "multimethods::not_match"; }
};

/**********************************************************************************************/
struct unknown {
    virtual ~unknown() {}
};


/**********************************************************************************************/
namespace detail {

/**********************************************************************************************/
class arg {
    unknown* base_;
    bool const_;
    void* p_;
    std::type_index type_;

    public:
    template<class T, class = typename std::enable_if<std::is_polymorphic<typename std::decay<T>::type>::value>::type>
    arg(T& v)
    : base_(dynamic_cast<unknown*>(&v))
    , const_(std::is_const<T>::value)
    , p_(const_cast<void*>(reinterpret_cast<const void*>(&v)))
    , type_(typeid(v)) {
    }

    template<class T, class = typename std::enable_if<!std::is_polymorphic<typename std::decay<T>::type>::value>::type, class U = T>
    arg(T& v)
    : base_(nullptr)
    , const_(std::is_const<T>::value)
    , p_(const_cast<void*>(reinterpret_cast<const void*>(&v)))
    , type_(typeid(v)) {
    }

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
template<class T>
struct i_method {
    virtual ~i_method() {}
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
template<class T, class F>
struct method_0 : i_method<T> {
    F f_;
    method_0(F f) : f_(f) {}

    T call() { return f_(); }
};

/**********************************************************************************************/
template<class T, class F>
struct method_1 : i_method<T> {
    F f_;
    method_1(F f) : f_(f) {}

    T call(arg p) {
        return f_(p.cast<typename function_traits<F>::arg1_type>());
    }
};

/**********************************************************************************************/
template<class T, class F>
struct method_2 : i_method<T> {
    F f_;
    method_2(F f) : f_(f) {}

    T call(arg p1, arg p2) {
        return f_(p1.cast<typename function_traits<F>::arg1_type>(),
                  p2.cast<typename function_traits<F>::arg2_type>());
    }
};

/**********************************************************************************************/
template<class T, class F>
struct method_3 : i_method<T> {
    F f_;
    method_3(F f) : f_(f) {}

    T call(arg p1, arg p2, arg p3) {
        return f_(p1.cast<typename function_traits<F>::arg1_type>(),
                  p2.cast<typename function_traits<F>::arg2_type>(),
                  p3.cast<typename function_traits<F>::arg3_type>());
    }
};

/**********************************************************************************************/
template<class T, class F>
struct method_4 : i_method<T> {
    F f_;
    method_4(F f) : f_(f) {}

    T call(arg p1, arg p2, arg p3, arg p4) {
        return f_(p1.cast<typename function_traits<F>::arg1_type>(),
                  p2.cast<typename function_traits<F>::arg2_type>(),
                  p3.cast<typename function_traits<F>::arg3_type>(),
                  p4.cast<typename function_traits<F>::arg4_type>());
    }
};


/**********************************************************************************************/
template<class T, class F> inline
auto make_method(F&& f) -> typename std::enable_if<function_traits<F>::arity == 0, i_method<T>*>::type {
    return new method_0<T, F>(f);
}

/**********************************************************************************************/
template<class T, class F> inline
auto make_method(F&& f) -> typename std::enable_if<function_traits<F>::arity == 1, i_method<T>*>::type {
    return new method_1<T, F>(f);
}

/**********************************************************************************************/
template<class T, class F> inline
auto make_method(F&& f) -> typename std::enable_if<function_traits<F>::arity == 2, i_method<T>*>::type {
    return new method_2<T, F>(f);
}

/**********************************************************************************************/
template<class T, class F> inline
auto make_method(F&& f) -> typename std::enable_if<function_traits<F>::arity == 3, i_method<T>*>::type {
    return new method_3<T, F>(f);
}

/**********************************************************************************************/
template<class T, class F> inline
auto make_method(F&& f) -> typename std::enable_if<function_traits<F>::arity == 4, i_method<T>*>::type {
    return new method_4<T, F>(f);
}

/**********************************************************************************************/
} }

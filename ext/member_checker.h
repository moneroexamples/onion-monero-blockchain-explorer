#ifndef MEMBER_CHECKER_H
#define MEMBER_CHECKER_H


#define DEFINE_MEMBER_CHECKER(member) \
    template<typename T, typename V = bool> \
    struct has_ ## member : false_type { }; \
    \
    template<typename T> \
    struct has_ ## member<T, \
        typename enable_if< \
            !is_same<decltype(declval<T>().member), void>::value, \
            bool \
            >::type \
        > : true_type { };

#define HAS_MEMBER(C, member) \
    has_ ## member<C>::value


// first getter if the member veriable is present, so we return its value
// second getter, when the member is not present, so we return empty value, e.g., empty string
#define DEFINE_MEMBER_GETTER(member, ret_value) \
    template<typename T> \
    typename enable_if<HAS_MEMBER(T, member), ret_value>::type \
    get_ ## member (T t){ \
        return t.member; \
    } \
    \
    template<typename T> \
    typename enable_if<!HAS_MEMBER(T, member), ret_value>::type \
    get_ ## member (T t){ \
        return ret_value(); \
    }

#endif // MEMBER_CHECKER_H

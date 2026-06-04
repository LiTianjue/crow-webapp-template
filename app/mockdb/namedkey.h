#ifndef NAMEDKEY_H
#define NAMEDKEY_H
#include <string>

template <typename T>
struct NamedKey {
    static_assert (sizeof (T) == 0,"this Type is not define for DB NamedKey");
};

#define NAMEDKEY(Na) \
    template<> \
    struct NamedKey<Na> \
    {std::string operator()(){return std::string(#Na);}};

#endif // NAMEDKEY_H

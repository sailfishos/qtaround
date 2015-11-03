#ifndef _CUTES_ERROR_HPP_
#define _CUTES_ERROR_HPP_
/**
 * @file error.hpp
 * @brief Unified exceptions
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <exception>
#include <QVariantMap>
#include <QDebug>

namespace qtaround { namespace error {

template <typename T>
QString dump(T && t)
{
    QString s;
    QDebug d(&s);
    d << t;
    return s;
}

class Error : public std::exception
{
public:
    Error(QVariantMap const &from) : m(from), cstr(nullptr) {}
    virtual ~Error() noexcept(true) { if (cstr) free(cstr); }
    virtual const char* what() const noexcept(true);

    QVariantMap m;
    mutable QString s;
    mutable char *cstr;
};

QDebug operator << (QDebug dst, error::Error const &src);

static inline void raise(QVariantMap const &m)
{
    throw Error(m);
}

static inline void raise(std::initializer_list<std::pair<QString, QVariant> > src)
{
    raise(QVariantMap(src));
}

template <typename T, typename T2, typename ... A>
void raise(T const &m1, T2 const &m2, A && ...args)
{
    QVariantMap x = m1;
    QVariantMap y = m2;
    x.unite(y);
    raise(x, std::forward<A>(args)...);
}

}}

#ifdef QTAROUND_NO_NS
namespace error = qtaround::error;
#endif

#endif // _CUTES_ERROR_HPP_

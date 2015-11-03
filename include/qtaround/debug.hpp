#ifndef _CUTES_DEBUG_HPP_
#define _CUTES_DEBUG_HPP_
/**
 * @file debug.hpp
 * @brief Debug tracing support
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014-2015 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/util.hpp>

#include <QVariant>
#include <QDebug>
#include <QTextStream>

#include <memory>

template <typename T, typename ... Args>
QDebug & operator << (QDebug &d, std::function<T(Args...)> const &)
{
    d << "std::function<...>";
    return d;
}

namespace qtaround { namespace debug {

enum class Level { First_ = 1, Debug = First_, Info, Warning, Error
        , Critical, Last_ = Critical };
/// compatible with syslog priorities
enum class Priority { First_ = 0, Emerg = First_, Alert, Crit
        , Err, Warning, Notice, Info
        , Debug, Last_ = Debug };

void set_max_priority(Priority);
Priority get_priority(Level);
void init();

template <Priority P>
struct Traits {
    Traits() { }
    QDebug stream();
    QMessageLogger logger;
};

extern template struct Traits<Priority::Debug>;
extern template struct Traits<Priority::Info>;
extern template struct Traits<Priority::Notice>;
extern template struct Traits<Priority::Warning>;
extern template struct Traits<Priority::Err>;
extern template struct Traits<Priority::Crit>;
extern template struct Traits<Priority::Alert>;
extern template struct Traits<Priority::Emerg>;

static inline void print(QDebug &&d)
{
    QDebug dd(std::move(d));
}

template <typename T, typename ... A>
void print(QDebug &&d, T &&v1, A&& ...args)
{
    d << v1;
    return print(std::move(d), std::forward<A>(args)...);
}

// template <Level L, typename ... A>
// void print_for(A&& ...args)
// {
//     Traits<L> ctx;
//     return print(ctx.stream(), std::forward<A>(args)...);
// }

template <typename ... A>
void print(A&& ...args)
{
    Traits<Priority::Emerg> ctx;
    return print(ctx.stream(), std::forward<A>(args)...);
}

void level(Level);
bool is_tracing_level(Level);
bool is_traceable(Priority);

template <typename ... A>
void print_ge(Level print_level, A&& ...args)
{
    if (is_tracing_level(print_level))
        print(std::forward<A>(args)...);
}

template <typename ... A>
void print_ge(Priority prio, A&& ...args)
{
    if (is_traceable(prio))
        print(std::forward<A>(args)...);
}

template <Priority P, typename ... A>
void print_ge_for(A&& ...args)
{
    if (is_traceable(P))
        print(std::forward<A>(args)...);
}

template <typename ... A>
void debug(A&& ...args)
{
    print_ge_for<Priority::Debug, A...>(std::forward<A>(args)...);
}

template <typename ... A>
void info(A&& ...args)
{
    print_ge(Priority::Info, std::forward<A>(args)...);
}

template <typename ... A>
void warning(A&& ...args)
{
    print_ge(Priority::Warning, std::forward<A>(args)...);
}

template <typename ... A>
void error(A&& ...args)
{
    print_ge(Priority::Err, std::forward<A>(args)...);
}

template <typename ... A>
void critical(A&& ...args)
{
    print_ge(Priority::Crit, std::forward<A>(args)...);
}

}}

#ifdef QTAROUND_NO_NS
namespace debug = qtaround::debug;
#endif

#endif // _CUTES_DEBUG_HPP_

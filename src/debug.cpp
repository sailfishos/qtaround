/**
 * @file debug.cpp
 * @brief Debug tracing support
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/debug.hpp>
#include <mutex>
#include <string>
#include <iostream>

namespace qtaround { namespace debug {


Priority get_priority(Level l)
{
    static Priority l2p[] = { Priority::Debug, Priority::Info
                              , Priority::Warning, Priority::Err
                              , Priority::Crit };
    int i = static_cast<int>(l) - static_cast<int>(Level::First_);
    return l2p[i];
}

namespace {

int priority_from_env();

int current_priority = static_cast<int>(priority_from_env());

void set_priority(Priority p)
{
    current_priority = static_cast<int>(p);
}

int get_priority(char const *name)
{
    static std::map<std::string, Priority> data = {
        {"Debug", Priority::Debug},
        {"Info", Priority::Info},
        {"Notice", Priority::Notice},
        {"Warning", Priority::Warning},
        {"Err", Priority::Err},
        {"Crit", Priority::Crit},
        {"Alert", Priority::Alert},
        {"Emerg", Priority::Emerg},
    };
    auto res = -1;
    if (name) {
        auto it = data.find(name);
        if (it != data.cend())
            res = static_cast<int>(it->second);
    }
    return res;
}


void set_level(Level l)
{
    set_priority(get_priority(l));
}

int env_get(char const *name, int def_val)
{
    int res = def_val;
    if (name) {
        auto c = ::getenv(name);
        if (c) {
            char *end = nullptr;
            auto conv = strtol(c, &end, 10);
            if (c + strlen(c) == end)
                res = conv;
        }
    }
    return res;
}

char const* env_get(char const *name, char const* def_val)
{
    auto res = def_val;
    if (name) {
        auto c = ::getenv(name);
        if (c)
            res = c;
    }
    return res;
}

int priority_from_env()
{
    auto prio = env_get("QTAROUND_DEBUG", -1);
    // allow out of range values
    if (prio == -1) {
        prio = get_priority(env_get("QTAROUND_DEBUG", nullptr));
        if (prio == -1) {
            auto level = env_get("CUTES_DEBUG", -1);
            prio = static_cast<int>
                ((can_convert<Level>(level)
                  ? get_priority(static_cast<Level>(level))
                  : Priority::Warning));
        }
    }
    return prio;
}

} // anon ns

// stub for backward compatibility
void init() { }

template struct Traits<Priority::Debug>;
template struct Traits<Priority::Info>;
template struct Traits<Priority::Notice>;
template struct Traits<Priority::Warning>;
template struct Traits<Priority::Err>;
template struct Traits<Priority::Crit>;
template struct Traits<Priority::Alert>;
template struct Traits<Priority::Emerg>;

template <>
QDebug Traits<Priority::Debug>::stream()
{
    return logger.debug();
}

template <>
QDebug Traits<Priority::Info>::stream()
{
    return logger.debug();
}

template <>
QDebug Traits<Priority::Notice>::stream()
{
    return logger.debug();
}

template <>
QDebug Traits<Priority::Warning>::stream()
{
    return logger.warning();
}

template <>
QDebug Traits<Priority::Err>::stream()
{
    return logger.critical();
}

template <>
QDebug Traits<Priority::Crit>::stream()
{
    return logger.critical();
}

template <>
QDebug Traits<Priority::Alert>::stream()
{
    return logger.critical();
}

template <>
QDebug Traits<Priority::Emerg>::stream()
{
    return logger.critical();
}

void level(Level level)
{
    set_level(level);
}

bool is_traceable(Priority prio)
{
    auto p = static_cast<int>(prio);
    return (current_priority >= p);
}

bool is_tracing_level(Level level)
{
    return is_traceable(get_priority(level));
}

}}

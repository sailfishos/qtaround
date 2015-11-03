#ifndef _CUTES_UTIL_HPP_
#define _CUTES_UTIL_HPP_
/**
 * @file util.hpp
 * @brief Misc. helpful utilities
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <cor/util.hpp>
#include "error.hpp"

#include <QVariant>
#include <QDebug>
#include <QTextStream>
#include <QMap>
#include <QStringList>

#include <tuple>
#include <memory>
#include <array>

namespace {

inline QString str(QVariant const &v)
{
    return v.toString();
}

inline QString str(QByteArray const &v)
{
    return QString::fromUtf8(v);
}

inline QString str(int v)
{
    return QString::number(v);
}

inline QString str(char const *v)
{
    return QString(v);
}

template <typename T, typename T2, typename ... A>
QString str(T && v, T2 && v2, A &&...args)
{
    return str(std::forward<T>(v)) + str(std::forward<T2>(v2), std::forward<A>(args)...);
}

inline bool is(QVariant const &v)
{
    return v.toBool();
}

inline bool is(QString const &v)
{
    return v.isEmpty() ? false : v.toUInt();
}

inline bool hasType(QVariant const &v, QMetaType::Type t)
{
    return static_cast<QMetaType::Type>(v.type()) == t;
}

template <typename X, typename Y> X get(Y const &);

template <>
double get<double>(QVariant const &v)
{
    return v.toDouble();
}

typedef QVariantMap map_type;
typedef QMap<QString, QString> string_map_type;
typedef QList<QVariant> list_type;
typedef std::tuple<QString, QVariant> map_tuple_type;

inline QVariantMap map(std::initializer_list<std::pair<QString, QVariant> > data)
{
    return QVariantMap(data);
}

inline QVariantList list(std::initializer_list<QVariant> data)
{
    return QVariantList(data);
}

inline QVariantMap map(QVariant const &v, bool check_type = true)
{
    if (check_type && !hasType(v, QMetaType::QVariantMap))
        qtaround::error::raise({{"msg", "Need QVariantMap"}, {"value", v}});
    return v.toMap();
}

inline QVariantMap const & map(QVariantMap const &data)
{
    return data;
}

template <typename X, typename Y>
inline QMap<X, Y> map(QList<std::tuple<X, Y> > const &src)
{
    QMap<X, Y> res;
    for (auto it = src.begin(); it != src.end(); ++it) {
        res.insert(std::get<0>(*it), std::get<1>(*it));
    }
    return res;
}

QVariant get(QVariantMap const &m, QString const &k1)
{
    return m[k1];
}

inline QStringList filterEmpty(QStringList const &src)
{
    return src.filter(QRegExp("^.+$"));
}

template<>
QString get(QVariant const &from)
{
    return from.toString();
}

template <typename T, typename I>
static bool can_convert
(I v, typename std::enable_if<std::is_enum<T>::value>::type* = 0)
{
    return (v >= static_cast<I>(T::First_)
            && v <= static_cast<I>(T::Last_));
}

template <typename T, typename FnT, typename I>
static bool call_if_convertible
(I v, FnT fn, typename std::enable_if<std::is_enum<T>::value>::type* = 0)
{
    bool res = can_convert<T>(v);
    if (res)
        fn(static_cast<T>(v));
    return res;
}

}

template <typename ... A>
QVariant get(QVariantMap const &m, QString const &k1, QString const &k2, A&& ...args)
{
    auto const &v = m[k1];
    return get(v.toMap(), k2, std::forward<A>(args)...);
}

template <typename T>
std::unique_ptr<T> box(T &&v)
{
    std::unique_ptr<T> p(new T(std::move(v)));
    return p;
}

template <typename T>
T unbox(std::unique_ptr<T> p)
{
    return std::move(*p);
}

/// deprecated
template <typename T> struct StructTraits;

/// deprecated
template <typename FieldsT>
struct Struct
{
    typedef FieldsT id_type;
    typedef typename StructTraits<FieldsT>::type data_type;
    static_assert(static_cast<size_t>(FieldsT::EOE) == std::tuple_size<data_type>::value
                  , "Enum should end with EOE == tuple size");

    Struct(data_type const &src) : data(src) {}
    Struct(data_type &&src) : data(std::move(src)) {}

    template <typename ... Args>
    Struct(Args &&...args) : data(std::forward<Args>(args)...) {}

    template <FieldsT Id>
    typename std::tuple_element<static_cast<size_t>(Id), data_type>::type &get()
    {
        return std::get<Index<Id>::value>(data);
    }

    template <FieldsT Id>
    typename std::tuple_element<static_cast<size_t>(Id), data_type>::type const &
        get() const
    {
        return std::get<Index<Id>::value>(data);
    }

    template <FieldsT Id>
    struct Index {
        static_assert(Id != FieldsT::EOE, "Should not be EOE");
        static const size_t value = static_cast<size_t>(Id);
    };

    template <size_t Index>
    struct Enum {
        static const FieldsT value = static_cast<FieldsT>(Index);
        static_assert(value < FieldsT::EOE, "Should be < EOE");
    };

private:
    data_type data;
};

template <typename ...Args>
constexpr size_t count(Args &&...)
{
    return sizeof...(Args);
}

/// deprecated
#define STRUCT_NAMES(Id, id_names...)           \
    template <Id i>                             \
    static char const * name()                                    \
    {                                                             \
        auto const end = static_cast<size_t>(Id::EOE);                  \
        static const std::array<char const *, end> names{{id_names}};   \
        static_assert(count(id_names) == end, "Check names count");     \
        return names[Struct<Id>::Index<i>::value];                      \
    }

namespace qtaround { namespace debug {

/// deprecated
template <size_t N>
struct StructDump
{
    template <typename FieldsT>
    static void out(QDebug &d, Struct<FieldsT> const &v)
    {
        static auto const end = (size_t)FieldsT::EOE;
        static auto const id = Struct<FieldsT>::template Enum<end - N>::value;
        static auto const *name(StructTraits<FieldsT>::template name<id>());
        auto const &r = v.template get<id>();
        d << name << "=" << r << ", ";
        StructDump<N - 1>::out(d, v);
    }
};

/// deprecated
template <>
struct StructDump<1>
{
    template <typename FieldsT>
    static void out(QDebug &d, Struct<FieldsT> const &v)
    {
        static auto const end = (size_t)FieldsT::EOE;
        static auto const id = Struct<FieldsT>::template Enum<end - 1>::value;
        static auto const *name(StructTraits<FieldsT>::template name<id>());
        auto const &r = v.template get<id>();
        d << name << "=" << r;
    }
};

/// deprecated
template <typename FieldsT>
QDebug & operator <<(QDebug &d, Struct<FieldsT> const &v)
{
    static const auto index = (size_t)FieldsT::EOE;
    static_assert(index != 0, "Struct should");
    d << "("; StructDump<index>::out(d, v); d << ")";
    return d;
}

}}

template <typename FieldsT, typename... ElementsT>
QDebug & operator <<(QDebug &dst, Record<FieldsT, ElementsT...> const &v)
{
    static constexpr auto index = static_cast<size_t>(FieldsT::Last_);
    dst << "("; RecordDump<index>::out(dst, v); dst << ")";
    return dst;
}

template <typename FieldsT, typename... ElementsT>
QString loggable(Record<FieldsT, ElementsT...> const &v)
{
    QString res;
    QDebug dst(res);
    dst << v;
    return res;
}

namespace qtaround { namespace util {

template <typename ResT, typename T, typename FnT>
QList<ResT> map(FnT fn, QList<T> const &src)
{
    QList<ResT> res;
    for (auto it = src.begin(); it != src.end(); ++it) {
        res.push_back(fn(*it));
    }
    return res;
}

template <typename ResT, typename FnT, typename K, typename V>
QList<ResT> map(FnT fn, QMap<K, V> const &src)
{
    QList<ResT> res;
    for (auto it = src.begin(); it != src.end(); ++it) {
        res.push_back(fn(it.key(), it.value()));
    }
    return res;
}

template <typename ResT, typename FnT>
QList<ResT> map(FnT fn, QString const &src)
{
    QList<ResT> res;
    for (auto it = src.begin(); it != src.end(); ++it) {
        res.push_back(fn(*it));
    }
    return res;
}

template <typename X, typename Y>
QList<std::tuple<X, Y> > zip(QList<X> const &x, QList<Y> const &y)
{
    QList<std::tuple<X, Y> > res;
    auto yit = y.begin();
    auto xit = x.begin();
    for(;xit != x.end(); ++xit, ++yit) {
        if (yit != y.end())
            res.push_back(std::make_tuple(*xit, *yit));
        else
            break;
    }
    for(;xit != x.end(); ++xit)
        res.push_back(std::make_tuple(*xit, Y()));

    return res;
}

template <typename ResKeyT, typename T, typename K>
QMap<ResKeyT, T> mapByField(QList<T> const &src, K const &key)
{
    auto fn = [&key](T const &v) { return std::make_tuple(get<ResKeyT>(v[key]), v); };
    auto pairs = map<std::tuple<ResKeyT, T> >(fn, src);
    return ::map(pairs);
}

double parseBytes(QString const &s, QString const &unit = "b"
                  , long multiplier = 1024);

typedef std::function<QVariant (QVariant const &
                                , QVariant const &
                                , QVariant const &)> visitor_type;
QVariant visit(visitor_type visitor, QVariant const &src, QVariant const &ctx);

}}

#define UNIQUE_PTR(T) std::unique_ptr<T, void(*)(T*)>

template <typename T>
typename UNIQUE_PTR(T)::deleter_type get_qobject_deleter()
{
    return [](T *p) { p->deleteLater(); };
}

template <typename T, typename ... Args>
UNIQUE_PTR(T) make_qobject_unique(Args &&...args)
{
    return UNIQUE_PTR(T)
        (new T(std::forward<Args>(args)...)
         , get_qobject_deleter<T>());
}

template <typename T>
UNIQUE_PTR(T) acquire_qobject_unique(T *p)
{
    return UNIQUE_PTR(T)(p, get_qobject_deleter<T>());
}

template <typename T>
UNIQUE_PTR(T) null_qobject_unique()
{
    return acquire_qobject_unique<T>(nullptr);
}

template <typename To, typename From>
UNIQUE_PTR(To) static_cast_qobject_unique(UNIQUE_PTR(From) from)
{
    return acquire_qobject_unique<To>(static_cast<To*>(from.release()));
}

template <typename T, typename ... Args>
std::shared_ptr<T> make_qobject_shared(Args &&...args)
{
    return std::shared_ptr<T>
        (new T(std::forward<Args>(args)...)
         , get_qobject_deleter<T>());
}

template <typename T>
std::shared_ptr<T> qobject_shared(T *p)
{
    return std::shared_ptr<T>(p, get_qobject_deleter<T>());
}

template <typename T>
std::shared_ptr<T> qobject_shared(UNIQUE_PTR(T) p)
{
    return std::shared_ptr<T>(p.release(), get_qobject_deleter<T>());
}

template <typename T>
std::shared_ptr<QObject> qobject_shared_cast
(UNIQUE_PTR(T) p, typename std::enable_if
 <std::is_convertible<T*, QObject*>::value>::type* = 0)
{
    return qobject_shared(static_cast<QObject*>(p.release()));
}

template <typename To, typename From>
std::shared_ptr<To> static_cast_qobject_shared(UNIQUE_PTR(From) from)
{
    auto p = static_cast_qobject_unique<To>(from);
    return qobject_shared(std::move(p));
}

template <typename T>
UNIQUE_PTR(T) qobject_box(T &&v)
{
    std::unique_ptr<T> p(new T(std::move(v)));
    return p;
}

#ifdef QTAROUND_NO_NS
namespace util = qtaround::util;
#endif

#endif // _CUTES_UTIL_HPP_

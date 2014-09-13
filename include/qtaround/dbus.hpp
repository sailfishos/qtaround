#ifndef _QTAROUND_DBUS_HPP_
#define _QTAROUND_DBUS_HPP_
/**
 * @file dbus.hpp
 * @brief D-Bus wrappers
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */


#include <QDBusPendingReply>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QDBusInterface>
#include <QDebug>

#include <stdexcept>
#include <functional>
#include <memory>

#include <qtaround/util.hpp>

namespace qtaround {
namespace dbus {

// copied from statefs-providers common headers, it is better to move
// this functionality to the qtaround library and use it
// directly. Most of d-bus functions return one argument, so to avoid
// complex constructions those wrappers support only the case with
// single return value

template <typename T>
QDBusPendingReply<T> sync(QDBusPendingReply<T> &&reply)
{
    QDBusPendingCallWatcher watcher(reply);
    watcher.waitForFinished();
    if (!watcher.isFinished())
        throw std::logic_error("D-Bus request is not executed");
    return reply;
}

void logDBusErr(QDBusError const &);

template <typename T, typename OnValue, typename OnError>
bool callbackOrError(QDBusPendingReply<T> const &reply
                     , OnValue fn, OnError err_fn)
{
    if (reply.isError()) {
        err_fn(reply.error());
        return false;
    }

    fn(reply.value());
    return true;
}

template <typename T, typename OnValue>
bool callbackOrError(QDBusPendingReply<T> const &reply, OnValue &&fn)
{
    return callbackOrError(reply, std::forward<OnValue>(fn), &logDBusErr);
}

template <typename T, typename OnValue, typename OnError>
void async(QObject *parent, QDBusPendingCallWatcher *watcher
           , OnValue fn, OnError err_fn)
{
    auto slot = [fn, err_fn](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<T> reply = *w;
        callbackOrError(reply, fn, err_fn);
        w->deleteLater();
    };
    parent->connect(watcher, &QDBusPendingCallWatcher::finished, slot);
}

template <typename T, typename OnValue>
void async(QObject *parent, QDBusPendingCallWatcher *watcher
           , OnValue &&fn)
{
    async<T>(parent, watcher, std::forward<OnValue>(fn), &logDBusErr);
}

template <typename T, typename OnValue, typename ...A>
void async(QObject *parent, QDBusPendingCall const &call
           , OnValue &&on_value, A &&...args)
{
    auto watcher = new QDBusPendingCallWatcher(call, parent);
    async<T>(parent, watcher, std::forward<OnValue>(on_value)
             , std::forward<A>(args)...);
}

template <typename T, typename OnValue, typename ...A>
void async(QObject *parent, QDBusPendingReply<T> const &reply
           , OnValue on_value, A &&...args)
{
    auto watcher = new QDBusPendingCallWatcher(reply, parent);
    async<T>(parent, watcher, std::forward<OnValue>(on_value)
             , std::forward<A>(args)...);
}

template <typename T, typename OnValue, typename ...A>
void async(QDBusInterface *interface, QString const &method
           , OnValue on_value, A &&...args)
{
    QDBusPendingReply<T> reply = interface->asyncCall(method);
    async<T>(interface, std::move(reply)
             , std::forward<OnValue>(on_value)
             , std::forward<A>(args)...);
}

template <typename OnValue, typename T>
bool sync(QDBusPendingReply<T> reply, OnValue on_value)
{
    QDBusPendingCallWatcher watcher(reply);
    watcher.waitForFinished();
    if (!watcher.isFinished()) {
        return false;
    }
    return callbackOrError(reply, on_value);
}

// tuple wrapper used for de/serialization
template <size_t Pos, typename T>
struct Tuple
{
    static void output(QDBusArgument &argument, T const &src)
    {
        argument << std::get<std::tuple_size<T>::value - Pos>(src);
        Tuple<Pos - 1, T>::output(argument, src);
    }

    static void input(QDBusArgument const &argument, T &dst)
    {
        argument >> std::get<std::tuple_size<T>::value - Pos>(dst);
        Tuple<Pos - 1, T>::input(argument, dst);
    }
};

template <typename T>
struct Tuple<0, T>
{
    static void output(QDBusArgument &, T const &)
    {
        // do nothing, after the last element
    }

    static void input(QDBusArgument const &, T &)
    {
        // do nothing, after the last element
    }
};

class ServiceWatch : public QObject
{
    Q_OBJECT;
public:
    ServiceWatch(QDBusConnection &bus, QString const &service
                 , QObject *parent = nullptr)
        : QObject(parent), bus_(bus), service_(service)
    {}

    virtual ~ServiceWatch() {}

    template <typename RegT, typename UnregT, typename OnExceptionT>
    void init(RegT onRegister, UnregT onUnregister, OnExceptionT onException)
    {
        if (watcher_)
            return;

        watcher_.reset(new QDBusServiceWatcher(service_, bus_));

        auto cb = [onRegister, onUnregister, onException]
            (const QString & serviceName, const QString &
             , const QString & newOwner) {
            try {
                if (newOwner == "")
                    onUnregister();
                else
                    onRegister();
            } catch(std::exception const &e) {
                onException(serviceName, e);
            }
        };
        connect(watcher_.get(), &QDBusServiceWatcher::serviceOwnerChanged, cb);
    }

    template <typename RegT, typename UnregT>
    void init(RegT onRegister, UnregT onUnregister)
    {
        init<RegT, UnregT>(onRegister, onUnregister
                           , &ServiceWatch::logException);
    }

    void init(std::function<void()>, std::function<void()>);
    void init(void (*)(), void (*)());

private:
    static void logException(const QString &, std::exception const &);

    QDBusConnection &bus_;
    QString service_;
    std::unique_ptr<QDBusServiceWatcher> watcher_;
};

QDBusPendingReply<QVariantMap> getAll(QDBusAbstractInterface *, const QString &);

static inline UNIQUE_PTR(QDBusInterface) interfaceCast
(QDBusInterface *i, QString const &to)
{
    return make_qobject_unique<QDBusInterface>
        (i->service(), i->path(), to, i->connection(), i);
}

template <typename OnProps, typename ...A>
void forAllProperties(QDBusInterface *iface, OnProps &&fn, A &&...args)
{
    auto properties = qobject_shared
        (interfaceCast(iface, "org.freedesktop.DBus.Properties"));
    auto wrapper = [properties, fn](QVariantMap const &p) { fn(p); }; 
    auto reply = getAll(properties.get(), iface->interface());
    async<QVariantMap>(iface, reply, wrapper, std::forward<A>(args)...);
}

}}

template <typename ... Args>
QDBusArgument & operator << (QDBusArgument &argument
                             , std::tuple<Args...> const &src)
{
    using qtaround::dbus::Tuple;
    typedef std::tuple<Args...> tuple_type;
    argument.beginStructure();
    Tuple<std::tuple_size<tuple_type>::value
              , tuple_type>::output(argument, src);
    argument.endStructure();
    return argument;
}

template <typename ... Args>
QDBusArgument const& operator >> (QDBusArgument const &argument
                                  , std::tuple<Args...> &dst)
{
    using qtaround::dbus::Tuple;
    typedef std::tuple<Args...> tuple_type;
    argument.beginStructure();
    Tuple<std::tuple_size<tuple_type>::value
              , tuple_type>::input(argument, dst);
    argument.endStructure();
    return argument;
}

#endif // _QTAROUND_DBUS_HPP_

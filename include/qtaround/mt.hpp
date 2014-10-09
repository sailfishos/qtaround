#ifndef _QTAROUND_MT_HPP_
#define _QTAROUND_MT_HPP_
/**
 * @file mt.hpp
 * @brief Concurrency support
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/util.hpp>
#include <QThread>
#include <QCoreApplication>

namespace qtaround { namespace mt {

class AnyActor;
typedef std::shared_ptr<AnyActor> AnyActorHandle;

class ActorContext : public QObject
{
    Q_OBJECT
public:
    typedef std::function<UNIQUE_PTR(QObject) ()> ctor_type;
    typedef std::function<void (AnyActorHandle)>callback_type;
    ActorContext(AnyActorHandle actor, ctor_type ctor, callback_type cb)
        : actor_(actor), ctor_(ctor), notify_(cb)
    {}

    AnyActorHandle actor_;
    ctor_type ctor_;
    callback_type notify_;
};

class AnyActor : public QThread
{
    Q_OBJECT
protected:
    virtual ~AnyActor();

    void run() Q_DECL_OVERRIDE;

public:
    AnyActor(QObject *parent) : QThread(parent) {}

    AnyActor(AnyActor const&) = delete;
    AnyActor& operator = (AnyActor const&) = delete;

    static void create(ActorContext::ctor_type
                       , ActorContext::callback_type
                       , QObject *parent = nullptr);

    static AnyActorHandle createSync
    (ActorContext::ctor_type, QObject *parent = nullptr);

    bool postEvent(QEvent *);
    bool sendEvent(QEvent *);

private:
    std::shared_ptr<QObject> obj_;
};

template <typename T> void startActor
(std::function<UNIQUE_PTR(T) ()> ctor
 , ActorContext::callback_type cb, QObject *parent = nullptr
 , typename std::enable_if<std::is_convertible<T*, QObject*>::value>::type* = 0)
{
    auto qobj_ctor = [ctor]() {
        return static_cast_qobject_unique<QObject>(ctor());
    };
    AnyActor::create(qobj_ctor, cb, parent);
}

template <typename T> AnyActorHandle startActorSync
(std::function<UNIQUE_PTR(T) ()> ctor, QObject *parent = nullptr
 , typename std::enable_if<std::is_convertible<T*, QObject*>::value>::type* = 0)
{
    auto qobj_ctor = [ctor]() {
        return static_cast_qobject_unique<QObject>(ctor());
    };
    return AnyActor::createSync(qobj_ctor, parent);
}

}}

#endif // _QTAROUND_MT_HPP_

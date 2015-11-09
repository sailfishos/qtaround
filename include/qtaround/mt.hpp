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

class Actor;

typedef std::shared_ptr<Actor> ActorHandle;
typedef std::function<UNIQUE_PTR(QObject) ()> qobj_ctor_type;
typedef std::function<void (ActorHandle)> actor_callback_type;

class ActorImpl;

class Actor : public QObject
{
    Q_OBJECT
public:
    Actor(QObject *parent);
    virtual ~Actor();

    Actor(Actor const&) = delete;
    Actor& operator = (Actor const&) = delete;

    static void create(qobj_ctor_type
                       , actor_callback_type
                       , QObject *parent = nullptr);

    static ActorHandle createSync
    (qobj_ctor_type, QObject *parent = nullptr);

    bool postEvent(QEvent *);
    bool sendEvent(QEvent *);
    void quit();
    bool quitSync(unsigned long timeout);
    bool wait(unsigned long timeout);

signals:
    void finished(Actor*);

private:
    friend class ActorImpl;
    ActorImpl *impl_;
};

template <typename T>
void startActor
(std::function<UNIQUE_PTR(T) ()> ctor
 , actor_callback_type cb
 , QObject *parent = nullptr
 , typename std::enable_if<std::is_convertible<T*, QObject*>::value>::type* = 0)
{
    auto qobj_ctor = [ctor]() {
        return static_cast_qobject_unique<QObject>(ctor());
    };
    Actor::create(qobj_ctor, cb, parent);
}

template <typename T>
ActorHandle startActorSync
(std::function<UNIQUE_PTR(T) ()> ctor, QObject *parent = nullptr
 , typename std::enable_if<std::is_convertible<T*, QObject*>::value>::type* = 0)
{
    auto qobj_ctor = [ctor]() {
        return static_cast_qobject_unique<QObject>(ctor());
    };
    return Actor::createSync(qobj_ctor, parent);
}

void deleteOnApplicationExit(ActorHandle);
}}

#endif // _QTAROUND_MT_HPP_

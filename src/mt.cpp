/**
 * @file mt.cpp
 * @brief Concurrency support
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/debug.hpp>
#include <qtaround/mt.hpp>
#include <mutex>
#include <condition_variable>

namespace qtaround { namespace mt {

AnyActor::~AnyActor()
{
    auto app = QCoreApplication::instance();
    if (app) {
        if (isRunning())
            quit();
        if (QThread::currentThread() != this)
            if (!wait(10000))
                debug::warning("Timeout: no quit from thread!");
    }
    if (obj_ && this != QThread::currentThread())
        debug::warning("Managed object is not deleted in a right thread Current:"
                       , QThread::currentThread(), ", Need:", this);
}

void AnyActor::run()
{
    auto ctx = std::static_pointer_cast<ActorContext>(std::move(obj_));
    obj_ = ctx->ctor_();
    ctx->notify_(std::move(ctx->actor_));
    exec();
    obj_.reset();
}

bool AnyActor::postEvent(QEvent *e)
{
    auto obj = obj_;

    if (obj) {
        QCoreApplication::postEvent(obj.get(), e);
        return true;
    } else {
        if (e) delete e;
        return false;
    }
}

bool AnyActor::sendEvent(QEvent *e)
{
    auto obj = obj_;
    if (obj) {
        return QCoreApplication::sendEvent(obj.get(), e);
    } else {
        if (e) delete e;
        return false;
    }
}

void AnyActor::create
(ActorContext::ctor_type ctor, ActorContext::callback_type cb
 , QObject *parent)
{
    auto self = make_qobject_shared<AnyActor>(parent);
    auto ctx = make_qobject_unique<ActorContext>
        (self, std::move(ctor), std::move(cb));
    self->obj_ = qobject_shared_cast(std::move(ctx));
    self->start();
}

AnyActorHandle AnyActor::createSync
(ActorContext::ctor_type ctor, QObject *parent)
{
    std::mutex mutex;
    std::condition_variable cond;
    AnyActorHandle result;
    std::unique_lock<std::mutex> l(mutex);
    create(ctor, [&](mt::AnyActorHandle p) {
            std::unique_lock<std::mutex> l(mutex);
            result = p;
            cond.notify_all();
        }, parent);
    cond.wait(l, [&result]() { return !!result; });
    return result;
}

}}

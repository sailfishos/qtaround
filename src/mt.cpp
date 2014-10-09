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

class Actor;

class ActorContext : public QObject
{
    Q_OBJECT
public:
    ActorContext(ActorHandle actor
                 , qobj_ctor_type ctor
                 , actor_callback_type cb)
        : actor_(actor), ctor_(ctor), notify_(cb)
    {}

    ActorHandle actor_;
    qobj_ctor_type ctor_;
    actor_callback_type notify_;
};

class ActorImpl : public QThread
{
    Q_OBJECT
protected:
    virtual ~ActorImpl();

    void run() Q_DECL_OVERRIDE;

public:
    ActorImpl(QObject *parent) : QThread(parent) {}

    ActorImpl(ActorImpl const&) = delete;
    ActorImpl& operator = (ActorImpl const&) = delete;

    static void create(qobj_ctor_type, actor_callback_type, QObject *);
    static ActorHandle createSync(qobj_ctor_type, QObject *parent);

    bool postEvent(QEvent *);
    bool sendEvent(QEvent *);

private:
    std::shared_ptr<QObject> obj_;
};

Actor::Actor(QObject *parent)
    : QObject(parent), impl_(new ActorImpl(this))
{
    connect(impl_, &QThread::finished
            , this, &Actor::finished
            , Qt::DirectConnection);
}

Actor::~Actor()
{}

void Actor::quit()
{
    impl_->quit();
}

void Actor::create(qobj_ctor_type ctor, actor_callback_type cb, QObject *parent)
{
    ActorImpl::create(ctor, cb, parent);
}

ActorHandle Actor::createSync(qobj_ctor_type ctor, QObject *parent)
{
    return ActorImpl::createSync(ctor, parent);
}

bool Actor::postEvent(QEvent *e)
{
    return impl_->postEvent(e);
}

bool Actor::sendEvent(QEvent *e)
{
    return impl_->sendEvent(e);
}

ActorImpl::~ActorImpl()
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

void ActorImpl::run()
{
    auto ctx = std::static_pointer_cast<ActorContext>(std::move(obj_));
    obj_ = ctx->ctor_();
    ctx->notify_(std::move(ctx->actor_));
    exec();
    obj_.reset();
}

bool ActorImpl::postEvent(QEvent *e)
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

bool ActorImpl::sendEvent(QEvent *e)
{
    auto obj = obj_;
    if (obj) {
        return QCoreApplication::sendEvent(obj.get(), e);
    } else {
        if (e) delete e;
        return false;
    }
}

void ActorImpl::create(qobj_ctor_type ctor, actor_callback_type cb
                       , QObject *parent)
{
    auto wrapper = make_qobject_shared<Actor>(parent);
    auto self = wrapper->impl_;
    auto ctx = make_qobject_unique<ActorContext>
        (wrapper, std::move(ctor), std::move(cb));
    self->obj_ = qobject_shared_cast(std::move(ctx));
    self->start();
}

ActorHandle ActorImpl::createSync(qobj_ctor_type ctor, QObject *parent)
{
    std::mutex mutex;
    std::condition_variable cond;
    ActorHandle result;
    std::unique_lock<std::mutex> l(mutex);
    create(ctor, [&](mt::ActorHandle p) {
            std::unique_lock<std::mutex> l(mutex);
            result = p;
            cond.notify_all();
        }, parent);
    cond.wait(l, [&result]() { return !!result; });
    return result;
}

}}

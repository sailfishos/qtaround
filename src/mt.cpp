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
#include <vector>

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

    bool quitSync(unsigned long timeout);
    bool waitIfOtherThread(unsigned long timeout);

private:
    std::shared_ptr<QObject> obj_;
};

Actor::Actor(QObject *parent)
    : QObject(parent), impl_(new ActorImpl(this))
{
    connect(impl_, &QThread::finished
            , [this]() { this->finished(this); });
}

Actor::~Actor()
{}

void Actor::quit()
{
    if (impl_->isRunning())
        impl_->quit();
}

bool Actor::quitSync(unsigned long timeout)
{
    return impl_->quitSync(timeout);
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

bool Actor::wait(unsigned long timeout)
{
    return impl_->waitIfOtherThread(timeout);
}

ActorImpl::~ActorImpl()
{
    auto app = QCoreApplication::instance();
    if (app) {
        quitSync(10000);
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

bool ActorImpl::quitSync(unsigned long timeout)
{
    if (!isRunning())
        return true;

    quit();
    return waitIfOtherThread(timeout);
}

bool ActorImpl::waitIfOtherThread(unsigned long timeout)
{
    if (!isRunning())
        return true;

    if (this != QThread::currentThread())
        if (!wait(timeout)) {
            return false;
        }
    return true;
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

class AppExitMonitor : public QObject
{
    Q_OBJECT
public:
    AppExitMonitor(QCoreApplication *);
    void insert(ActorHandle);
private slots:
    void beforeAppQuit();
    void actorFinished(Actor*);
private:
    std::map<intptr_t, std::weak_ptr<Actor> > actors_;
};

static AppExitMonitor *monitor_;
static std::once_flag monitor_once_;

void deleteOnApplicationExit(ActorHandle h)
{
    std::call_once(monitor_once_, []() {
            auto app = QCoreApplication::instance();
            if (app)
                monitor_ = new AppExitMonitor(app);
            else
                debug::warning("No QCoreApplication == no AppExitMonitor");
        });
    if (monitor_)
        monitor_->insert(h);
    else
        debug::warning("No AppExitMonitor, don't track", h.get());
}

AppExitMonitor::AppExitMonitor(QCoreApplication *app)
    : QObject(app)
{
    Q_ASSERT(app);
    connect(app, SIGNAL(aboutToQuit())
            , this, SLOT(beforeAppQuit()));
}

void AppExitMonitor::insert(ActorHandle p)
{
    auto id = reinterpret_cast<intptr_t>(p.get());
    actors_.insert(std::make_pair(id, std::weak_ptr<Actor>(p)));
    connect(p.get(), &Actor::finished
            , this, &AppExitMonitor::actorFinished
            , Qt::QueuedConnection);
}

void AppExitMonitor::actorFinished(Actor *p)
{
    auto id = reinterpret_cast<intptr_t>(p);
    actors_.erase(id);
}

void AppExitMonitor::beforeAppQuit()
{
    typedef std::vector<std::shared_ptr<Actor> > actors_type;
    actors_type left_actors;
    for (auto &kv : actors_) {
        auto actor = kv.second.lock();
        if (actor)
            left_actors.emplace_back(actor);
    }
    for (auto &actor : left_actors)
        actor->quit();

    actors_type delayed_actors;
    for (auto &actor : left_actors)
        if (!actor->wait(1)) // or maybe even 0?
            delayed_actors.emplace_back(std::move(actor));

    left_actors.clear();
    for (auto &actor : delayed_actors)
        if (!actor->wait(10000))
            debug::warning("Timeout quiting from actor", actor.get());
}

}}

#include "mt.moc"

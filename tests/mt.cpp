#include <qtaround/mt.hpp>
#include <tut/tut.hpp>
#include "tests_common.hpp"
#include <mutex>
#include <condition_variable>
#include <QFile>

namespace tut
{

struct mt_test
{
    virtual ~mt_test()
    {
    }
};

typedef test_group<mt_test> tf;
typedef tf::object object;
tf vault_mt_test("mt");

enum test_ids {
    tid_actor =  1
};

class Test;
class Event : public QEvent
{
public:
    enum Type {
        Id1 = QEvent::User
        , ExecFn
    };

    Event() : QEvent(static_cast<QEvent::Type>(Id1)) {}
    Event(std::function<void(Test*)> fn)
        : QEvent(static_cast<QEvent::Type>(ExecFn))
        , fn_(fn)
    {}
    virtual ~Event() {}

    std::function<void(Test*)> fn_;
};

class Test : public QObject
{
    Q_OBJECT
public:

    Test()
        : creation_thread(QThread::currentThread())
    {}

    virtual bool event(QEvent *e)
    {
        if (e->type() < QEvent::User)
            return QObject::event(e);
        auto t = static_cast<Event::Type>(e->type());
        auto msg = static_cast<Event*>(e);
        if (t == Event::ExecFn) {
            msg->fn_(this); 
        }
        return true;
    }
    
    QThread *creation_thread;
};

template<> template<>
void object::test<tid_actor>()
{
    namespace mt = qtaround::mt;
    auto main_thread = QThread::currentThread();
    decltype(main_thread) actor_thread = nullptr;

    mt::AnyActorHandle actor;
    std::mutex mutex;
    std::condition_variable cond;
    auto make_test = []() { return make_qobject_unique<Test>(); };
    do {
        std::unique_lock<std::mutex> l(mutex);
        mt::startActor<Test>(make_test, [&](mt::AnyActorHandle a) {
                actor_thread = QThread::currentThread();
                std::unique_lock<std::mutex> l(mutex);
                actor = a;
                a.reset();
                cond.notify_all();
            });
        cond.wait_for(l, std::chrono::seconds(5), [&actor]() { return !!actor; });
    } while (0);

    ensure("Actor should be here", !!actor);
    ensure_ne("Threads are different", actor_thread, main_thread);

    std::condition_variable exit_cond;
    auto isExited = false;
    QObject::connect(actor.get(), &QThread::finished, [&]() {
            std::unique_lock<std::mutex> l(mutex);
            isExited = true;
            exit_cond.notify_all();
        });
    ensure_eq("Use count is wrong", actor.use_count(), 1);
    ensure("Should post events", actor->postEvent(new Event()));
    actor->quit();
    do {
        std::unique_lock<std::mutex> l(mutex);
        ensure("Not exited?"
               , exit_cond.wait_for(l, std::chrono::seconds(2)
                                    , [&isExited]() { return isExited; }));
    } while (0);
    ensure("Should not post events", !actor->postEvent(new Event()));


    auto actor2 = mt::startActorSync<Test>(make_test);
    ensure("Actor2 should be here", !!actor2);
    QThread *testObjThread = nullptr;
    QThread *eventThread = nullptr;
    auto check_fn = [&](Test *test) {
        std::unique_lock<std::mutex> l(mutex);
        eventThread = QThread::currentThread();
        testObjThread = test->creation_thread;
        cond.notify_all();
    };
    ensure("Should post events", actor2->postEvent(new Event(check_fn)));
    do {
        std::unique_lock<std::mutex> l(mutex);
        cond.wait_for(l, std::chrono::seconds(2)
                      , [&testObjThread]() { return testObjThread; });
    } while (0);
    ensure_ne("Incorrect object thread", main_thread, testObjThread);
    ensure_ne("Incorrect event processing thread", main_thread, eventThread);
    ensure_eq("Creation and processing threads should be the same", testObjThread, eventThread);
}

}

#include "mt.moc"

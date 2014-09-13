#include <qtaround/dbus.hpp>

#include "tests_common.hpp"
#include <tut/tut.hpp>
#include <cor/util.hpp>

#include <QDBusConnection>
#include <QCoreApplication>
#include <QDBusInterface>
#include <QTimer>

namespace tut
{

struct dbus_test
{
    virtual ~dbus_test()
    {
    }
};

typedef test_group<dbus_test> tf;
typedef tf::object object;
tf qtaround_dbus_test("dbus");

enum test_ids {
    tid_interface = 1,
    tid_call,
    tid_method
};

namespace {

class QueuedInvoker : public QObject
{
    Q_OBJECT;
public:
    QueuedInvoker(QObject *parent) : QObject(parent), was_executed_(false)
    {
        QMetaObject::invokeMethod(this, "execute", Qt::QueuedConnection);
    }

    ~QueuedInvoker()
    {
        // exception in dtor - bad but it will be final ;)
        ensure(AT, was_executed_);
    }

    virtual void action() =0;

private slots:
    void execute()
    {
        action();
        was_executed_ = true;
        deleteLater();
    }
private:
    bool was_executed_;
};

class QueueFunction : public QueuedInvoker
{
    Q_OBJECT;
public:
    QueueFunction(QObject *parent, std::function<void()> fn)
        : QueuedInvoker(parent), fn_(fn) {}

    virtual void action() { fn_(); }
private:
    std::function<void()> fn_;
};

class Stage : public QObject
{
    Q_OBJECT;
public:
    Stage(QString const &name)
        : is_ok(false), app(QCoreApplication::instance())
        , stage(name)
    {
        auto guard = std::bind(&Stage::notExecutedGuard, this);
        new QueueFunction(this, guard);
    }

    void execEventLoop(int timeout = 5000)
    {
        notExecutedGuard();
        is_ok = false;
        auto t = failOnTimeout(timeout);
        ensure_eq(stage.toStdString(), app->exec(), 0);
        t->stop();
        t->deleteLater();
        ensure(stage.toStdString(), is_ok);
    };

    void asyncDone(bool ok)
    {
        is_ok = ok;
        app->exit(ok ? 0 : 1);
    }

private:

    QTimer *failOnTimeout(int timeout)
    {
        auto t = new QTimer{app};
        t->setSingleShot(true);
        t->connect(t, &QTimer::timeout, [this, t]() {
                if (!is_ok) app->exit(2);
            });
        t->start(timeout);
        return t;
    };

    void notExecutedGuard()
    {
        ensure(stage.toStdString(), !is_ok);
    };

    bool is_ok;
    QCoreApplication *app;
    QString stage;
};

const QString service = "org.freedesktop.DBus";
const QString path = "/org/freedesktop/DBus";
const QString interface = "org.freedesktop.DBus";
const QString method = "ListNames";

}

template<> template<>
void object::test<tid_interface>()
{
    auto bus = QDBusConnection::sessionBus();
    using qtaround::dbus::async;

    Stage stage("async call for interface");
    auto iface = new QDBusInterface(service, path, interface, bus, &stage);
    QDBusPendingReply<QStringList> res = iface->asyncCall(method);
    async<QStringList>(&stage, res,
                       [&stage](QStringList const &names) {
                           stage.asyncDone(names.size() > 0);
                       });
    stage.execEventLoop();
}

template<> template<>
void object::test<tid_call>()
{
    auto bus = QDBusConnection::sessionBus();
    using qtaround::dbus::async;
    Stage stage("async call");
    auto call = bus.asyncCall(QDBusMessage::createMethodCall
                              (service, path, interface, method));
    async<QStringList>(&stage, call,
                       [&stage](QStringList const &names) {
                           stage.asyncDone(names.size() > 0);
                      });
    stage.execEventLoop();
}

template<> template<>
void object::test<tid_method>()
{
    auto bus = QDBusConnection::sessionBus();
    using qtaround::dbus::async;

    Stage stage("async method call");
    auto iface = new QDBusInterface(service, path, interface, bus, &stage);
    async<QStringList>(iface, method,
                       [&stage](QStringList const &names) {
                           stage.asyncDone(names.size() > 0);
                       });
    stage.execEventLoop();
}

}

#include "dbus.moc"

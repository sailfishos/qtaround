#include <qtaround/dbus.hpp>

namespace qtaround {
namespace dbus {

void ServiceWatch::init(std::function<void()> onRegister
                   , std::function<void()> onUnregister)
{
    init<std::function<void()>, std::function<void()> >(onRegister, onUnregister);
}

void ServiceWatch::init(void (*onRegister)(), void (*onUnregister)())
{
    init<void (*)(), void (*)()>(onRegister, onUnregister);
}

void ServiceWatch::logException(const QString & serviceName, std::exception const &e)
{
    qWarning() << "Uncaught exception " << e.what() << " handling "
               << serviceName << " un/registration";
}

void logDBusErr(QDBusError const &err)
{
    qWarning() << "D-Bus request error " << err.name()
               << ": " << err.message();
}

QDBusPendingReply<QVariantMap> getAll
(QDBusAbstractInterface *interface, const QString &name)
{
    QList<QVariant> args;
    args << QVariant::fromValue(name);
    return interface->asyncCallWithArgumentList(QLatin1String("GetAll"), args);
}


}}

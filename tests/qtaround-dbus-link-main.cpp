#include "test_link_common.hpp"
#include <qtaround/dbus.hpp>

int main()
{
    QDBusConnection c{""};
    qtaround::dbus::ServiceWatch w(c, "");
    utest_dummy();
    return 0;
}

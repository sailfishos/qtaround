Summary: QtAround library
Name: qtaround
Version: 0.0.0
Release: 1
License: LGPL21
Group: Development/Liraries
URL: https://github.com/nemomobile/qtaround
Source0: %{name}-%{version}.tar.bz2
BuildRequires: cmake >= 2.8
BuildRequires: pkgconfig(cor) >= 0.1.14
BuildRequires: pkgconfig(tut) >= 0.0.3
BuildRequires: pkgconfig(Qt5Core) >= 5.2.0
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
QtAround library used to port the-vault to C++. Mostly consists of
thin wrappers around Qt classes and standard Linux utilities.

%package devel
Summary: QtAround library
Group: Development/Libraries
Requires: qtaround = %{version}-%{release}
%description devel
QtAround library used to port the-vault to C++. Mostly consists of
thin wrappers around Qt classes and standard Linux utilities.

%package dbus
Summary: QtAround D-Bus wrappers
Group: Development/Libraries
Requires: qtaround = %{version}-%{release}
BuildRequires: pkgconfig(Qt5DBus) >= 5.2.0
%description dbus
QtAround library: D-Bus wrappers

%package dbus-devel
Summary: QtAround D-Bus development files
Group: Development/Libraries
Requires: qtaround = %{version}-%{release}
Requires: qtaround-dbus = %{version}-%{release}
Requires: qtaround-devel = %{version}-%{release}
%description dbus-devel
%{summary}

%package tests
Summary:    Tests for qtaround
License:    LGPLv2.1
Group:      System Environment/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   %{name}-dbus = %{version}-%{release}
%description tests
%summary

%prep
%setup -q

%build
%cmake -DVERSION=%{version} %{?_with_multiarch:-DENABLE_MULTIARCH=ON}
make %{?jobs:-j%jobs}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=%{buildroot}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_libdir}/libqtaround.so*

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/qtaround.pc
%{_libdir}/pkgconfig/qtaround-1.pc
%{_includedir}/qtaround/debug.hpp
%{_includedir}/qtaround/error.hpp
%{_includedir}/qtaround/json.hpp
%{_includedir}/qtaround/os.hpp
%{_includedir}/qtaround/subprocess.hpp
%{_includedir}/qtaround/sys.hpp
%{_includedir}/qtaround/util.hpp

%files dbus
%defattr(-,root,root,-)
%{_libdir}/libqtaround-dbus.so*

%files dbus-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/qtaround-dbus.pc
%{_includedir}/qtaround/dbus.hpp

%files tests
%defattr(-,root,root,-)
/opt/tests/qtaround/*

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%post dbus -p /sbin/ldconfig
%postun dbus -p /sbin/ldconfig

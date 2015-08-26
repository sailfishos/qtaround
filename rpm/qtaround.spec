%{!?cmake_install: %global cmake_install make install DESTDIR=%{buildroot}}

Summary: QtAround library
Name: qtaround
Version: 0.0.0
Release: 1
License: LGPL21
Group: System/Libraries
URL: https://github.com/nemomobile/qtaround
Source0: %{name}-%{version}.tar.bz2
BuildRequires: cmake >= 2.8
BuildRequires: pkgconfig(cor) >= 0.1.17
BuildRequires: pkgconfig(tut) >= 0.0.3
BuildRequires: pkgconfig(Qt5Core) >= 5.2.0
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
QtAround library used to port the-vault to C++. Mostly consists of
thin wrappers around Qt classes and standard Linux utilities.

%package -n libqtaround2
Group: System/Libraries
Summary: QtAround library
Provides: libqtaround = %{version}-%{release}
Obsoletes: libqtaround < %{version}
%description -n libqtaround2
%summary

%package devel
Summary: QtAround library
Group: Development/Libraries
Requires: libqtaround2 = %{version}
Requires: pkgconfig(cor) >= 0.1.17
%description devel
QtAround library used to port the-vault to C++. Mostly consists of
thin wrappers around Qt classes and standard Linux utilities.

%package dbus
Summary: QtAround D-Bus wrappers
Group: Development/Libraries
Requires: libqtaround2 = %{version}
BuildRequires: pkgconfig(Qt5DBus) >= 5.2.0
%description dbus
QtAround library: D-Bus wrappers

%package dbus-devel
Summary: QtAround D-Bus development files
Group: Development/Libraries
Requires: libqtaround2 = %{version}
Requires: qtaround-dbus = %{version}
Requires: qtaround-devel = %{version}
%description dbus-devel
%{summary}

%package tests
Summary:    Tests for qtaround
License:    LGPLv2.1
Group:      System Environment/Libraries
Requires:   %{name} = %{version}
Requires:   %{name}-dbus = %{version}
%if %{undefined suse_version}
Requires:   btrfs-progs
%else
Requires:   btrfsprogs
%endif
%description tests
%summary

%prep
%setup -q

%build
%cmake -DVERSION=%{version} %{?_with_multiarch:-DENABLE_MULTIARCH=ON}
make %{?jobs:-j%jobs}

%install
rm -rf $RPM_BUILD_ROOT
%cmake_install

%clean
rm -rf $RPM_BUILD_ROOT

%files -n libqtaround2
%defattr(-,root,root,-)
%{_libdir}/libqtaround.so.2
%{_libdir}/libqtaround.so.%{version}

%files devel
%defattr(-,root,root,-)
%{_libdir}/libqtaround.so
%{_libdir}/pkgconfig/qtaround.pc
%{_libdir}/pkgconfig/qtaround-1.pc
%{_includedir}/qtaround/debug.hpp
%{_includedir}/qtaround/error.hpp
%{_includedir}/qtaround/json.hpp
%{_includedir}/qtaround/os.hpp
%{_includedir}/qtaround/subprocess.hpp
%{_includedir}/qtaround/sys.hpp
%{_includedir}/qtaround/util.hpp
%{_includedir}/qtaround/mt.hpp

%files dbus
%defattr(-,root,root,-)
%{_libdir}/libqtaround-dbus.so.0
%{_libdir}/libqtaround-dbus.so.%{version}

%files dbus-devel
%defattr(-,root,root,-)
%{_libdir}/libqtaround-dbus.so
%{_libdir}/pkgconfig/qtaround-dbus.pc
%{_includedir}/qtaround/dbus.hpp

%files tests
%defattr(-,root,root,-)
/opt/tests/qtaround/*

%post -n libqtaround2 -p /sbin/ldconfig
%postun -n libqtaround2 -p /sbin/ldconfig

%post dbus -p /sbin/ldconfig
%postun dbus -p /sbin/ldconfig

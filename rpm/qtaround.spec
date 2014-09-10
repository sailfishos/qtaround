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
%dir %{_includedir}/qtaround
%{_includedir}/qtaround/*.hpp

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

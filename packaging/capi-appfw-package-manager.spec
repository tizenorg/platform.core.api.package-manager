Name:       capi-appfw-package-manager
Summary:    Package Manager API
Version: 0.0.30
Release:    1
Group:      System/API
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	capi-appfw-package-manager.manifest
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(pkgmgr)
BuildRequires:  pkgconfig(pkgmgr-installer)
BuildRequires:  pkgconfig(pkgmgr-info)
BuildRequires:	pkgconfig(vconf)
BuildRequires:	pkgconfig(aul)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(cynara-client)

%description
The Package Manager API provides functions to install, uninstall the package,
and also privides event listening function.

%package devel
Summary:  Package Manager API (Development)
Group:    System/API
Requires: %{name} = %{version}-%{release}

%description devel
The Package Manager API provides functions to install, uninstall the package,
and also privides event listening function. (DEV)


%prep
%setup -q
cp %{SOURCE1001} .


%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
%cmake . -DFULLVER=%{version} -DMAJORVER=${MAJORVER}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest %{name}.manifest
%{_libdir}/libcapi-appfw-package-manager.so.*
%{_bindir}/pkgmgr_tool

%files devel
%manifest %{name}.manifest
%{_includedir}/appfw/package_manager.h
%{_includedir}/appfw/package_info.h
%{_libdir}/libcapi-appfw-package-manager.so
%{_libdir}/pkgconfig/*.pc



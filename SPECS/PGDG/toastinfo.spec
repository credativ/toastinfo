Name:		toastinfo
Version:	1.0
Release:	1%{?dist}
Summary:	A PostgreSQL extension module to view toast value information.

Group:		database
License:	BSD
URL:		https://github.com/df7cb/toastinfo
Source0:	https://github.com/df7cb/toastinfo/archive/v%{version}.tar.gz

BuildRequires:	postgresql%{pgdgversion}-devel
Requires:	postgresql%{pgdgversion}

%description
This PostgreSQL extension exposes the internal storage structure of variable-length datatypes, called varlena.

%prep
%setup -q


%build
PG_CONFIG=%{pginstdir}/bin/pg_config USE_PGXS=1 make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
PG_CONFIG=%{pginstdir}/bin/pg_config USE_PGXS=1 make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%{pginstdir}/lib/toastinfo.so
%{pginstdir}/share/extension/toastinfo--1.sql
%{pginstdir}/share/extension/toastinfo.control


%changelog
* Tue Apr 03 2018 - Bernd Helmle <bernd.helmle@credativ.de> 1.0-1
- Upstream release 1.0

#
# harbour-taskswitcher spec
# (C) piggz 2018-2019
#

Name:       harbour-taskswitcher

%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

Summary:    Keyboard taskswitcher
Version:    0.6
Release:    1
Group:      Qt/Qt
License:    LICENSE
URL:        https://github.com/piggz/harbour-taskswitcher
Source0:    %{name}-%{version}.tar.bz2

BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(sailfishapp) >= 0.0.10
BuildRequires:  pkgconfig(nemonotifications-qt5)
BuildRequires:  pkgconfig(Qt5SystemInfo)
BuildRequires:  mce-headers
BuildRequires:  desktop-file-utils
BuildRequires:  qt5-qttools-linguist

Requires:   sailfishsilica-qt5 >= 0.10.9

%description
%{summary}

%prep
%setup -q -n %{name}-%{version}

%build

%qtc_qmake5 SPECVERSION=%{version}

%qtc_make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%qmake5_install

desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/harbour-taskswitcher
%attr(755,root,root) %{_bindir}/harbour-taskswitcher-user
%{_datadir}/harbour-taskswitcher-user/
%{_datadir}/applications
%{_datadir}/icons/hicolor/*/apps/%{name}-user.png
/usr/lib/systemd/user/harbour-taskswitcher.service
/usr/lib/systemd/user/harbour-taskswitcher-user.service
%{_datadir}/jolla-settings/entries/


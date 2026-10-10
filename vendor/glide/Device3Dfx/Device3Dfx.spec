Summary: Device driver for 3Dfx boards for Linux kernels
Name: Device3Dfx
Version: 2.3
Release: 5
Icon: 3dfx.gif
Source: Device3Dfx.tar.gz
License: GPL
Group: Drivers

%changelog

* Sat Apr 08 2000 Joseph Kain <joseph@3dfx.com>
  Release 2.3-5
- Check both the kernel and the processor for MTRR support before enabling.

* Wed Jan 04 2000 Joseph Kain <joseph@3dfx.com>
  Release 2.3-4

- Forgot to include kinfo.h in 3dfx_driver.c in the last release.   This 
  fixes some build problems for SMP kernels.

* Wed Dec 22 1999 Joseph Kain <joseph@3dfx.com>
  Release 2.3-3

- Fixes to the Makefile to stop grepping the header files.  Changes to 
  mtrr.c and 3dfx_driver.c to support these Makefile changes.
- 3dfx_driver.c now supports 2.3.14+ kernels.

* Mon Dec 13 1999 Joseph Kain <joseph@3dfx.com>
  Release 2.3-2
  
- Removed depmod -a from the Makefile.  This elliminates errors in the
  build on systems with modules with broken depedancies (Happens on Madrake)
- Changed depmod -a to depmod -a > /dev/null in the post and postun to
  elliminate any warnings/errors with modules that don't have anything
  to do with Device3Dfx.  Warnings scare people.

* Tue Nov 09 1999 Joseph Kain <joseph@3dfx.com>
  Release 2.3:

- Made cards static to remove conflict with hisax ISDN driver.
- Removed 3dfx.o from the source distribution

* Fri Jun 25 1999 Daryll Strauss <daryll@harlot.rb.ca.us>

  Release 2.2:

- Set MTRR for VB/V3 boards
- Allow build without kernel sources installed
- Code cleanup
- Improvements in /etc/conf.modules setup

* Sun May 16 1999 Daryll Strauss <daryll@harlot.rb.ca.us>

  Release 2.1:

- Added support for multiple board mappings
- Enlarged size of mappings
- Changes to support VB/V3 boards

* Sun Apr 4 1999 Daryll Strauss <daryll@harlot.rb.ca.us>

  Release 2.0-1:

- This is just a rename of Michael's version to return it to my original 
  naming scheme.
- Added the License field to the spec file.

* Thu Mar 11 1999 Michael Vance <mkv102@psu.edu>

  Release 2.5-2:

- Fixed for 2.2.3 kernels because of an mmap() update

* Fri Oct 30 1998 Carlo Wood <carlo@runaway.xs4all.nl>

  Release 2.5-1:

- Upped source to Dev3Dfx-2.5.tar.gz
- Boosted version number of rpm to the version of the tar.gz.
  Moved everything to the Makefile, so Dev3Dfx-2.5.tar.gz is
  self containing and will work without rpm too.
  
* Wed Oct 21 1998 Carlo Wood <carlo@runaway.xs4all.nl>

  Release 1.2-5:

- Upped source to Dev3Dfx-2.4.tar.gz
- Removed dangerous and confusing use of /lib/modules/preferred
- Removed the use of `uname': It now will compile for the kernel
  version in /usr/src/linux and not be bothered by the running
  kernel version.

* Wed Oct 21 1998 Carlo Wood <carlo@runaway.xs4all.nl>

  Release 1.2-4:

- Upped source to Dev3Dfx-2.3.tar.gz

* Mon Oct 19 1998 Carlo Wood <carlo@runaway.xs4all.nl>
 
  Release 1.2-3:

- Corrected Summary and Description to refer to 2.x rather
  then 2.1 kernels.
- Removed script lines that write an `option' line to /etc/conf.modules.
- Upped source to Dev3Dfx-2.2.tar.gz

* Sun Oct 18 1998 Carlo Wood <carlo@runaway.xs4all.nl>

  Release 1.2-2:

- Added OPT_CFLAGS stuff.
  
* Sun Oct 18 1998 Carlo Wood <carlo@runaway.xs4all.nl>

  Release 1.2-1:

- Packaged version 1.2 with support for 2.1 kernels by John Taylor and
  MTRR settings added by Jens Axboe.
 
%description
This package installs the 3Dfx device driver to allow access to 3Dfx
boards without the user having root privledges.  It should work on >= 2.x
kernels and set the MTRR settings correctly.  It should also work with
SMP kernels (>= 2.1).

%prep
%setup -c

%build
make OPT_CFLAGS="$RPM_OPT_FLAGS"

%install
make RPM_INSTALL="1" install | grep '^/lib/modules/' > modules-file-list

%post
if [ "$1" = 1 ]; then
	grep -v 3dfx /etc/conf.modules > /etc/conf.modules.tmp
	echo alias char-major-107 3dfx >> /etc/conf.modules.tmp
	mv /etc/conf.modules.tmp /etc/conf.modules
fi
/sbin/depmod -a > /dev/null

%postun
if [ "$1" = 0 ]; then
	grep -v 3dfx /etc/conf.modules > /etc/conf.modules.tmp
	mv /etc/conf.modules.tmp /etc/conf.modules
fi
/sbin/depmod -a > /dev/null

%verifyscript
inconf=`grep 'alias char-major-107 3dfx' /etc/conf.modules`
if [ "x$inconf" = "x" ]; then
	echo "3dfx entry not included in /etc/conf.modules"
fi

%files -f modules-file-list
%verify(not mode user group) /dev/3dfx


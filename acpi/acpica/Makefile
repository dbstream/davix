# SPDX-License-Identifier: MIT

obj-y += dsargs.c.o dscontrol.c.o dsdebug.c.o dsfield.c.o dsinit.c.o \
	dsmethod.c.o dsmthdat.c.o dsobject.c.o dsopcode.c.o dspkginit.c.o \
	dsutils.c.o dswexec.c.o dswload2.c.o dswload.c.o dswscope.c.o \
	dswstate.c.o

obj-y += evevent.c.o evglock.c.o evgpeblk.c.o evgpe.c.o evgpeinit.c.o \
	evgpeutil.c.o evhandler.c.o evmisc.c.o evregion.c.o evrgnini.c.o \
	evsci.c.o evxface.c.o evxfevnt.c.o evxfgpe.c.o evxfregn.c.o

obj-y += exconcat.c.o exconfig.c.o exconvrt.c.o excreate.c.o exdebug.c.o \
	exdump.c.o exfield.c.o exfldio.c.o exmisc.c.o exmutex.c.o exnames.c.o \
	exoparg1.c.o exoparg2.c.o exoparg3.c.o exoparg6.c.o exprep.c.o \
	exregion.c.o exresnte.c.o exresolv.c.o exresop.c.o exserial.c.o \
	exstore.c.o exstoren.c.o exstorob.c.o exsystem.c.o extrace.c.o \
	exutils.c.o

obj-y += hwacpi.c.o hwesleep.c.o hwgpe.c.o hwpci.c.o hwregs.c.o hwsleep.c.o \
	hwtimer.c.o hwvalid.c.o hwxface.c.o hwxfsleep.c.o

obj-y += nsaccess.c.o nsalloc.c.o nsarguments.c.o nsconvert.c.o nsdump.c.o \
	nsdumpdv.c.o nseval.c.o nsinit.c.o nsload.c.o nsnames.c.o nsobject.c.o \
	nsparse.c.o nspredef.c.o nsprepkg.c.o nsrepair2.c.o nsrepair.c.o \
	nssearch.c.o nsutils.c.o nswalk.c.o nsxfeval.c.o nsxfname.c.o \
	nsxfobj.c.o

obj-y += psargs.c.o psloop.c.o psobject.c.o psopcode.c.o psopinfo.c.o \
	psparse.c.o psscope.c.o pstree.c.o psutils.c.o pswalk.c.o psxface.c.o

obj-y += rsaddr.c.o rscalc.c.o rscreate.c.o rsdump.c.o rsdumpinfo.c.o \
	rsinfo.c.o rsio.c.o rsirq.c.o rslist.c.o rsmemory.c.o rsmisc.c.o \
	rsserial.c.o rsutils.c.o rsxface.c.o

obj-y += tbdata.c.o tbfadt.c.o tbfind.c.o tbinstal.c.o tbprint.c.o tbutils.c.o \
	tbxface.c.o tbxfload.c.o tbxfroot.c.o

obj-y += utaddress.c.o utalloc.c.o utascii.c.o utbuffer.c.o utcache.c.o \
	utcksum.c.o utcopy.c.o utdebug.c.o utdecode.c.o utdelete.c.o \
	uterror.c.o uteval.c.o utexcep.c.o utglobal.c.o uthex.c.o utids.c.o \
	utinit.c.o utlock.c.o utmath.c.o utmisc.c.o utmutex.c.o utnonansi.c.o \
	utobject.c.o utosi.c.o utownerid.c.o utpredef.c.o utprint.c.o \
	utresdecode.c.o utresrc.c.o utstate.c.o utstring.c.o utstrsuppt.c.o \
	utstrtoul64.c.o uttrack.c.o utuuid.c.o utxface.c.o utxferror.c.o \
	utxfinit.c.o utxfmutex.c.o

# The Intel gods decided to write non-optimal source code for
# `sparse -Wsparse-all -Wsparse-error`, and I'm not going to
# manually change every single source file to fix it.
SPARSEFLAGS += -Wno-decl
SPARSEFLAGS += -Wno-constant-suffix
SPARSEFLAGS += -Wno-constexpr-not-const
SPARSEFLAGS += -Wno-init-cstring
SPARSEFLAGS += -Wno-flexible-array-nested
SPARSEFLAGS += -Wno-flexible-array-sizeof
CFLAGS-utdebug.c.o += -Wno-dangling-pointer

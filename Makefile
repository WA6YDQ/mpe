all		:	dir.o copy.o rename.o new.o create.o type.o delete.o led.o date.o \
			readonly.o readwrite.o public.o private.o sh.o shtest.o \
			status.o help.o concat.o qsort.o rmdup.o ldir.o dump.o getenv.o \
			crypt.o echo.o slist.o

		cc -o dir dir.o
		cc -o copy copy.o
		cc -o rename rename.o
		cc -o new new.o
		cc -o create create.o
		cc -o type type.o
		cc -o delete delete.o
		cc -o led led.o
		cc -o date date.o
		cc -o readonly readonly.o
		cc -o readwrite readwrite.o
		cc -o public public.o
		cc -o private private.o
		cc -o sh sh.o -lreadline
		cc -o shtest shtest.o
		cc -o status status.o
		cc -o help help.o
		cc -o concat concat.o
		cc -o qsort qsort.o
		cc -o rmdup rmdup.o
		cc -o ldir ldir.o
		cc -o dump dump.o
		cc -o getenv getenv.o
		cc -o crypt crypt.o
		cc -o echo echo.o
		cc -o slist slist.o

INSTALLDIR = /mini/mpe
install	:
		cp dir ${INSTALLDIR}/cmds/dir
		cp copy ${INSTALLDIR}/cmds/copy
		cp rename ${INSTALLDIR}/cmds/rename
		cp new ${INSTALLDIR}/cmds/new
		cp create ${INSTALLDIR}/cmds/create
		cp type ${INSTALLDIR}/cmds/type
		cp delete ${INSTALLDIR}/cmds/delete
		cp led ${INSTALLDIR}/cmds/led
		cp date ${INSTALLDIR}/cmds/date
		cp readonly ${INSTALLDIR}/cmds/readonly
		cp readwrite ${INSTALLDIR}/cmds/readwrite
		cp public ${INSTALLDIR}/cmds/public
		cp private ${INSTALLDIR}/cmds/private
		unlink ${INSTALLDIR}/cmds/sh
		cp sh ${INSTALLDIR}/cmds/sh
		cp shtest ${INSTALLDIR}/cmds/shtest
		cp status ${INSTALLDIR}/cmds/status
		cp help ${INSTALLDIR}/cmds/help
		cp concat ${INSTALLDIR}/cmds/concat
		cp qsort ${INSTALLDIR}/cmds/qsort
		cp rmdup ${INSTALLDIR}/cmds/rmdup
		cp ldir ${INSTALLDIR}/cmds/ldir
		cp dump ${INSTALLDIR}/cmds/dump
		cp *.hlp ${INSTALLDIR}/helpfiles/
		cp basic.reference ${INSTALLDIR}/docs/basic.reference
		cp getenv ${INSTALLDIR}/cmds/getenv
		cp *.ref ${INSTALLDIR}/docs/
		cp crypt ${INSTALLDIR}/cmds/crypt
		cp echo ${INSTALLDIR}/cmds/echo
		cp slist ${INSTALLDIR}/cmds/slist







# individual compiles

dir		:	dir.o
		cc -o dir dir.o

copy	:	copy.o
		cc -o copy copy.o

rename	:	rename.o
		cc -o rename rename.o

create	:	create.o
		cc -o create create.o

type	:	type.o
		cc -o type type.o

delete	:	delete.o
		cc -o delete delete.o

led		:	led.o
		cc -o led led.o

date	:	date.o
		cc -o date date.o

readonly	:	readonly.o
		cc -o readonly readonly.o

readwrite	:	readwrite.o
		cc -o readwrite readwrite.o

public	:	public.o
		cc -o public public.o

private	:	private.o
		cc -o private private.o

new	:	new.o
		cc -o new new.o

sh	:	sh.o
		cc -o sh sh.o -lreadline

shtest	:	shtest.o
		cc -o shtest shtest.o

status	:	status.o
		cc -o status status.o

help	:	help.o
		cc -o help help.o

concat	:	concat.o
		cc -o concat concat.o

qsort	:	qsort.o
		cc -o qsort qsort.o

rmdup	:	rmdup.o
		cc -o rmdup rmdup.o

ldir	:	ldir.o
		cc -o ldir ldir.o

dump	:	dump.o
		cc -o dump dump.c

getenv	:	getenv.o
		cc -o getenv getenv.c

crypt	:	crypt.o
		cc -o crypt crypt.c

echo	:	echo.o
		cc -o echo echo.c

slist	:	slist.o
		cc -o slist slist.c


# remove compilation files
clean	:
		/bin/rm *.o
	
		


#build files
FLAGS=

dir.o	:	dir.c
	cc -c ${FLAGS} dir.c
copy.o	:	copy.c
	cc -c ${FLAGS} copy.c
rename.o	: rename.c
	cc -c ${FLAGS} rename.c
create.o	:	create.c
	cc -c ${FLAGS} create.c
type.o	:	type.c
	cc -c ${FLAGS} type.c
delete.o	:	delete.c
	cc -c ${FLAGS} delete.c
led.o		:	led.c
	cc -c ${FLAGS} led.c
date.o	:	date.c
	cc -c ${FLAGS} date.c
readonly.o	:	readonly.c
	cc -c ${FLAGS} readonly.c
readwrite.o	:	readwrite.c
	cc -c ${FLAGS} readwrite.c
public.o	:	public.c
	cc -c ${FLAGS} public.c
private.o	:	private.c
	cc -c ${FLAGS} private.c
new.o	:	new.c
	cc -c ${FLAGS} new.c
sh.o	:	sh.c
	cc -c ${FLAGS} sh.c
shtest.o	:	shtest.c
	cc -c ${FLAGS} shtest.c
status.o	:	status.c
	cc -c ${FLAGS} status.c
help.o	:	help.c
	cc -c ${FLAGS} help.c
concat.o	:	concat.c
	cc -c ${FLAGS} concat.c
qsort.o	:	qsort.c
	cc -c ${FLAGS} qsort.c
rmdup.o	:	rmdup.c
	cc -c ${FLAGS} rmdup.c
ldir.o	:	ldir.c
	cc -c ${FLAGS} ldir.c
dump.o	:	dump.c
	cc -c ${FLAGS} dump.c
getenv.o	:	getenv.c
	cc -c ${FLAGS} getenv.c
crypt.o	:	crypt.c
	cc -c ${FLAGS} crypt.c
echo.o	:	echo.c
	cc -c ${FLAGS} echo.c
slist.o	:	slist.c
	cc -c ${FLAGS} slist.c



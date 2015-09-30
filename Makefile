INCLUDE+=-I.
WARN_CPPFLAGS=-Werror -Wall -Wextra -Wold-style-cast -Wsign-promo -Wfloat-equal -Wshadow -Wcast-qual -Wzero-as-null-pointer-constant -Wno-non-virtual-dtor -Wstack-usage=8192 -fmax-errors=10 -std=c++11 -pedantic -Wno-unused-but-set-variable
#-Wconversion 
COMMON_CPPFLAGS?=$(WARN_CPPFLAGS) -DHAVE_SYSLOG_H -D_LARGEFILE64_SOURCE
ifdef DEBUG
	CPPFLAGS?=-O0 -g -fno-inline -march=native
else
	CPPFLAGS?=-O9 -march=native -fomit-frame-pointer -pipe -DNDEBUG
endif

CPPFLAGS+=$(INCLUDE) $(COMMON_CPPFLAGS)
LDFLAGS+=
LOADLIBES+=

ALL:=client server
all: $(ALL)
clean:
	rm -f $(ALL)

EXT_SRC=ext/mkstr.cpp ext/escape.cpp
UNISTD_SRC=unistd/addrinfo.cpp unistd/netdb.cpp unistd/time.cpp unistd/unistd.cpp ${EXT_SRC}
server: server.cpp ${UNISTD_SRC}
client: client.cpp ${UNISTD_SRC}


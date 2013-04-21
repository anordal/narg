
PREFIX ?= /usr/local
CFLAGS ?= -Os -Wall
MANSEC ?= 3
MANDIR ?= $(PREFIX)/share/man/man$(MANSEC)

CFLAGS += -std=gnu99 -fPIC

NAM := $(notdir $(CURDIR))
HED := $(wildcard *.h)
OBJ := $(patsubst %.c, %.o, $(wildcard *.c))

EXP := lib$(NAM).h
STA := lib$(NAM).a
SON := lib$(NAM).so.0
DYN := $(SON).0.1

.PHONY: all
all: $(DYN)
.PHONY: doc
doc: $(MAN)

.PHONY: clean
clean:
	rm -f $(OBJ) $(DYN) $(STA) $(MAN)
.PHONY: install
install: $(DYN) $(EXP) |$(PREFIX)/lib/ $(PREFIX)/include/
	install $(DYN) $(PREFIX)/lib/
	install $(EXP) $(PREFIX)/include/
	ldconfig
.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/lib/$(DYN)
	rm -f $(PREFIX)/include/$(EXP)

$(DYN): $(OBJ)
	gcc -shared -Wl,-soname,$(SON) $(OBJ) -o $(DYN)
$(STA): $(OBJ)
	ar rcs $@ $<
%.o: %.c $(HED)
	$(CC) -c $(CFLAGS) $< -o $@
%/:
	mkdir -p $@

MAN := lib$(NAM).$(MANSEC).gz

.PHONY: install-doc
install-doc: $(MAN) |$(MANDIR)/
	install $(MAN) $(MANDIR)/
.PHONY: uninstall-doc
uninstall-doc:
	rm -f $(MANDIR)/$(MAN)

%.gz: %
	gzip $<
%.$(MANSEC): %.markdown
	md2man $< > $@
%.html: %.markdown
	md2man-html $< > $@


PREFIX ?= /usr/local
CFLAGS ?= -Os -Wall
MANSEC ?= 3
MANDIR ?= $(PREFIX)/share/man/man$(MANSEC)

CFLAGS += -std=gnu99

NAM := $(notdir $(CURDIR))
HED := $(wildcard *.h)
OBJ := $(patsubst %.c, %.o, $(wildcard *.c))

EXP := lib$(NAM).h
STA := lib$(NAM).a
SON := lib$(NAM).so.0
DYN := $(SON).0.1
MAN := lib$(NAM).$(MANSEC).gz
TST := $(wildcard test/test_*.c)

.PHONY: all
all: $(DYN)
.PHONY: doc
doc: $(MAN)

.PHONY: clean
clean:
	rm -f $(OBJ) $(DYN) $(STA) $(MAN) $(patsubst %.c, %.o, $(TST)) $(patsubst %.c, %, $(TST))
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
	gcc -shared -Wl,-soname,$(SON) $^ -o $@
$(STA): $(OBJ)
	ar rcs $@ $^
%.o: %.c $(HED)
	$(CC) -c $(CFLAGS) -fPIC $< -o $@
%/:
	mkdir -p $@

.PHONY: install-doc
install-doc: $(MAN) |$(MANDIR)/
	install $(MAN) $(MANDIR)/
.PHONY: uninstall-doc
uninstall-doc:
	rm -f $(MANDIR)/$(MAN)

%.gz: %
	gzip -f $<
%.$(MANSEC): %.md
	md2man $< > $@
%.html: %.md
	md2man-html $< > $@

TESTS := $(patsubst test/test_%.c, test/run_%, $(TST))

.PHONY: test $(TESTS)
test: $(TESTS)
	@echo TESTS PASSED

$(TESTS): test/run_% : test/test_%
	$<

test/%: test/%.c $(STA)
	$(CC) $(CFLAGS) $^ -o $@

CC = gcc
CFLAGS = -Wall -Wextra -O2
BINDIR = build

all: $(BINDIR)/parent $(BINDIR)/child

$(BINDIR)/parent: parent.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/parent parent.c

$(BINDIR)/child: child.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/child child.c

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR)

.PHONY: all clean
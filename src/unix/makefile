OBJDIR = ../../obj.$(ARCH)/unix

SRCS = main.c sound.c keyboard.c xdep.c linux.c solaris.c sunos.c irix.c hpux.c
SNDSRCS = sndserver.c

OBJS = $(OBJDIR)/main.o $(OBJDIR)/sound.o $(OBJDIR)/xdep.o $(OBJDIR)/$(ARCH).o \
	$(OBJDIR)/lin2ulaw.o $(OBJDIR)/keyboard.o

SNDOBJS = $(OBJDIR)/sndserver.o


TARGET = osdepend.a $(OBJDIR)/sndserver.o

osdepend.a: $(OBJS)
	ar -rc $(OBJDIR)/osdepend.a $(OBJS)

$(OBJDIR)/%.o: %.c xmame.h
	$(CC) $(DEFS) -I../../src/ $(MS) $(JS) $(CFLAGS) -o $@ -c $<
	
$(OBJDIR)/main.o: main.c xmame.h
$(OBJDIR)/sound.o: sound.c xmame.h
$(OBJDIR)/keyboard.o: keyboard.c xmame.h
$(OBJDIR)/xdep.o: xdep.c xmame.h
$(OBJDIR)/linux.o: linux.c xmame.h
$(OBJDIR)/solaris.o: solaris.c xmame.h lin2ulaw.h
$(OBJDIR)/lin2ulaw.o: lin2ulaw.c xmame.h lin2ulaw.h
$(OBJDIR)/sunos.o: sunos.c xmame.h
$(OBJDIR)/irix.o: irix.c xmame.h
$(OBJDIR)/hpux.o: hpux.cxmame.h
$(OBJDIR)/sndserver.o: sndserver.c xmame.h

CFLAGS=-Wall -Werror
TARGET=router

vpath %.c common
vpath %.h common

OBJDIR = obj

OBJS =  $(OBJDIR)/$(TARGET).o \
        $(OBJDIR)/logging.o \
        $(OBJDIR)/cmd.o \
        $(OBJDIR)/line.o \
        $(OBJDIR)/serial.o \
        $(OBJDIR)/escape.o \
        $(OBJDIR)/mqtt.o \
        $(OBJDIR)/midpublist.o \
        $(OBJDIR)/midsublist.o \
        $(OBJDIR)/midunsublist.o \
        $(OBJDIR)/addrsublist.o 

CFLAGS += -L/usr/local/lib -lmosquitto -lev -I. -I/usr/local/include

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

$(OBJDIR)/%.o : %.c *.h common/*.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(TARGET) $(TARGET).exe $(OBJDIR) *.o


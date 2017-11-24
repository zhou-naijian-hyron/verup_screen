#######
#
# Common Lib Makefile
#

TOPDIR =$(PWD)/..
include $(TOPDIR)/make.rules

####
# set target & soruce file
TARGET  = sample_dfb
SRC1    = verup_dfb_api.c
SRC2    = sample_dfb.c
SRCS    = $(SRC1) $(SRC2)

OBJ1    = $(SRC1:.c=.o)
OBJ2    = $(SRC2:.c=.o)
OBJS    = $(OBJ1) $(OBJ2)

####
# set include and library
INCS += -I$(TOPDIR)/lib/directFB/include/directfb
LIBS += -L$(TOPDIR)/lib/directFB/lib -lstdc++ -ldirect -ldirectfb -l++dfb -lfusion

####
# set compile
all : $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS_DEF) -o $(TARGET) $(OBJS) $(LIBS)
.c.o:
	$(CC) $(CFLAGS_DEF) $(INCS) $(LIBS) -c $<

clean:
	rm -rf $(OBJS) $(TARGET)

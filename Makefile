include ../../Rule.mk

BIN = err_disable 

CFLAGS = -g -Wall -D_REENTRANT -D__LINUX__ 

#added by xichen for FS3600-10
ifeq ($(MODULE),FS3600-10) 
CFLAGS += -DFS3600-10
endif

INCLUDES = -I./ -I../bcmutils 
LIBS= -L../bcmutils -lbcmutils
CFLAGS += -DIMP=\"$(IMP)\" -DPNUM=$(PNUM) -DGNUM=$(GNUM)

OBJS_1  =  err_disable.o ../bcmutils/sk_define.o
#OBJS_1  =  err_disable.o
all: $(BIN)

err_disable:$(OBJS_1)
	$(CC)  $(CFLAGS) $(OBJS_1) $(LIBS) -o $@
	

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

install:
	$(STRIP) $(BIN)
	cp -a $(BIN) ../../target/usr/sbin
	
clean:
	rm -f $(BIN) *.o


#############################################################################
# Makefile for building: BuggyApp
# Generated by qmake (3.1) (Qt 5.11.3)
# Project:  ../BuggyApp.pro
# Template: app
# Command: /home/sergei/Qt/5.11.3/gcc_64/bin/qmake -o Makefile ../BuggyApp.pro -spec linux-g++ CONFIG+=qtquickcompiler
#############################################################################

MAKEFILE      = Makefile

####### Compiler, tools and options

CC			= gcc
CXX			= g++
CFLAGS		= -pipe -O3 -Wall -W -fPIC 
CXXFLAGS	= $(CFLAGS) -std=gnu++1z
INCPATH		= -I.

DEL_FILE	= rm -f

LINK		= g++
LFLAGS		= 
#LFLAGS		= -Wl,-O3
LIBS		= -L/usr/local/lib -lpthread 

####### Files
SOURCES	= \
		main.cpp \
		grody/picohttpparser/picohttpparser.c \
		grody/webserver.c \
		grody/thread.c \
		grody/io.c \
		grody/fork.c 
#OBJECTS	= \
#		main.o \
#		picohttpparser.o \
#		webserver.o \
#		thread.o \
#		io.o \
#		fork.o
OBJcpp	= $(SOURCES:.cpp=.o)
OBJECTS = $(OBJcpp:.c=.o)

HEADERS	= \
        grody/picohttpparser/picohttpparser.h \
		grody/threaded_webserver.h \
		grody/webserver.h \
		grody/thread.h \
		grody/fork.h

TARGET	= BuggyApp


####### Build rules
$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

all: $(TARGET)

debug: CXXFLAGS += -DDEBUG -g
debug: CFLAGS += -DDEBUG -g
debug: all


clean:  
	-$(DEL_FILE) $(OBJECTS) $(TARGET)

####### Compile
$(HEADERS):

main.o: main.cpp $(HEADERS)
	$(CXX) -c $(CFLAGS) $(INCPATH) -o $@ $<

picohttpparser.o: grody/picohttpparser/picohttpparser.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

webserver.o: grody/webserver.c grody/webserver.h $(HEADERS)
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

thread.o: grody/thread.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

io.o: grody/io.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

fork.o: grody/fork.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<



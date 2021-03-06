
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
		grody/single_thread.c \
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
		grody/single_thread.h \
		grody/fork.h

TARGET	= BuggyApp


####### Build rules
all: $(TARGET)

debug:	CFLAGS		= -pipe -DDEBUG -g -Wall -W -fPIC 
debug:	CXXFLAGS	= $(CFLAGS) -std=gnu++1z
debug:	all

clean:  
	-$(DEL_FILE) $(OBJECTS)

$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

####### Compile
$(HEADERS):

%.o: %.cpp $(HEADERS)
	$(CXX) -c $(CFLAGS) $(INCPATH) -o $@ $<
%.o: grody/picohttpparser/%.c grody/%.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

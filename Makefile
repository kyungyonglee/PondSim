CC=g++
CFLAGS=-c -Wall -g
LDFLAGS=
SOURCES=event_test.cpp event_scheduler.cpp event_action.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=event_test

all: $(SOURCES) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXECUTABLE)

event_test.o: $(SOURCES)
	$(CC) $(CFLAGS) event_test.cpp

event_scheduler.o: event_scheduler.cpp event_scheduler.h
	$(CC) $(CFLAGS) event_scheduler.cpp

event_action.o : event_action.cpp event_action.h
	$(CC) $(CFLAGS) event_action.cpp

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXECCUTABLE)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $(EXECUTABLE)

clean:
	rm -rf *o event_test


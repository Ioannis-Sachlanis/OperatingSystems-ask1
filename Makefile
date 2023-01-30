all:	parent child

parent:	parent.o shared.o
	g++ -o parent parent.o shared.o

parent.o: parent.cpp
	g++ -c parent.cpp

shared.o: shared.cpp
	g++ -c shared.cpp

child: child.o shared.o
	g++ -o child child.o shared.o

child.o: child.cpp
	g++ -c child.cpp

clean:
	rm -f *.o child parent log*.txt

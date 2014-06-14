

CXXFLAGS=-Wall -O3 -std=gnu++11 -I.
CFLAGS=-Wall -O3 -I. -std=gnu99

INCLUDE_GRAPH:=include-graph.o type2str.o linkage2str.o

include-graph: $(INCLUDE_GRAPH)
	$(CC) -o $@ $(INCLUDE_GRAPH) -Llib -lclang 

clean:
	$(RM) $(INCLUDE_GRAPH) include-graph

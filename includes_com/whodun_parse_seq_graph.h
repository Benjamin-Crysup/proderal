#ifndef WHODUN_PARSE_SEQ_GRAPH_H
#define WHODUN_PARSE_SEQ_GRAPH_H 1

#include <map>
#include <vector>
#include <iostream>

#include "whodun_datread.h"
#include "whodun_compress.h"
#include "whodun_parse_seq.h"

/**A sequence graph.*/
class SequenceGraph{
public:
	/**Set up an empty graph.*/
	SequenceGraph();
	/**Tear down.*/
	~SequenceGraph();
	/**The sequence.*/
	std::vector<char> seqStore;
	/**The forward jumps (jump before, jump to), sorted by jump before.*/
	std::vector< std::pair<uintptr_t,uintptr_t> > forwJumps;
	/**The backward jumps (jump to, jump before), sorted by jump to.*/
	std::vector< std::pair<uintptr_t,uintptr_t> > backJumps;
};

/**Read a sequence graph file.*/
class SequenceGraphReader : public SequenceReader{
public:
	/**The number of nodes in the graph.*/
	uintptr_t numNode;
	/**The names of each node.*/
	char** nodeNames;
	/**The character index each node targets.*/
	uintptr_t* nodeCharInd;
	/**The number of locations jumps occur at.*/
	uintptr_t numJumps;
	/**The character index before which each jump happens.*/
	uintptr_t* jumpCharInd;
	/**The number of locations the jumps go to.*/
	uintptr_t* jumpNumJumps;
	/**The names of the targets of each jump.*/
	char*** jumpTgtNames;
	/**Whether each jump allows pass through.*/
	char* jumpPassThrough;
	
	/**
	 * Wrap a fasta file to parse graphs.
	 * @param toWrap The thing to wrap.
	 */
	SequenceGraphReader(SequenceReader* toWrap);
	/**Clean up*/
	~SequenceGraphReader();
	int readNextEntry();
	
	/**
	 * Flatten the graph for future use.
	 * @param storeGraph The place to put the flattened graph.
	 */
	void flattenGraph(SequenceGraph* storeGraph);
	
	/**The base reader: fastcf is built on top of fastq.*/
	SequenceReader* baseReader;
	/**The allocation for the converted sequence.*/
	std::vector<char> seqStore;
	/**Storage for the node names.*/
	std::vector<char> nodeNameStore;
	/**Indirect storage.*/
	std::vector<char*> nodeNameStoreP;
	/**Storage for nodeCharInd.*/
	std::vector<uintptr_t> nodeCharIndV;
	/**Storage for jumpCharInd.*/
	std::vector<uintptr_t> jumpCharIndV;
	/**Storage for jumpNumJumps.*/
	std::vector<uintptr_t> jumpNumJumpsV;
	/**Storage for target names.*/
	std::vector<char> jumpTgtStore;
	/**More general storage.*/
	std::vector<char*> jumpTgtStoreP;
	/**Yet more general storage.*/
	std::vector<char**> jumpTgtStorePP;
	/**Storage for pass through.*/
	std::vector<char> jumpPassThroughV;
	/**Temporary storage for a map from node names to node indices.*/
	std::map<std::string,uintptr_t> nodeNameMap;
	/**Temporary storage for nodes lacking a self link.*/
	std::set<uintptr_t> nodeSelfL;
};

#endif

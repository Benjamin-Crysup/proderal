#ifndef WHODUN_NMCY_H
#define WHODUN_NMCY_H 1

/*Never measure, cut yourself.*/

/**Like std::pair.*/
template <typename TA, typename TB, typename TC>
class triple{
public:
	/**The first thing.*/
	TA first;
	/**The second thing.*/
	TB second;
	/**The third thing.*/
	TC third;
	
	/**Set up an uniniailized tuple.*/
	triple(){}
	
	/**
	 * Set up a tuple with the given values.
	 * @param myF The first value.
	 * @param myS The second value.
	 * @param myT The third value.
	 */
	triple(TA myF, TB myS, TC myT){
		first = myF;
		second = myS;
		third = myT;
	}
	
	/**Compare element by element.*/
	bool operator < (const triple<TA,TB,TC>& compTo) const{
		if(first < compTo.first){ return true; }
		if(compTo.first < first){ return false; }
		if(second < compTo.second){ return true; }
		if(compTo.second < second){ return false; }
		if(third < compTo.third){ return true; }
		return false;
	}
};


#endif
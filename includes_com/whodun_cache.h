#ifndef WHODUN_CACHE_H
#define WHODUN_CACHE_H 1

#include <vector>

/**Allocate containers in a reusable manner: can cut down on allocation.*/
template <typename OfT>
class ReusableContainerCache{
public:
	/**Make an empty cache.*/
	ReusableContainerCache(){}
	/**Destroy the allocated stuff.*/
	~ReusableContainerCache(){
		for(uintptr_t i = 0; i<canReuse.size(); i++){
			delete(canReuse[i]);
		}
	}
	/**
	 * Allocate a container.
	 * @return The allocated container. Must be returned with dealloc.
	 */
	OfT* alloc(){
		OfT* toRet = 0;
		if(canReuse.size()){
			toRet = canReuse[canReuse.size()-1];
			canReuse.pop_back();
		}
		else{
			toRet = new OfT();
		}
		return toRet;
	}
	
	/**
	 * Return a container.
	 * @param toDe The container to return.
	 */
	void dealloc(OfT* toDe){
		canReuse.push_back(toDe);
	}
	
	/**All the containers waiting to be reused.*/
	std::vector<OfT*> canReuse;
};

#endif

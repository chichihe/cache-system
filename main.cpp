#include"CacheBase.h"
#include"LRUCache.h"
#include<iostream>
int main(void) {
	Cache::LRUNode<int, int> aa(1,1);
	std::cout << aa.getAccessCount() << std::endl;
	aa.increAccessCount();
	std::cout << aa.getAccessCount() << std::endl;
	Cache::LRUCache<int, int> ss(2);
	ss.put(1, 1);
	ss.put(2, 2);
	ss.get(1);
	ss.put(3, 3);
	ss.get(2);
	ss.put(4, 4);
	ss.get(3);
	ss.get(4);
}
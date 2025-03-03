#pragma once
#include"CacheBase.h"
#include<memory>
#include<atomic>
#include<mutex>
#include<unordered_map>
namespace Cache {
	template<typename Key, typename Value> class LRUCache;

	template<typename Key, typename Value>
	class LRUNode {
	private:
		Key _key;
		Value _value;
		std::atomic<size_t> _accessCount;// 最好使用原子变量
		// 使用weak_ptr避免循环引用导致的内存泄露
		std::weak_ptr<LRUNode<Key, Value>> prev;
		std::shared_ptr<LRUNode<Key, Value>> next;
		// mutable std::mutex _mutex;  // 保护_value的互斥锁 加入后是否会严重影响性能？
	public:
		// 构造函数
		LRUNode(Key key, Value value)
			:_key(key),
			_value(value),
			_accessCount(1),
			next(nullptr),
			prev()
		{};
		Key getKey() { return _key; }
		Value getValue() { return _value; }// 考虑线程安全std::lock_guard<std::mutex> lock(_mutex);
		void setValue(Value value) { _value = value; }// 考虑线程安全getValue执行时不允许对值进行修改
		size_t getAccessCount() { return _accessCount.load(std::memory_order_relaxed); }//使用load return _accessCount.load(std::memory_order_relaxed); 
		void increAccessCount() { _accessCount.fetch_add(1, std::memory_order_relaxed); }// 自增操作不是线程安全的
		//相比++_accessCount性能更好

		// getNext() getPrev() setNext setPrev
		friend class LRUCache<Key, Value>;
	};

	template<typename Key, typename Value>
	class LRUCache : public Cache::CachePolicy<Key,Value> {
		using LRUNode = LRUNode<Key, Value>;
		using LRUNodePtr = std::shared_ptr<LRUNode>;
		using LRUMap = std::unordered_map<Key, LRUNodePtr>;
	public:
		LRUCache(size_t capacity) : _capacity(capacity),_avaliable(capacity) {
			_initialize();
		//初始化双向链表左右的哨兵节点i
		}
		void put(Key key, Value value) override {
			std::lock_guard<std::mutex> lock (_mutex);//线程安全
			if (_node_map.find(key) != _node_map.end()) {//节点存在，更新其值，并将其移动到链表尾部
				LRUNodePtr node = _node_map[key];
				updateNodeValue(node,value);
			}
			else// 节点不存在，需要新加入节点
				addNewNode(key,value);
		
		}
		bool get(Key key, Value& value) override {
			std::lock_guard<std::mutex> lock(_mutex);// 访问时值不应被修改

			auto it = _node_map.find(key);
			if (it == _node_map.end()) return false;
			
			LRUNodePtr node = it->second;
			node->increAccessCount();
			value = node->getValue();
			moveMostRecentToTail(node);
			return true;
		}
		Value get(Key key) override {
			std::lock_guard<std::mutex> lock(_mutex);
			auto it = _node_map.find(key);
			if (it == _node_map.end()) return Value();
			
			LRUNodePtr node = it->second;
			node->increAccessCount();
			Value v = node->getValue();
			moveMostRecentToTail(node);
			return v;
		}

	private:
		// 内部双向链表+哈希结构
		size_t _capacity;
		size_t _avaliable;
		LRUNodePtr _dummy_head;
		LRUNodePtr _dummy_tail;
		LRUMap _node_map;
		mutable std::mutex _mutex;// 对数据进行修改时，需要使用互斥锁
		void _initialize() {
			_dummy_head = std::make_shared<LRUNode>(Key(), Value());
			_dummy_tail = std::make_shared<LRUNode>(Key(), Value());
			_dummy_head->next = _dummy_tail;
			_dummy_tail->prev = _dummy_head;
		}
		void insertToTail(LRUNodePtr node) {
			node->next = _dummy_tail;
			auto tail_prev = _dummy_tail->prev.lock();
			node->prev = tail_prev;
			tail_prev->next = node;
			_dummy_tail->prev = node;
		}
		void detachNode(LRUNodePtr node) {
			auto prev_node = node->prev.lock();
			node->next->prev = prev_node;
			if(prev_node)
				prev_node->next = node->next;
		}
		void updateNodeValue(LRUNodePtr node,Value new_value) {
			node->setValue(new_value);
			moveMostRecentToTail(node);
		}
		void eliminateLeastRecent() {
			LRUNodePtr least_recent_node = _dummy_head->next;
			if (least_recent_node == _dummy_tail)
				return;
			Key least_recent_key = least_recent_node->getKey();
			detachNode(least_recent_node);
			_node_map.erase(least_recent_key);
		}
		void moveMostRecentToTail(LRUNodePtr node){
			detachNode(node);
			insertToTail(node);
		}
		void addNewNode(Key key, Value value) {
			LRUNodePtr new_node = std::make_shared<LRUNode>(key, value);
			if (_avaliable) {
				_avaliable--;
			}
			else{// 容量已满，淘汰最不经常使用的
				eliminateLeastRecent();
			}
			insertToTail(new_node);
			_node_map[key] = new_node;
		}
	};

}

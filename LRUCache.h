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
		std::atomic<size_t> _accessCount;// ���ʹ��ԭ�ӱ���
		// ʹ��weak_ptr����ѭ�����õ��µ��ڴ�й¶
		std::weak_ptr<LRUNode<Key, Value>> prev;
		std::shared_ptr<LRUNode<Key, Value>> next;
		// mutable std::mutex _mutex;  // ����_value�Ļ����� ������Ƿ������Ӱ�����ܣ�
	public:
		// ���캯��
		LRUNode(Key key, Value value)
			:_key(key),
			_value(value),
			_accessCount(1),
			next(nullptr),
			prev()
		{};
		Key getKey() { return _key; }
		Value getValue() { return _value; }// �����̰߳�ȫstd::lock_guard<std::mutex> lock(_mutex);
		void setValue(Value value) { _value = value; }// �����̰߳�ȫgetValueִ��ʱ�������ֵ�����޸�
		size_t getAccessCount() { return _accessCount.load(std::memory_order_relaxed); }//ʹ��load return _accessCount.load(std::memory_order_relaxed); 
		void increAccessCount() { _accessCount.fetch_add(1, std::memory_order_relaxed); }// �������������̰߳�ȫ��
		//���++_accessCount���ܸ���

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
		//��ʼ��˫���������ҵ��ڱ��ڵ�i
		}
		void put(Key key, Value value) override {
			std::lock_guard<std::mutex> lock (_mutex);//�̰߳�ȫ
			if (_node_map.find(key) != _node_map.end()) {//�ڵ���ڣ�������ֵ���������ƶ�������β��
				LRUNodePtr node = _node_map[key];
				updateNodeValue(node,value);
			}
			else// �ڵ㲻���ڣ���Ҫ�¼���ڵ�
				addNewNode(key,value);
		
		}
		bool get(Key key, Value& value) override {
			std::lock_guard<std::mutex> lock(_mutex);// ����ʱֵ��Ӧ���޸�

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
		// �ڲ�˫������+��ϣ�ṹ
		size_t _capacity;
		size_t _avaliable;
		LRUNodePtr _dummy_head;
		LRUNodePtr _dummy_tail;
		LRUMap _node_map;
		mutable std::mutex _mutex;// �����ݽ����޸�ʱ����Ҫʹ�û�����
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
			else{// ������������̭�����ʹ�õ�
				eliminateLeastRecent();
			}
			insertToTail(new_node);
			_node_map[key] = new_node;
		}
	};

}

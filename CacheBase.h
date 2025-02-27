#pragma once
namespace Cache{
template<typename Key,typename Value>
class CachePolicy {
public:
	// ���������������������Ϊ�麯��������ᵼ����Դй©
	virtual ~CachePolicy() {};
	// ���溯������Ϊ���麯������Ҫ�ھ���Ļ�������滻������ʵ��
	virtual void put(Key key, Value value) = 0��

	virtual bool get(Key key, Value & value) = 0;
	
	virtual Value get(Key key);
};//CachePolicy
}//namespace CachePolicy

#pragma once
namespace Cache{
template<typename Key,typename Value>
class CachePolicy {
public:
	// 基类的析构函数必须声明为虚函数，否则会导致资源泄漏
	virtual ~CachePolicy() {};
	// 下面函数声明为纯虚函数，需要在具体的缓存策略替换方法中实现
	virtual void put(Key key, Value value) = 0；

	virtual bool get(Key key, Value & value) = 0;
	
	virtual Value get(Key key);
};//CachePolicy
}//namespace CachePolicy

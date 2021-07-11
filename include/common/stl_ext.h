#pragma once

typedef void (*deleter_func_t)(char*& );

// 删除元素
template<typename T>
void deleter(T*& ptr)
{
	if(ptr)
	{
		delete ptr;
		ptr = 0;
	}
}


// 删除数组
template<typename T>
void deleter2(T*& ptr)
{
	if(ptr)
	{
		delete[] ptr;
		ptr = 0;
	}
}

template<typename ObjectType>
class raiiarray
{
	typedef ObjectType* ObjectPtr;

	ObjectPtr _resource;

public:
	raiiarray(const ObjectPtr resource = ObjectPtr() )
	{
		 _resource = resource;
	}

	~raiiarray() {  delete [] _resource; }

	void ptr(ObjectPtr& psrc)
	{
		// 如果为同一对象，无副作用
		if( psrc == _resource)
		{
			return;
		}

		// 释放旧对象，管理新对象,原指针失效
		if( _resource)
		{
			delete [] _resource;
		}
		_resource = psrc;
		psrc = ObjectPtr();
	}
	ObjectPtr ptr(){ return _resource;}
	const ObjectPtr ptr()const{ return _resource;}
};

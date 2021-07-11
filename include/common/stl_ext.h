#pragma once

typedef void (*deleter_func_t)(char*& );

// ɾ��Ԫ��
template<typename T>
void deleter(T*& ptr)
{
	if(ptr)
	{
		delete ptr;
		ptr = 0;
	}
}


// ɾ������
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
		// ���Ϊͬһ�����޸�����
		if( psrc == _resource)
		{
			return;
		}

		// �ͷžɶ��󣬹����¶���,ԭָ��ʧЧ
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

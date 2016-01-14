#pragma once

///////////////////////////////
template <typename T>
class sp
{
public:
	sp (T* p)
		: m_pointee (p)
	{}

	T* operator -> () const
	{
		return m_pointee;
	}

	T& operator * () const
	{
		return *m_pointee;
	}

	operator T* () const
	{
		return m_pointee;
	}

private:
	T* m_pointee = nullptr;
};

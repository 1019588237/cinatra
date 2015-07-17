#pragma once
#include "noncopyable.hpp"
#include "response.hpp"
using namespace cinatra;

#define HAS_MEMBER(member)\
	template<typename T, typename... Args>struct has_member_##member\
{\
private:\
	template<typename U> static auto Check(int) -> decltype(std::declval<U>().member(std::declval<Args>()...), std::true_type()); \
	template<typename U> static std::false_type Check(...); \
public:\
enum{ value = std::is_same<decltype(Check<T>(0)), std::true_type>::value }; \
}; \

HAS_MEMBER(Foo)
HAS_MEMBER(before)
HAS_MEMBER(after)

template<typename Func, typename... Args>
struct AOP : NonCopyable
{
	AOP(Func&& f) : m_func(std::forward<Func>(f))
	{
	}

	template<typename T>
	typename std::enable_if<has_member_before<T, Args...>::value&&has_member_after<T, Args...>::value, bool>::type invoke(Response& res, Args&&... args, T&& aspect)
	{
		bool r = false;
		aspect.before(std::forward<Args>(args)...);
		if (!res.is_complete())
		{
			r = m_func(std::forward<Args>(args)...);
		}

		aspect.after(std::forward<Args>(args)...);
		return r;
	}

	template<typename T>
	typename std::enable_if<has_member_before<T, Args...>::value&&!has_member_after<T, Args...>::value, bool>::type invoke(Response& res, Args&&... args, T&& aspect)
	{
		bool r = false;
		aspect.before(std::forward<Args>(args)...);//�����߼�֮ǰ�������߼�.
		if (!res.is_complete())
			r = m_func(std::forward<Args>(args)...);//�����߼�.

		return r;
	}

	template<typename T>
	typename std::enable_if<!has_member_before<T, Args...>::value&&has_member_after<T, Args...>::value, bool>::type invoke(Response& res, Args&&... args, T&& aspect)
	{
		bool r = m_func(std::forward<Args>(args)...);//�����߼�.
		aspect.after(std::forward<Args>(args)...);//�����߼�֮��������߼�.
		return r;
	}

	template<typename T, typename Self>
	typename std::enable_if<has_member_before<T, Args...>::value&&has_member_after<T, Args...>::value, bool>::type invoke_member(Response& res, Self* self, Args&&... args, T&& aspect)
	{
		bool r = false;
		aspect.before(std::forward<Args>(args)...);//�����߼�֮ǰ�������߼�.
		if (!res.is_complete())
		{
			r = (*self.*m_func)(std::forward<Args>(args)...);//�����߼�.
		}

		aspect.after(std::forward<Args>(args)...);//�����߼�֮��������߼�.
		return r;
	}

	//template<typename Head, typename... Tail>
	//void invoke(bool& result, Args&&... args, Head&&headAspect, Tail&&... tailAspect)
	//{
	//	headAspect.before(std::forward<Args>(args)...);
	//	invoke(result, std::forward<Args>(args)..., std::forward<Tail>(tailAspect)...);
	//	headAspect.after(std::forward<Args>(args)...);
	//}

private:
	Func m_func; //��֯��ĺ���.
};
template<typename T> using identity_t = T;

//AOP�ĸ����������򻯵���.
template<typename... AP, typename... Args, typename Func>
typename std::enable_if<!std::is_member_function_pointer<Func>::value, bool>::type invoke(Response& res, Func&&f, Args&&... args)
{
	AOP<Func, Args...> asp(std::forward<Func>(f));
	return asp.invoke(res, std::forward<Args>(args)..., identity_t<AP>()...);
}

template<typename... AP, typename... Args, typename Func, typename Self>
typename std::enable_if<std::is_member_function_pointer<Func>::value, bool>::type invoke(Response& res, Func&&f, Self* self, Args&&... args)
{
	AOP<Func, Args...> asp(std::forward<Func>(f));
	return asp.invoke_member(res, self, std::forward<Args>(args)..., identity_t<AP>()...);
}
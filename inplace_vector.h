#pragma once

#include <type_traits>


template<typename T>
concept can_pass_in_register
	= std::is_integral_v<T>
	|| std::is_floating_point_v<T>
	|| std::is_enum_v<T>
	|| std::is_reference_v<T>
	|| std::is_pointer_v<T>
	|| (std::is_trivial_v<T> && sizeof(T) <= 8);

template<typename T>
struct argument_type { using in = const T&; };
template<can_pass_in_register T>
struct argument_type<T> { using in = const T; };


template<typename T, unsigned Capacity_, typename SizeType = unsigned char>
class inplace_vector
{
public:
	using value_type = T;
	using size_type = SizeType;
	using pointer = T*;
	using reference = T&;
	using iterator = pointer;
	using const_iterator = const pointer;
	using arg_type = argument_type<value_type>::in;

	static const size_type Capacity = Capacity_;

public:
	inplace_vector() = default;

	iterator begin() { return &m_elements[0]; }
	iterator end() { return &m_elements[m_used]; }
	const_iterator begin() const { return &m_elements[0]; }
	const_iterator end() const { return &m_elements[m_used]; }

	[[nodiscard]] bool empty() const { return (m_used == 0); }
	[[nodiscard]] bool full() const { return (m_used == Capacity); }
	[[nodiscard]] size_type size() const { return m_used; }

	void clear() { m_used = 0; }

	void push_back(arg_type v)
	{
		size_type newIx = m_used;
		m_elements[newIx] = v;
		m_used = newIx + 1;
	}
	void pop_back()
	{
		--m_used;
	}

	arg_type front() const { return m_elements[0]; }
	arg_type back() const { return m_elements[m_used - 1]; }

	reference operator[](size_type i)				{ return m_elements[i]; }
	const reference operator[](size_type i) const	{ return m_elements[i]; }

	void erase_front()
	{
		if (m_used > 1)
		{
			for (size_type from = 1; from < m_used; ++from)
				m_elements[from-1] = m_elements[from];
		}
		--m_used;
	}
	void erase_unsorted(const_iterator it)
	{
		if (m_used > 1)
			std::swap(*it, &m_elements[m_used - 1]);
		--m_used;
	}

	void erase(const_iterator it)
	{
		size_type ix = (it - begin());
		if (ix+1 < m_used)
		{
			for (size_type from = ix+1; from < m_used; ++from)
				m_elements[from-1] = m_elements[from];
		}
		--m_used;
	}

private:
	value_type		m_elements[Capacity];
	size_type		m_used = 0;
};


#pragma once
#define INTERNALSCOPE static
#define APP_CLAMP(value, min, max) (value < min) ? min : (value > max) ? max : value;
/* Win32 */
#include <Windows.h>
/* STD */
#include <assert.h>
#include <string>
#include <stdint.h>
#include <functional>
#include <vector>
#include <array>
#include <fstream>
#include <filesystem>

class Buffer
{
public:
	using Byte_t = uint8_t;
public:
	Buffer()
		:
		m_Data(nullptr),
		m_Size(0)
	{}

	~Buffer()
	{
		if (m_Data)
			delete[] m_Data;

		m_Data = nullptr;
		m_Size = 0;
	}

	/* Performs no allocation */
	void Set(const std::size_t size, const void* data)
	{
		if (m_Data)
			delete[] m_Data;

		m_Size = size;
		m_Data = (void*)data;
	}

	void Allocate(const std::size_t size)
	{
		if (m_Size && m_Size < size)
			delete[] m_Data;

		m_Size = size;
		m_Data = new Byte_t[size];
	}

	void Write(const std::size_t size, const void* data, const std::size_t offset = 0)
	{
		assert(m_Size >= size);
		memcpy(m_Data, data, size);
	}

	void Clear()
	{
		if (m_Data)
			delete[] m_Data;

		m_Size = 0;
	}

	void* Data() const
	{
		return m_Data;
	}

	std::size_t Size() const
	{
		return m_Size;
	}
private:
	void* m_Data;
	std::size_t m_Size;
};
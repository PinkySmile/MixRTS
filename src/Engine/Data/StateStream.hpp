//
// Created by PinkySmile on 13/11/2026.
//

#ifndef MIXRTS_STATESTREAM_HPP
#define MIXRTS_STATESTREAM_HPP


#include <bit>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <type_traits>
#include "Resources/Assert.hpp"
#include "Data/Vector.hpp"

//! Append-only little-endian writer. The buffer is owned by the caller so its capacity can be reused.
class StateWriter {
private:
	std::vector<uint8_t> &_buf;

	void _append(const void *data, size_t size)
	{
		size_t at = this->_buf.size();

		this->_buf.resize(at + size);
		if (size)
			memcpy(this->_buf.data() + at, data, size);
	}

public:
	explicit StateWriter(std::vector<uint8_t> &buf) : _buf(buf) {}

	template<typename T>
	void pod(T value)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
		static_assert(std::endian::native == std::endian::little);
		this->_append(&value, sizeof(T));
	}

	void u8(uint8_t value) { this->pod(value); }
	void u16(uint16_t value) { this->pod(value); }
	void u32(uint32_t value) { this->pod(value); }
	void u64(uint64_t value) { this->pod(value); }
	void i32(int32_t value) { this->pod(value); }
	void f32(float value) { this->pod(value); }
	void f64(double value) { this->pod(value); }
	void size(size_t value) { assert_exp(value <= UINT32_MAX); this->pod(static_cast<uint32_t>(value)); }
	void boolean(bool value) { this->u8(value ? 1 : 0); }

	void string(const std::string &value)
	{
		this->size(value.size());
		this->_append(value.data(), value.size());
	}

	void bytes(const void *data, size_t size) { this->_append(data, size); }

	//! Count, then the raw bytes. T must be trivially copyable and already little-endian.
	template<typename T>
	void span(const std::vector<T> &values)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		static_assert(std::endian::native == std::endian::little);
		this->size(values.size());
		this->_append(values.data(), values.size() * sizeof(T));
	}

	template<typename T>
	void vec2(const Vector2<T> &value)
	{
		this->pod(value.x);
		this->pod(value.y);
	}
};

//! Bounds-checked reader over a snapshot blob.
class StateReader {
private:
	const uint8_t *_data;
	size_t _size;
	size_t _pos = 0;

	void _need(size_t size) const
	{
		assert_msg(this->_pos + size <= this->_size, "Truncated simulation state");
	}

public:
	StateReader(const uint8_t *data, size_t size) : _data(data), _size(size) {}
	explicit StateReader(const std::vector<uint8_t> &data) : StateReader(data.data(), data.size()) {}

	[[nodiscard]] size_t remaining() const { return this->_size - this->_pos; }

	template<typename T>
	T pod()
	{
		static_assert(std::is_trivially_copyable_v<T>);
		static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
		static_assert(std::endian::native == std::endian::little);

		T value;

		this->_need(sizeof(T));
		memcpy(&value, this->_data + this->_pos, sizeof(T));
		this->_pos += sizeof(T);
		return value;
	}

	uint8_t u8() { return this->pod<uint8_t>(); }
	uint16_t u16() { return this->pod<uint16_t>(); }
	uint32_t u32() { return this->pod<uint32_t>(); }
	uint64_t u64() { return this->pod<uint64_t>(); }
	int32_t i32() { return this->pod<int32_t>(); }
	size_t size() { return this->pod<uint32_t>(); }
	float f32() { return this->pod<float>(); }
	double f64() { return this->pod<double>(); }
	bool boolean() { return this->u8() != 0; }

	std::string string()
	{
		size_t length = this->size();
		std::string value;

		assert_msg(length <= this->remaining(), "Truncated simulation state");
		value.resize(static_cast<size_t>(length));
		if (length) {
			memcpy(value.data(), this->_data + this->_pos, static_cast<size_t>(length));
			this->_pos += static_cast<size_t>(length);
		}
		return value;
	}

	void bytes(void *data, size_t size)
	{
		this->_need(size);
		if (size)
			memcpy(data, this->_data + this->_pos, size);
		this->_pos += size;
	}

	template<typename T>
	void span(std::vector<T> &values)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		static_assert(std::endian::native == std::endian::little);

		size_t count = this->size();

		assert_msg(count <= this->remaining() / sizeof(T), "Truncated simulation state");
		values.resize(static_cast<size_t>(count));
		this->bytes(values.data(), values.size() * sizeof(T));
	}

	template<typename T>
	Vector2<T> vec2()
	{
		Vector2<T> value;

		value.x = this->pod<T>();
		value.y = this->pod<T>();
		return value;
	}
};


#endif //MIXRTS_STATESTREAM_HPP

//
// Created by anonymus-raccoon on 1/15/20.
//

#ifndef MIXRTS_VECTOR_HPP
#define MIXRTS_VECTOR_HPP


#include <ostream>
#include <cmath>
#ifndef NO_TGUI
#include <TGUI/Vector2.hpp>
#endif
#include <SFML/System/Vector2.hpp>

template<typename T>
class Vector2 {
public:
	T x;
	T y;

	Vector2() noexcept
		: x(0), y(0) {}

	Vector2(T x, T y) noexcept
		: x(x), y(y) {}

	template <typename T2>
	Vector2(Vector2<T2> v) noexcept
		: x(v.x), y(v.y) {}

	Vector2(sf::Vector2<T> v) noexcept
		: x(v.x), y(v.y) {}

	template <typename T2>
	[[nodiscard]] operator sf::Vector2<T2>() const noexcept
	{
		return sf::Vector2<T2>(this->x, this->y);
	}

#ifndef NO_TGUI
	Vector2(tgui::Vector2<T> v) noexcept
		: x(v.x), y(v.y) {}

	template <typename T2>
	[[nodiscard]] operator tgui::Vector2<T2>() const noexcept
	{
		return tgui::Vector2<T2>(this->x, this->y);
	}
#endif

	template<typename T2>
	[[nodiscard]] Vector2<T2> to() const noexcept
	{
		return {
			static_cast<T2>(this->x),
			static_cast<T2>(this->y)
		};
	}

	template <typename T2>
	[[nodiscard]] bool operator==(const Vector2<T2> &other) const
	{
		return other.x == this->x && this->y == other.y;
	}

	template <typename T2>
	[[nodiscard]] bool operator!=(const Vector2<T2> &other) const
	{
		return other.x != this->x || this->y != other.y;
	}

	template <typename T2>
	[[nodiscard]] bool operator<(const Vector2<T2> &other) const
	{
		if (this->x != other.x)
			return this->x < other.x;
		return this->y < other.y;
	}

	template<typename T2>
	Vector2 &operator+=(const Vector2<T2> &vec) noexcept
	{
		this->x += vec.x;
		this->y += vec.y;
		return *this;
	}

	template<typename T2>
	[[nodiscard]] auto operator+(const Vector2<T2> &vec) const noexcept
	{
		return Vector2<decltype(this->x + vec.x)>(this->x + vec.x, this->y + vec.y);
	}

	template<typename T2>
	Vector2 &operator-=(const Vector2<T2> &vec) noexcept
	{
		this->x -= vec.x;
		this->y -= vec.y;
		return *this;
	}

	template<typename T2>
	[[nodiscard]] auto operator-(const Vector2<T2> &vec) const noexcept
	{
		return Vector2<decltype(this->x - vec.x)>(this->x - vec.x, this->y - vec.y);
	}

	template<typename T2>
	Vector2 &operator*=(T2 d) noexcept
	{
		this->x *= d;
		this->y *= d;
		return *this;
	}

	template<typename T2>
	[[nodiscard]] auto operator*(const Vector2<T2> &b) const noexcept
	{
		return this->x * b.x + this->y * b.y;
	}

	template<typename T2>
	[[nodiscard]] auto operator*(T2 d) const noexcept
	{
		return Vector2<decltype(this->x * d)>(this->x * d, this->y * d);
	}

	template<typename T2>
	Vector2 &operator/=(Vector2<T2> &b) noexcept
	{
		this->x /= b.x;
		this->y /= b.y;
		return *this;
	}

	template<typename T2>
	[[nodiscard]] auto operator/(Vector2<T2> &b) const noexcept
	{
		return Vector2<decltype(this->x / b)>(this->x / b.x, this->y / b.y);
	}

	template<typename T2>
	Vector2 &operator/=(T2 b) noexcept
	{
		this->x /= b;
		this->y /= b;
		return *this;
	}

	template<typename T2>
	[[nodiscard]] auto operator/(T2 b) const noexcept
	{
		return Vector2<decltype(this->x / b)>(this->x / b, this->y / b);
	}

	Vector2 operator-() const noexcept
	{
		return Vector2(-this->x, -this->y);
	}

	template<typename T2>
	[[nodiscard]] double distance(const Vector2<T2> &o) const noexcept
	{
		return (*this - o).magnitude();
	}

	[[nodiscard]] double magnitude() const noexcept
	{
		return std::sqrt(this->x * this->x + this->y * this->y);
	}

	template<typename T2>
	[[nodiscard]] double distance2(const Vector2<T2> &o) const noexcept
	{
		return (*this - o).magnitude2();
	}

	[[nodiscard]] double magnitude2() const noexcept
	{
		return this->x * this->x + this->y * this->y;
	}

	[[nodiscard]] double angle() const noexcept
	{
		return std::atan2(this->y, this->x);
	}

	[[nodiscard]] double angle(const Vector2 &o) const noexcept
	{
		return std::atan2(o.y - this->y, o.x - this->x);
	}

	[[nodiscard]] Vector2<float> normal(const Vector2 &p2)
	{
		return Vector2<float>{this->y - p2.y, p2.x - this->x}.normalized();
	}

	Vector2 normalize() noexcept
	{
		double mag = this->magnitude();

		this->x /= mag;
		this->y /= mag;
		return *this;
	}

	[[nodiscard]] Vector2 normalized() const noexcept
	{
		T mag = this->magnitude();

		return Vector2(this->x / mag, this->y / mag);
	}

	void project(const Vector2 &point) noexcept
	{
		*this = this->projection(point);
	}

	[[nodiscard]] Vector2<float> projection(const Vector2 &point) const noexcept
	{
		return point * this / std::pow(this->magnitude(), 2) * *this;
	}

	void rotate(float angle, const Vector2 &center) noexcept
	{
		if (angle == 0.f)
			return;

		float c = cos(angle);
		float s = sin(angle);
		auto newX = c * (static_cast<float>(this->x) - center.x) - s * (static_cast<float>(this->y) - center.y) + center.x;

		this->y = s * (static_cast<float>(this->x) - center.x) + c * (static_cast<float>(this->y) - center.y) + center.y;
		this->x = newX;
	}

	[[nodiscard]] Vector2<float> rotation(float angle, const Vector2 &center) const noexcept
	{
		Vector2<float> result = *this;

		result.rotate(angle, center);
		return result;
	}

	[[nodiscard]] Vector2 abs() noexcept
	{
		return {
			std::abs(this->x),
			std::abs(this->y)
		};
	}
};

typedef Vector2<float>              Vector2f;
typedef Vector2<int>                Vector2i;
typedef Vector2<unsigned>           Vector2u;
typedef Vector2<long>               Vector2l;
typedef Vector2<unsigned long>      Vector2ul;
typedef Vector2<long long>          Vector2ll;
typedef Vector2<unsigned long long> Vector2ull;
typedef Vector2<double>             Vector2d;
typedef Vector2<bool>               Vector2b;

template<typename T>
std::ostream &operator<<(std::ostream &s, const Vector2<T> &v)
{
	s << v.x << " " << v.y;
	return s;
}

template<typename T>
std::istream &operator>>(std::istream &s, Vector2<T> &v)
{
	s >> v.x >> v.y;
	return s;
}


#include <sol/sol.hpp>
// --- Generic Lua conversion handlers for Vector2<T> ---
// These are picked up by sol2 through argument-dependent lookup (ADL), since
// Vector2<T> lives in the global namespace. Vector2<T> is accepted from Lua
// either as a registered Vector2<T> usertype, or as a plain table of the
// form { x = ..., y = ... }, and is always pushed back as the latter.

template <typename T, typename Handler>
bool sol_lua_check(sol::types<Vector2<T>>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	// Accept if registered as a Vector2<T> usertype
	if (sol::stack::check_usertype<Vector2<T>>(L, index)) {
		tracking.use(1);
		return true;
	}

	// Accept if it's a Lua table containing 'x' and 'y' convertible to T
	if (lua_type(L, index) == LUA_TTABLE) {
		sol::stack_table table(L, index);
		sol::object x = table["x"];
		sol::object y = table["y"];

		if (x.is<T>() && y.is<T>()) {
			tracking.use(1);
			return true;
		}
	}

	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {x, y}");
	return false;
}

template <typename T>
Vector2<T> sol_lua_get(sol::types<Vector2<T>>, lua_State *L, int index, sol::stack::record &tracking)
{
	// If passed as a registered Vector2<T> usertype
	if (sol::stack::check_usertype<Vector2<T>>(L, index))
		return sol::stack::get_usertype<Vector2<T>>(L, index, tracking);

	// Otherwise treat it as a plain table { x = ..., y = ... }
	sol::stack_table table(L, index);
	T x = table.get<T>("x");
	T y = table.get<T>("y");

	tracking.use(1);
	return Vector2<T>(x, y);
}

template <typename T>
int sol_lua_push(lua_State *L, const Vector2<T> &vec)
{
	lua_createtable(L, 0, 2);
	sol::stack::push(L, vec.x);
	lua_setfield(L, -2, "x");
	sol::stack::push(L, vec.y);
	lua_setfield(L, -2, "y");
	return 1;
}


#endif //MIXRTS_VECTOR_HPP

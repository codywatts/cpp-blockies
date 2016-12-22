#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

/// Use "String" as an alias for std::string.
using String = std::string;

/// The Array<T> class wraps std::vector<T> and adds a few functions which are
/// defined on arrays in JavaScript.
template<class T>
class Array : public std::vector<T>
{
	public:
		using std::vector<T>::vector;

		/// Implementation of JavaScript's Array.prototype.slice
		Array slice(size_type startIndex, size_type count) const
		{
			Array::const_iterator begin = (cbegin() + startIndex);
			return Array(begin, begin + count);
		}

		/// Implementation of JavaScript's Array.prototype.reverse
		void reverse()
		{
			std::reverse(begin(), end());
		}

		/// Implementation of JavaScript's Array.prototype.push
		void push(const T& value)
		{
			push_back(value);
		}

		/// Implementation of JavaScript's Array.prototype.concat
		Array concat(const Array& other) const
		{
			Array concatenatedArray = *this;
			concatenatedArray.insert(end(), other.begin(), other.end());
			return concatenatedArray;
		}
};

/// The Number struct wraps a "double" in an attempt to emulate how numbers are
/// treated in JavaScript (especially with regards to bitwise operations.)
struct Number
{
	public:
		constexpr Number(double value) : m_value(value) {}
		constexpr Number() : Number(0.0) {}

		/// User-defined conversion to double, exposing the underlying value.
		constexpr operator double() const { return m_value; }

		/// User-defined conversion to string.
		operator String() const { return std::to_string(m_value); }

		/// Bitwise operators
		constexpr Number operator<<(int n) const
		{
			return Number(static_cast<int32_t>(static_cast<int64_t>(m_value) & 0xFFFFFFFF) << n);
		}
		constexpr Number operator>>(int n) const
		{
			return Number(static_cast<int32_t>(static_cast<int64_t>(m_value) & 0xFFFFFFFF) >> n);
		}
		constexpr Number operator^(const Number& rhs) const
		{
			return static_cast<Number>(static_cast<int32_t>(static_cast<int64_t>(m_value) & 0xFFFFFFFF) ^ static_cast<int32_t>(static_cast<int64_t>(rhs.m_value) & 0xFFFFFFFF));
		}

		/// Post-increment operator
		constexpr Number operator++(int unused) const
		{
			return Number(m_value + 1.0);
		}

		// String concatenation operator
		String operator+(const char* rhs) const { return static_cast<String>(*this) + rhs; }

		// Arithmetic operations
		template <typename T> constexpr Number operator+(const T& rhs) const { return m_value + rhs; }
		template <typename T> constexpr Number operator-(const T& rhs) const { return m_value - rhs; }
		template <typename T> constexpr Number operator*(const T& rhs) const { return m_value * rhs; }
		template <typename T> constexpr Number operator/(const T& rhs) const { return m_value / rhs; }

		// Comparators
		template <typename T> constexpr bool operator<(const T& rhs) const { return (m_value < rhs); }
		template <typename T> constexpr bool operator==(const T& rhs) const { return (m_value == rhs); }

	private:
		double m_value { 0.0 };
};

/// This function replaces the >>> operator, which does not exist in C++
constexpr Number unsignedShiftRight(const Number& number, int n)
{
	return static_cast<uint32_t>(number >> n);
}

/// This function concatenates the string-representation of a number to a
/// string, as in JavaScript.
String operator+(const String& lhs, const Number& rhs)
{
	return lhs + static_cast<String>(rhs);
}

/// This is an implementation of the Xorshift pseudorandom number generator.
class PseudorandomNumberGenerator
{
	public:
		void seed(const String& seed)
		{
			for (auto i = 0; i < 4; i++)
			{
				m_seed[i] = 0;
			}
			for (auto i = 0; i < seed.size(); i++)
			{
				m_seed[i % 4] = ((m_seed[i % 4] << 5) - m_seed[i % 4]) + seed[i];
			}
		}

		Number generate()
		{
			// Based on Java's String.hashCode(), expanded to 4 32bit values.
			auto t = m_seed[0] ^ (m_seed[0] << 11);

			m_seed[0] = m_seed[1];
			m_seed[1] = m_seed[2];
			m_seed[2] = m_seed[3];
			m_seed[3] = (m_seed[3] ^ (m_seed[3] >> 19) ^ t ^ (t >> 8));

			return unsignedShiftRight(m_seed[3], 0) / unsignedShiftRight(1 << 31, 0);
		};

	private:
		Number m_seed[4] { 0, 0, 0, 0 }; // Xorshift: [x, y, z, w] 32 bit values.
};

int main()
{
	PseudorandomNumberGenerator randomNumberGenerator {};

	auto rand = [&]()
	{
		return randomNumberGenerator.generate();
	};

	auto createColor = [&]()
	{
		//saturation is the whole color spectrum
		Number h = floor(rand() * 360);
		//saturation goes from 40 to 100, it avoids greyish colors
		auto s = ((rand() * 60) + 40) + "%";
		//lightness can be anything from 0 to 100, but probabilities are a bell curve around 50%
		auto l = ((rand() + rand() + rand() + rand()) * 25) + "%";

		auto color = "hsl(" + h + "," + s + "," + l + ")";
		return color;
	};

	auto createImageData = [&](const auto& size)
	{
		auto width = size; // Only support square icons for now
		auto height = size;

		auto dataWidth = ceil(width / 2);
		auto mirrorWidth = width - dataWidth;

		auto data = {};
		for (auto y = 0; y < height; y++)
		{
			auto row = {};
			for (auto x = 0; x < dataWidth; x++)
			{
				// this makes foreground and background color to have a 43% (1/2.3) probability
				// spot color has 13% chance
				row[x] = floor(rand() * 2.3);
			}
			auto r = row.slice(0, mirrorWidth);
			r.reverse();
			row = row.concat(r);

			for (auto i = 0; i < row.size(); i++)
			{
				data.push(row[i]);
			}
		}

		return data;
	};

	auto createCanvas = [&](const auto& imageData, const auto& color, const auto& scale, const auto& bgcolor, const auto& spotcolor)
	{
		auto c = document.createElement("canvas");
		auto width = sqrt(imageData.size());
		c.width = c.height = width * scale;

		auto cc = c.getContext("2d");
		cc.fillStyle = bgcolor;
		cc.fillRect(0, 0, c.width, c.height);
		cc.fillStyle = color;

		for (auto i = 0; i < imageData.size(); i++)
		{
			auto row = floor(i / width);
			auto col = i % width;
			// if data is 2, choose spot color, if 1 choose foreground
			cc.fillStyle = (imageData[i] == 1) ? color : spotcolor;

			// if data is 0, leave the background
			if (imageData[i])
			{
				cc.fillRect(col * scale, row * scale, scale, scale);
			}
		}

		return c;
	};

	auto createIcon = [&](const auto& opts)
	{
		opts = opts || {};
		auto size = opts.size || 8;
		auto scale = opts.scale || 4;
		auto seed = opts.seed || floor((random() * pow(10, 16))).toString(16);

		randomNumberGenerator.seed(seed);

		auto color = opts.color || createColor();
		auto bgcolor = opts.bgcolor || createColor();
		auto spotcolor = opts.spotcolor || createColor();
		auto imageData = createImageData(size);
		auto canvas = createCanvas(imageData, color, scale, bgcolor, spotcolor);

		return canvas;
	};

	return 0;
}

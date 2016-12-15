//Open source under MIT license. https://github.com/PrismTower/BigInteger
//Remember to add parameter -std=c++11 if you use g++ as the compiler.

#pragma once
#include <iostream>
#include <cstring>
#include <cmath>
#include <string>
#include <random>

namespace UPmath
{
	typedef unsigned int uint32;
static_assert(sizeof(uint32)==4, "sizeof(uint32) must be 4 bytes!");
	template <class T> const T operator+(const T& lhs, const T& rhs) { return T(lhs) += rhs; }
	template <class T> const T operator-(const T& lhs, const T& rhs) { return T(lhs) -= rhs; }
	template <class T> const T operator>>(const T& lhs, const uint32 shift) { return T(lhs) >>= shift; }
	template <class T> const T operator<<(const T& lhs, const uint32 shift) { return T(lhs) <<= shift; }

	extern std::random_device gRandomDevice;
	
	class BigInteger
	{
	public:
		uint32 capacity = 0, size = 0;//in DWORD (NOT byte)
		bool negativity = false;//*this <= 0 when `negativity` is true and >= 0 when it is false

		BigInteger();
		BigInteger(bool isHexadecimal, const std::string&);//decimal or hexdecimal
		BigInteger(const std::string&);//init as hexadecimal only if the string starts with `0x` or `0X`
		BigInteger(int);
		BigInteger(long long);
		BigInteger(unsigned long long);
		BigInteger(double);
		BigInteger(void* dataPtr, size_t dataSize);//for cryptography
		BigInteger(const BigInteger& source);
		const BigInteger& operator=(BigInteger&& source);
		BigInteger(BigInteger&& source);
		~BigInteger() { if (_exclusivelyMemoryAllocated) delete[] _valPtr; }

		int intValue() const;//truncate
		double doubleValue() const;
		void saveBytesToBuffer(char* dst, size_t writingSize) const;
		const std::string toString() const;
		char* toHexadecimalCharArray() const;
		bool* convertAbsToBinaryArray(size_t* out_digitsSize) const;//from lower digit to higher. eg: 4 -> 001

		void setLowerBitsToRandom(uint32 bitLength);
		bool testBit(uint32 bitPos) const;
		const BigInteger& clearToZero();
		int compareAbsoluteValueTo(const BigInteger&) const;
		int compareTo(const BigInteger&) const;
		bool operator==(const BigInteger& rhs) { return compareTo(rhs) == 0; }
		bool operator!=(const BigInteger& rhs) { return compareTo(rhs) != 0; }
		bool operator<(const BigInteger& rhs) { return compareTo(rhs) < 0; }
		bool operator>(const BigInteger& rhs) { return compareTo(rhs) > 0; }
		bool operator<=(const BigInteger& rhs) { return compareTo(rhs) <= 0; }
		bool operator>=(const BigInteger& rhs) { return compareTo(rhs) >= 0; }
		const BigInteger operator-() { BigInteger t(*this); t.negativity = !t.negativity; return t; }
		const BigInteger& operator+=(const BigInteger& rhs);
		const BigInteger& operator-=(const BigInteger& rhs);
		const BigInteger& operator>>=(const uint32 shift);
		const BigInteger& operator<<=(const uint32 shift);
		friend BigInteger operator*(const BigInteger& lhs, const BigInteger& rhs);
		friend BigInteger operator/(const BigInteger& lhs, const BigInteger& rhs);
		const BigInteger absDivideAndSetThisToRemainder(const BigInteger& rhs);
		const BigInteger& operator%=(const BigInteger&);
		BigInteger operator%(const BigInteger& rhs) { return BigInteger(*this) %= rhs; }
		BigInteger pow(uint32 positive_exponent) const;
		BigInteger modPow(const BigInteger& exponent, const BigInteger& m) const;
		struct _EEAstruct;
		static BigInteger gcd(const BigInteger& a, const BigInteger& b);
		BigInteger modInverse(const BigInteger& m) const;//return BigInteger(0) when the inverse doesnot exist
		bool isPrime(int confidenceFactor = -1) const;
		//In order to make it convenient to implement multi-thread primality test, I decide to set this func `public`.
		bool _isStrongProbablePrime(const BigInteger& a, const BigInteger& thisMinus1) const;

	private:
		bool _exclusivelyMemoryAllocated = true;
		uint32* _valPtr = nullptr;

		void _push(uint32 value);
		void _setSize(uint32 maxPossibleSize);
		static uint32 _calcMinimumCapacity(uint32 size_);
		void _absSubtract(const BigInteger& rhs);//only if abs(rhs) <= abs(*this)
		static BigInteger _absMultiply(const BigInteger& lhs, const BigInteger& rhs);
		BigInteger _fastModPow(bool* binaryArrayOfExponent, size_t digitsSizeOfExponent, const BigInteger& m) const;
		static _EEAstruct _extendedEuclid(BigInteger* a, BigInteger* b);
		static BigInteger* _euclidGcd(BigInteger* a, BigInteger* b);

		inline BigInteger(uint32* val, uint32 size_) : _exclusivelyMemoryAllocated(false), _valPtr(val)
		{ 
			_setSize(size_);
			capacity = _calcMinimumCapacity(size);
		}
		inline BigInteger _higherHalfBits() const
		{
			uint32 halfCapacity = capacity >> 1;
			return BigInteger(_valPtr + halfCapacity, (size < halfCapacity) ? 0 : size - halfCapacity);
		}
		inline BigInteger _lowerHalfBits() const
		{
			uint32 halfCapacity = capacity >> 1;
			return BigInteger(_valPtr, (size < halfCapacity) ? size : halfCapacity);
		}
	};

	struct BigInteger::_EEAstruct { BigInteger *d, *x, *y; };

	inline std::ostream& operator<<(std::ostream& os, const BigInteger& dt)
	{
		os << dt.toString();
		return os;
	}
	
}//namespace UPMath
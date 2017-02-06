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
#define FFT_MULTIPLICATION
#define BITS_OF_DWORD 32
	typedef unsigned int uint32;
	static_assert(sizeof(uint32) == BITS_OF_DWORD / 8, "sizeof(uint32) must be 4 bytes!");

	extern std::random_device gRandomDevice;
	
	class BigInteger
	{
	public:
		uint32 capacity = 0, size = 0;//in DWORD (NOT byte)
		bool negativity = false;//*this <= 0 when `negativity` is true and >= 0 when it is false

		BigInteger();
		BigInteger(bool isHexadecimal, const std::string&);//decimal or hexdecimal
		explicit BigInteger(const std::string&);//init as hexadecimal only if the string starts with `0x` or `0X`
		BigInteger(int);
		BigInteger(long long);
		BigInteger(unsigned long long);
		BigInteger(const void* dataPtr, size_t dataSize);//for cryptography
		BigInteger(const BigInteger& source);
		const BigInteger& operator=(const BigInteger& source);
		BigInteger(BigInteger&& source);
		const BigInteger& operator=(BigInteger&& source);
		~BigInteger() { delete[] _valPtr; }

		int intValue() const;//abs modular 0x80000000
		double doubleValue() const;
		void saveBytesToBuffer(char* dst, size_t writingSize) const;
		const std::string toString() const;
		char* toHexadecimalCharArray() const;
		bool* convertAbsToBinaryArray(size_t* out_digitsSize) const;//from lower digit to higher. eg: 4 -> 001
		bool* convertAbsToBinaryArray(bool* dst, size_t* out_digitsSize) const;//save results to `dst` and get rid of requesting new heap space

		void setLowerBitsToRandom(uint32 bitLength);
		bool testBit(uint32 bitPos) const;
		void clearToZero();
		int compareAbsoluteValueTo(const BigInteger&) const;
		int compareTo(const BigInteger&) const;
		bool operator==(const BigInteger& rhs) const { return compareTo(rhs) == 0; }
		bool operator!=(const BigInteger& rhs) const { return compareTo(rhs) != 0; }
		bool operator<(const BigInteger& rhs) const { return compareTo(rhs) < 0; }
		bool operator>(const BigInteger& rhs) const { return compareTo(rhs) > 0; }
		bool operator<=(const BigInteger& rhs) const { return compareTo(rhs) <= 0; }
		bool operator>=(const BigInteger& rhs) const { return compareTo(rhs) >= 0; }
		BigInteger operator-() const { BigInteger t(*this); t.negativity = !t.negativity; return t; }
		const BigInteger& operator+=(const BigInteger& rhs);
		const BigInteger& operator-=(const BigInteger& rhs);
		const BigInteger& operator>>=(const uint32 shift);
		const BigInteger& operator<<=(const uint32 shift);
		friend BigInteger operator*(const BigInteger& lhs, const BigInteger& rhs);
		friend BigInteger operator/(const BigInteger& lhs, const BigInteger& rhs);
		BigInteger absDivideAndSetThisToRemainder(const BigInteger& rhs);
		const BigInteger& operator%=(const BigInteger&);
		BigInteger operator%(const BigInteger& rhs) const { return BigInteger(*this) %= rhs; }
		BigInteger pow(uint32 positive_exponent) const;
		BigInteger modPow(const BigInteger& exponent, const BigInteger& m) const;
		BigInteger fastModPow(const BigInteger& exponent, const BigInteger& m) const;
		struct _EEAstruct;
		static BigInteger gcd(const BigInteger& a, const BigInteger& b);
		BigInteger modInverse(const BigInteger& m) const;//return BigInteger(0) when the inverse doesnot exist
		bool isPrime(int confidenceFactor = -1) const;
		//In order to make it convenient to implement multi-thread primality test, I decide to set this func `public`.
		bool _isStrongProbablePrime(const BigInteger& a, const BigInteger& thisMinus1) const;//logically wrong when *this == 2

	private:
		uint32* _valPtr = nullptr;
		class _SharedMemBigInteger;
		void _push(uint32 value);
		void _setSize(uint32 maxPossibleSize);
		void _absSubtract(const BigInteger& rhs);//only if abs(rhs) <= abs(*this)
		static BigInteger _absMultiply(const BigInteger& lhs, const BigInteger& rhs);
		void BigInteger::_seperateToHigherAndLowerHalfs(BigInteger* dst) const;
#ifdef FFT_MULTIPLICATION
#define FFT_MAX_N 1024u
#define FFT_WORD_BITLEN 16
	public:
		inline void _FFTgetBeginAndEndOfVal(unsigned short* &begin, unsigned short* &end) const
		{ 
			begin = reinterpret_cast<unsigned short*>(_valPtr);
			end = reinterpret_cast<unsigned short*>(_valPtr + size);
		}
	private:
		static bool _isRootsOfUnityInitialized;
		static void _FFTMultiply(BigInteger& dst, const BigInteger& lhs, const BigInteger& rhs, void* buffer = nullptr);
#endif
		BigInteger _trivialModPow(bool* binaryArrayOfExponent, size_t bitsOfExponent, const BigInteger& m) const;
		BigInteger _montgomeryModPow(bool* binaryArrayOfExponent, size_t bitsOfExponent, const BigInteger& m) const;
		static _EEAstruct _extendedEuclid(BigInteger* a, BigInteger* b);
		static BigInteger* _euclidGcd(BigInteger* a, BigInteger* b);

		static unsigned char _requiredCapacityList[129];
		static uint32 _calcMinimumCapacity(uint32 size_);
		inline static uint32 _getMinimumCapacity(uint32 size_)
		{
			if (size_ < sizeof(_requiredCapacityList)) return _requiredCapacityList[size_];
			return _calcMinimumCapacity(size_);
		}

	};

	struct BigInteger::_EEAstruct { BigInteger *d, *x, *y; };

	class BigInteger::_SharedMemBigInteger : public BigInteger {
	public:
		inline _SharedMemBigInteger(const BigInteger& source) {
			_valPtr = source._valPtr;
			negativity = source.negativity;
			capacity = source.capacity;
			size = source.size;
		}
		_SharedMemBigInteger(uint32 size_, uint32* val);
		~_SharedMemBigInteger() { _valPtr = nullptr; }
	};

	inline BigInteger operator+(const BigInteger& lhs, const BigInteger& rhs) { return BigInteger(lhs) += rhs; }
	inline BigInteger operator-(const BigInteger& lhs, const BigInteger& rhs) { return BigInteger(lhs) -= rhs; }
	template <class T> T operator>>(const T& lhs, const uint32 shift) { return T(lhs) >>= shift; }
	template <class T> T operator<<(const T& lhs, const uint32 shift) { return T(lhs) <<= shift; }

	inline std::ostream& operator<<(std::ostream& os, const BigInteger& dt)
	{
		os << dt.toString();
		return os;
	}
	
}//namespace UPMath
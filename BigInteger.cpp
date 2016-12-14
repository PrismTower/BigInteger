//Open source under MIT license. https://github.com/PrismTower/BigInteger
//This is negative demostration of bad coding style which contains quantity of bugs difficult to find.

#include "BigInteger.h"

namespace UPmath
{
	BigInteger::BigInteger(const BigInteger& source)
	{
		if (this == &source) return;
		negativity = source.negativity;
		size = source.size;
		capacity = _calcMinimumCapacity(size);
		if (nullptr == (_valPtr = new uint32[capacity]))
			throw std::bad_alloc();
		uint32 *_temptr, *_comptr, *_souptr = source._valPtr;
		for (_comptr = (_temptr = _valPtr) + size; _temptr < _comptr; ++_temptr, ++_souptr)
			*_temptr = *_souptr;
		_comptr = _valPtr + capacity;
		while (_temptr < _comptr) *(_temptr++) = 0;
	}

	const BigInteger& BigInteger::operator=(BigInteger&& source)
	{
		_valPtr = source._valPtr;
		_exclusivelyMemoryAllocated = source._exclusivelyMemoryAllocated;
		negativity = source.negativity;
		capacity = source.capacity;
		size = source.size;
		source._valPtr = nullptr;
		return *this;
	}

	BigInteger::BigInteger(BigInteger&& source)
	{
		_valPtr = source._valPtr;
		_exclusivelyMemoryAllocated = source._exclusivelyMemoryAllocated;
		negativity = source.negativity;
		capacity = source.capacity;
		size = source.size;
		source._valPtr = nullptr;
	}

	BigInteger::BigInteger() { }

	BigInteger::BigInteger(double d) :BigInteger(static_cast<long long>(d)) { }

	BigInteger::BigInteger(int num)
	{
		if (num) {
			if (num < 0) { negativity = true; num = -num; }
			(_valPtr = new uint32[capacity = size = 1])[0] = num;
		}
	}

	BigInteger::BigInteger(long long num)
	{
static_assert(sizeof(long long) / sizeof(uint32) == 2, "Compile error in BigInteger(long long)");
		_valPtr = new uint32[capacity = 2];
		if (num < 0) { negativity = true; num = -num; }
		if (num) _valPtr[0] = static_cast<uint32>(num), ++size;
		num >>= 32;
		if (num) _valPtr[1] = static_cast<uint32>(num), ++size;
	}

	BigInteger::BigInteger(unsigned long long num)
	{
		_valPtr = new uint32[capacity = 2];
		if (num) _valPtr[0] = static_cast<uint32>(num), ++size;
		num >>= 32;
		if (num) _valPtr[1] = static_cast<uint32>(num), ++size;
	}

	BigInteger::BigInteger(bool isHexadecimal, const std::string& str)
	{
		int i = 0, strLen = (int)str.length(), t;
		bool neg_ = false;
		if (str[i] == '-') { neg_ = true; ++i; }
		else if (str[i] == '+') ++i;
		if (!isHexadecimal)
		{
			while (i < strLen && (t = (int)str[i] - '0') >= 0 && t < 10) {
				BigInteger tupi = *this <<= 1;
				*this <<= 2; *this += tupi; *this += BigInteger(t);
				++i;
			}
		}
		else
		{
			(_valPtr = new uint32[capacity = 1])[0] = 0;
			if (strLen >= 2 && ( ('\x5F' & str[i + 1]) == 'X')) i += 2;
			while (i < strLen && (t = (int)str[i] - '0') >= 0) {
				if (t > 9) {
					t = (int)(str[i] & '\x5F') - 'A' + 10;
					if ((t < 10) | (t >= 16)) break;
				}
				*this <<= 4;
				this->_valPtr[0] |= t;
				if (size == 0 && t) ++size;
				++i;
			}
			_setSize(capacity);
		}
		negativity = neg_;
	}

	BigInteger::BigInteger(const std::string& str) :
		BigInteger((str.length() >= 2 && str[0] == '0' && (('\x5F' & str[1]) == 'X')), str) { }

	void BigInteger::_push(const uint32 value)
	{
		if (capacity > size)
			_valPtr[size++] = value;
		else {
			uint32 *newptr, *temptr;
			if (nullptr == (newptr = new uint32[capacity <<= 1]))
				throw std::bad_alloc();
			temptr = newptr + size;
			while (newptr < temptr)	*(newptr++) = *(_valPtr++);
			*newptr = value;
			delete[] (_valPtr -= size);
			temptr = (_valPtr = newptr - size++) + capacity;
			while (++newptr < temptr) *newptr = 0;
		}
	}

	uint32 BigInteger::_calcMinimumCapacity(uint32 size_)
	{
		if (size_ == 0) return 0;
		uint32 minCap = 1;
		while (size_ > minCap) minCap <<= 1;
		return minCap;
	}

	void BigInteger::_setSize(uint32 maxPossibleSize)
	{
		if ((size = maxPossibleSize))
			for (uint32* ptr = _valPtr + size; --ptr >= _valPtr; --size)
				if (*ptr) break;
	}



	int BigInteger::compareAbsoluteValueTo(const BigInteger& rhs) const
	{
		if (size != rhs.size)
			return (size > rhs.size) ? 1 : -1;
		for (uint32 i = size; i > 0; ) {
			--i;
			if (_valPtr[i] != rhs._valPtr[i])
				return (_valPtr[i] > rhs._valPtr[i]) ? 1 : -1;
		}
		return 0;
	}

	int BigInteger::compareTo(const BigInteger& rhs) const
	{
		if (negativity != rhs.negativity) {
			if (0 == (size | rhs.size)) return 0;
			return negativity ? -1 : 1;
		}
		int result = compareAbsoluteValueTo(rhs);
		return negativity ? result : -result;
	}

	void BigInteger::_absSubtract(const BigInteger& rhs)
	{
		uint32 temp;
		for (uint32 ti = 0, *ptr = _valPtr; ti < rhs.size; ++ti, ++ptr) {
			temp = *ptr;
			if ((*ptr -= rhs._valPtr[ti]) > temp) {
				uint32 _i = ti;
				while (true) if (_valPtr[++_i]--) break;
			}
		}
		_setSize(size);
	}

	const BigInteger& BigInteger::operator+=(const BigInteger& rhs)
	{
		if (this == &rhs) *this <<= 1;
		else if (negativity == rhs.negativity)
		{
			uint32 commonPiecesCount = (size > rhs.size) ? rhs.size : size;
			uint32 *l_ptr, *r_ptr, *c_ptr;//lhs的数组指针，rhs的数组指针，用于比较地址的指针
			if (commonPiecesCount != rhs.size)
			{//rhs is longer
				if (capacity < rhs.size) {
					uint32* newptr = new uint32[capacity = _calcMinimumCapacity(rhs.size)];
					if (nullptr == newptr) throw std::bad_alloc();
					for (c_ptr = (l_ptr = _valPtr) + size; l_ptr < c_ptr; ++newptr, ++l_ptr)
						 *newptr = *l_ptr;
					delete[] _valPtr;
					_valPtr = (l_ptr = newptr) - size;
					newptr = _valPtr + rhs.size; 
					for (c_ptr = _valPtr + capacity; newptr < c_ptr; ++newptr) *newptr = 0;
				}
				l_ptr = _valPtr + size;
				r_ptr = rhs._valPtr + size;
				c_ptr = rhs._valPtr + (size = rhs.size);
				while (r_ptr < c_ptr) *(l_ptr++) = *(r_ptr++);
			}
			l_ptr = _valPtr;
			r_ptr = rhs._valPtr;
			for (c_ptr = rhs._valPtr + commonPiecesCount; r_ptr < c_ptr; ++l_ptr, ++r_ptr)
			{
				if ((*l_ptr += *r_ptr) < *r_ptr) {
					uint32 _i = l_ptr - _valPtr;
					while (++_i < size) if (++_valPtr[_i]) break;
					if (_i == size) {
						_i = l_ptr - _valPtr;
						_push(1);
						l_ptr = _valPtr + _i;
					}
				}
			}
		}
		else if (compareAbsoluteValueTo(rhs) >= 0)
			this->_absSubtract(rhs);
		else
		{
			BigInteger absGreaterOne(rhs);
			absGreaterOne._absSubtract(*this);
			this->~BigInteger();
			*this = std::move(absGreaterOne);
		}
		return *this;
	}

	const BigInteger& BigInteger::operator-=(const BigInteger& rhs)
	{
		if (this == &rhs) return this->clearToZero();
		BigInteger signModified_rhs(rhs._valPtr, rhs.size);
		signModified_rhs.negativity = !rhs.negativity;
		*this += signModified_rhs;
		return *this;
	}

	BigInteger& BigInteger::clearToZero()
	{
		negativity = false;
		size = capacity = 0;
		if (_exclusivelyMemoryAllocated) {
			delete[] _valPtr;
			_valPtr = nullptr;
		}
		return *this;
	}

	const BigInteger& BigInteger::operator>>=(const uint32 shift)
	{
		uint32 a = shift >> 5;
		if (a >= size) return this->clearToZero();
		else if (a) {
			uint32 i = 0;
			size -= a;
			do {
				_valPtr[i] = _valPtr[i + a];
				++i;
			} while (i < size);
			for (a += size; i < a; ++i) _valPtr[i] = 0;
		}
		a = shift & 31;
		if (a && size)
		{
			uint32 pm1 = size - 1;
			for (uint32 i = 0; i < pm1; ++i)
				_valPtr[i] = (_valPtr[i] >> a) | ((_valPtr[i + 1] & ((1 << a) - 1)) << (32 - a));
			if (0 == (_valPtr[pm1] >>= a)) --size;
		}
		return *this;
	}

	const BigInteger& BigInteger::operator<<=(const uint32 shift)
	{
		if (size)
		{
			uint32 a = shift >> 5, b = shift & 31;
			bool newMemAlloced = false;
			if (b) ++a;
			if (a) {
				uint32 *souptr, *tarptr, *comptr;
				if (capacity < (size += a)) {
					newMemAlloced = true;
					capacity = _calcMinimumCapacity(size);
					if (nullptr == (tarptr = new uint32[capacity]))
						throw std::bad_alloc();
					comptr = tarptr + size;
					tarptr += capacity;
					while (tarptr > comptr) *(--tarptr) = 0;
				}
				else
					tarptr = _valPtr + size;
				souptr = _valPtr + (size - a);
				while (souptr > _valPtr) *(--tarptr) = *(--souptr);
				for (comptr = tarptr - a; tarptr > comptr; ) *(--tarptr) = 0;
				if (newMemAlloced) {
					delete[] _valPtr;
					_valPtr = tarptr;
				}
			}
			if (b) *this >>= (32 - b);
		}
		return *this;
	}


	BigInteger operator*(const BigInteger& lhs, const BigInteger& rhs)
	{//multipication
		BigInteger capacityModified_lhs(lhs._valPtr, lhs.size);
		BigInteger capacityModified_rhs(rhs._valPtr, rhs.size);
		BigInteger ret = BigInteger::_absMultiply(capacityModified_lhs, capacityModified_rhs);
		ret.negativity = (lhs.negativity != rhs.negativity);
		return ret;
	}

	BigInteger BigInteger::_absMultiply(const BigInteger& lhs, const BigInteger& rhs)
	{
		if (!lhs.size || !rhs.size) return BigInteger();
		if (lhs.capacity != rhs.capacity)
		{
			const BigInteger& longer = (lhs.capacity >= rhs.capacity) ? lhs : rhs;
			const BigInteger& shorter = (lhs.capacity >= rhs.capacity) ? rhs : lhs;
			BigInteger ret = _absMultiply(longer._higherHalfBits(), shorter);
			ret <<= (longer.capacity << 4);
			ret += _absMultiply(longer._lowerHalfBits(), shorter);
			return ret;
		}
		if (lhs.capacity > 1)
		{
			BigInteger higher_part = _absMultiply(lhs._higherHalfBits(), rhs._higherHalfBits());
			BigInteger lower_part = _absMultiply(lhs._lowerHalfBits(), rhs._lowerHalfBits());
			BigInteger mid_part = higher_part + lower_part;
			mid_part += ((lhs._lowerHalfBits() - lhs._higherHalfBits()) * (rhs._higherHalfBits() - rhs._lowerHalfBits()));
			higher_part <<= (lhs.capacity << 5);
			mid_part <<= (lhs.capacity << 4);
			higher_part += (mid_part += lower_part);
			return higher_part;
		}
		return BigInteger((unsigned long long)*lhs._valPtr * *rhs._valPtr);
	}
	
	const BigInteger BigInteger::absDivideAndSetThisToRemainder(const BigInteger& rhs)
	{//set *this to remainder and return the quotient
		if (this == &rhs) {	this->clearToZero(); return BigInteger(1); }
		int pDiff = this->size - rhs.size;
		if (0 == rhs.size) throw std::logic_error("Divide by zero");
		if (pDiff < 0) return BigInteger();
		int pow = ((pDiff + 1) << 5);
		bool* binaryList = new bool[pow];
		if (nullptr == binaryList) throw std::bad_alloc();
		BigInteger newrhs(rhs); newrhs <<= --pow;
		int leftMostBit = -1;
		do {
			if (newrhs.compareAbsoluteValueTo(*this) > 0)
				binaryList[pow] = false;
			else {
				this->_absSubtract(newrhs);
				binaryList[pow] = true;
				if (leftMostBit < 0) leftMostBit = pow;
			}
			newrhs >>= 1;
		} while (--pow >= 0);
		if (leftMostBit < 0) {
			delete[] binaryList;
			return BigInteger(0);
		}
		BigInteger ret;
		ret.size = 1 + (leftMostBit >> 5);
		ret._valPtr = new uint32[ret.capacity = _calcMinimumCapacity(ret.size)];
		for (uint32 piece = 0; pow < leftMostBit; ++piece)
		{
			ret._valPtr[piece] = 0;
			for (uint32 bi = 0; bi < 32; ++bi)
				ret._valPtr[piece] |= ((uint32)binaryList[++pow] << bi);
		}
		delete[] binaryList;
		return ret;
	}

	BigInteger operator/(const BigInteger& lhs, const BigInteger& rhs)
	{
		BigInteger newlhs(lhs);
		newlhs.negativity = false;
		BigInteger ret = newlhs.absDivideAndSetThisToRemainder(rhs);
		ret.negativity = (lhs.negativity != rhs.negativity);
		return ret;
	}

	const BigInteger& BigInteger::operator%=(const BigInteger& rhs)
	{
		this->absDivideAndSetThisToRemainder(rhs);
		return *this;
	}


	int BigInteger::intValue() const
	{
		if (size == 0) return 0;
		int ret = _valPtr[0];
		return negativity ? -ret : ret;
	}

	double BigInteger::doubleValue() const
	{
		double ret = 0.0;
		for (uint32 i = 0; i < size; ++i) {
			ret *= (double)0x100000000;
			ret += _valPtr[i];
		}
		return negativity ? -ret : ret;
	}

	const std::string BigInteger::toString() const
	{
		if (size == 0) return std::string("0");
		uint32 t = (size * 10) + 2;
		char* buffer = new char[t];
		if (nullptr == buffer) throw std::bad_alloc();
		char* c = buffer + t;
		*(--c) = '\0';
		const BigInteger ten(10);
		BigInteger* newthis = new BigInteger(*this);
		do {
			BigInteger* temptr = new BigInteger(newthis->absDivideAndSetThisToRemainder(ten));
			*(--c) = *(newthis->_valPtr) + '0';
			delete newthis;
			newthis = temptr;
		} while (newthis->size);
		delete newthis;
		if (negativity) *(--c) = '-';
		std::string ret(c);
		delete[] buffer;
		return ret;
	}

	char* BigInteger::toHexadecimalCharArray() const
	{//this part will work correctly only if the sizeof(uint32) is 4
		char* c_str = new char[(size << 3) + 2], *c;
		if (nullptr == (c = c_str)) throw std::bad_alloc();
		if (size)
		{
			if (negativity) *(c++) = '-';
			uint32 *v_ptr;
			for (v_ptr = _valPtr + size; v_ptr > _valPtr; ) {
				uint32 piece = *(--v_ptr), shifting = 32;
				do {
					shifting -= 4;
					uint32 letter = (piece >> shifting) & 0xF;
					*(c++) = (letter > 9) ? ('A' - 10 + letter) : ('0' + letter);
				} while (shifting);
			}
		}
		else *(c++) = '0';
		*c = '\0';
		return c_str;
	}

	bool* BigInteger::convertAbsToBinaryArray(size_t* out_digitsSize) const
	{
		if (!this->size) {
			*out_digitsSize = 0;
			return nullptr;
		}
		size_t digitsSize = this->size * (size_t)32;
		bool* ret = new bool[digitsSize];
		if (nullptr == ret) throw std::bad_alloc();
		for (uint32 i = 0, pieceIndex = 0; pieceIndex < this->size; ++pieceIndex, i += 32)
		{
			uint32 piece = this->_valPtr[pieceIndex];
			ret[i + 0] = (piece & (1 << 0)) != 0; ret[i + 1] = (piece & (1 << 1)) != 0;	ret[i + 2] = (piece & (1 << 2)) != 0; ret[i + 3] = (piece & (1 << 3)) != 0;
			ret[i + 4] = (piece & (1 << 4)) != 0; ret[i + 5] = (piece & (1 << 5)) != 0;	ret[i + 6] = (piece & (1 << 6)) != 0; ret[i + 7] = (piece & (1 << 7)) != 0;
			ret[i + 8] = (piece & (1 << 8)) != 0; ret[i + 9] = (piece & (1 << 9)) != 0; ret[i + 10] = (piece & (1 << 10)) != 0; ret[i + 11] = (piece & (1 << 11)) != 0;
			ret[i + 12] = (piece & (1 << 12)) != 0;	ret[i + 13] = (piece & (1 << 13)) != 0;	ret[i + 14] = (piece & (1 << 14)) != 0;	ret[i + 15] = (piece & (1 << 15)) != 0;
			ret[i + 16] = (piece & (1 << 16)) != 0;	ret[i + 17] = (piece & (1 << 17)) != 0;	ret[i + 18] = (piece & (1 << 18)) != 0;	ret[i + 19] = (piece & (1 << 19)) != 0;
			ret[i + 20] = (piece & (1 << 20)) != 0;	ret[i + 21] = (piece & (1 << 21)) != 0;	ret[i + 22] = (piece & (1 << 22)) != 0;	ret[i + 23] = (piece & (1 << 23)) != 0;
			ret[i + 24] = (piece & (1 << 24)) != 0;	ret[i + 25] = (piece & (1 << 25)) != 0;	ret[i + 26] = (piece & (1 << 26)) != 0;	ret[i + 27] = (piece & (1 << 27)) != 0;
			ret[i + 28] = (piece & (1 << 28)) != 0;	ret[i + 29] = (piece & (1 << 29)) != 0;	ret[i + 30] = (piece & (1 << 30)) != 0;	ret[i + 31] = (piece & (1 << 31)) != 0;
		}
		do { --digitsSize; } while (!ret[digitsSize]) ;
		*out_digitsSize = ++digitsSize;
		return ret;
	}


	BigInteger BigInteger::pow(uint32 positive_exponent) const
	{
		if (0 == positive_exponent) return BigInteger(1);
		BigInteger t = this->pow(positive_exponent >> 1);
		BigInteger tSquared = _absMultiply(t, t);//如果这样写会内存泄漏 t = _absMultiply(t, t); 下面也有类似的问题
		if (positive_exponent & 1) {
			BigInteger ret = _absMultiply(*this, tSquared);
			ret.negativity = this->negativity;
			return ret;
		}
		return tSquared;
	}

	BigInteger BigInteger::modPow(const BigInteger& exponent, const BigInteger& m) const
	{
		if (0 == exponent.size) return BigInteger(1);
		if ((0 == m.size) | m.negativity) throw std::logic_error("modular for `modPow` function must be positive.");
		size_t digitsSizeOfExponent;
		bool* binaryArrayOfExponent = exponent.convertAbsToBinaryArray(&digitsSizeOfExponent);
		BigInteger base = exponent.negativity ? this->modInverse(m) : *this;
		if (base.negativity) {
			base.absDivideAndSetThisToRemainder(m);
			base += m;
		}
		BigInteger result = base._fastModPow(binaryArrayOfExponent, digitsSizeOfExponent, m);
		delete[] binaryArrayOfExponent;
		return result;
	}

	BigInteger BigInteger::_fastModPow(bool* binaryArrayOfExponent, size_t digitsSizeOfExponent, const BigInteger& m) const
	{
		if (digitsSizeOfExponent == 1) return binaryArrayOfExponent[0] ? BigInteger(*this) : BigInteger(1);
		BigInteger t = this->_fastModPow(binaryArrayOfExponent + 1, digitsSizeOfExponent - 1, m);
		BigInteger tSquared = _absMultiply(t, t);
		tSquared.absDivideAndSetThisToRemainder(m);
		if (binaryArrayOfExponent[0]) {
			BigInteger ret = _absMultiply(*this, tSquared);
			ret.absDivideAndSetThisToRemainder(m);
			return ret;
		}
		return tSquared;
	}

	//return BigInteger(0) when the inverse doesnot exist
	BigInteger BigInteger::modInverse(const BigInteger& m) const
	{
		if ((0 == m.size) | m.negativity) throw std::logic_error("modular for `modInverse` function must be positive.");
		if (gcd(*this, m) != BigInteger(1)) return BigInteger(0);
		BigInteger a_(*this), m_(m);
		_EEAstruct eeaStruct = _extendedEuclid(&a_, &m_);
		BigInteger ddd = *this * *eeaStruct.x + m * *eeaStruct.y;
		BigInteger ret = std::move(*eeaStruct.x);
		delete eeaStruct.d; delete eeaStruct.x; delete eeaStruct.y;
		if (ret.negativity) ret += m;
		return ret;
	}

	BigInteger::_EEAstruct BigInteger::_extendedEuclid(BigInteger* a, BigInteger* b)
	{
		if (b->size == 0) return BigInteger::_EEAstruct{ new BigInteger(*a), new BigInteger(1), new BigInteger(0) };
		bool neg_ = (a->negativity != b->negativity);
		BigInteger quotient = a->absDivideAndSetThisToRemainder(*b);
		quotient.negativity = neg_;
		BigInteger::_EEAstruct ts = _extendedEuclid(b, a);
		*ts.x -= quotient * *ts.y;
		return BigInteger::_EEAstruct{ ts.d, ts.y, ts.x };
	}

	BigInteger BigInteger::gcd(const BigInteger& a, const BigInteger& b)
	{
		BigInteger a_(a), b_(b);
		BigInteger* result = _euclidGcd(&a_, &b_);
		BigInteger ret = std::move(*result);
		ret.negativity = false;
		return ret;
	}

	BigInteger* BigInteger::_euclidGcd(BigInteger* a, BigInteger* b)
	{
		if (b->size == 0) return a;
		a->absDivideAndSetThisToRemainder(*b);
		return _euclidGcd(b, a);
	}

	//Deterministic Robin-Miller Primality Test
	//for integers contain more than 1000 bits this function could be excessively slow and 
	//in that case it is a ! LITTLE BIT ! better to invoke `isProbablePrime` which is actually a Monte-Carlo method
	bool BigInteger::isPrime() const
	{// 2^127 - 1 = 7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
		if (0 == size || negativity) return false;
		if (_valPtr[0] <= 3) return (_valPtr[0] > 1);
		if ((_valPtr[0] & 1) == 0) return false;
		BigInteger thisMinus1(*this); thisMinus1 += BigInteger(-1);
		BigInteger a(1);
		if (size <= 2)
		{
			const char testNumbers[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37 };
			int index = 0;
			do {
				a._valPtr[0] = testNumbers[index];
				if (a.compareAbsoluteValueTo(thisMinus1) >= 0) return true;
				if (!_isStrongProbablePrime(a, thisMinus1))	return false;
			} while (++index < sizeof(testNumbers) / sizeof(char));
		}
		else
		{
			double ln_n = log(size * 32);
			uint32 maxTester = (uint32)(2.0 * ln_n * ln_n);
			uint32 base = 2;
			do {
				a._valPtr[0] = base;
				if (!_isStrongProbablePrime(a, thisMinus1))	return false;
			} while (++base <= maxTester);
		}
		return true;
	}

	bool BigInteger::_isStrongProbablePrime(const BigInteger& a, const BigInteger& thisMinus1) const
	{
		BigInteger bigInteger_1(1);
		if (a.modPow(thisMinus1, *this) != bigInteger_1) return false;
		BigInteger u(thisMinus1);
		do { u >>= 1; } while ((u._valPtr[0] & 1) == 0);
		BigInteger x = a.modPow(u, *this);
		do {
			BigInteger y = x.modPow(x, *this);
			if (y == bigInteger_1 && x != bigInteger_1 && x != thisMinus1) return false;
			x.clearToZero();//不加这一句就内存泄漏！调用移动构造函数之前要怎么析构原对象呢？
			x = std::move(y);
			u <<= 1;
		} while (u < *this);
		if (x != 1) return false;
		return true;
	}

	bool BigInteger::isProbablePrime(int confidenceFactor) const
	{
		if (0 == size || negativity) return false;
		if (_valPtr[0] <= 3) return (_valPtr[0] > 1);
		if ((_valPtr[0] & 1) == 0) return false;
		BigInteger thisMinus1(*this); thisMinus1 += BigInteger(-1);
		BigInteger a(1);
		if (size <= 2) return this->isPrime();
		do {
			uint32 randomNumber;
			if (RAND_MAX <= 0x7fff)
				randomNumber = (rand() << 16) | rand();
			else
				randomNumber = rand();
			a._valPtr[0] = randomNumber;
			if (!_isStrongProbablePrime(a, thisMinus1))	return false;
		} while (--confidenceFactor > 0);
		return true;
	}

	//for cryptography
	BigInteger::BigInteger(void* dataPtr, size_t dataSize)
	{
		size = (dataSize + sizeof(uint32) - 1) / sizeof(uint32);
		if (nullptr == (_valPtr = new uint32[capacity = _calcMinimumCapacity(size)]))
			throw std::bad_alloc();
		memcpy(_valPtr, dataPtr, dataSize);
		memset(_valPtr + dataSize, 0, capacity * sizeof(uint32) - dataSize);
		_setSize(size);
	}

	void BigInteger::saveBytesToBuffer(char* dst, size_t writingSize) const
	{
		size_t integerSizeInByte = size * sizeof(uint32);
		if (integerSizeInByte >= writingSize) memcpy(dst, _valPtr, writingSize);
		else {
			memcpy(dst, _valPtr, integerSizeInByte);
			memset(dst + integerSizeInByte, 0, writingSize - integerSizeInByte);
		}
	}

}//namespace UPMath
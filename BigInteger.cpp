//Open source under MIT license. https://github.com/PrismTower/BigInteger

#include "BigInteger.h"

namespace UPmath
{
	std::random_device gRandomDevice;

	BigInteger::BigInteger() { }

	BigInteger::BigInteger(int num)
	{
		if (num) {
			if (num < 0) { negativity = true; num = -num; }
			(_valPtr = new uint32[capacity = size = 1])[0] = num;
		}
	}

	BigInteger::BigInteger(long long num) : BigInteger((unsigned long long)(num < 0 ? -num : num)) { negativity = (num < 0); }
	BigInteger::BigInteger(unsigned long long num)
	{
		static_assert(sizeof(long long) / sizeof(uint32) == 2, "Compile error in BigInteger(long long)");
		_valPtr = new uint32[capacity = 2];
		if (num) _valPtr[0] = static_cast<uint32>(num), ++size;
		num >>= BITS_OF_DWORD;
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
		}
		negativity = neg_;
	}

	BigInteger::BigInteger(const BigInteger& source)
	{
		if (this == &source) return;
		negativity = source.negativity;
		size = source.size;
		capacity = _getMinimumCapacity(size);
		_valPtr = new uint32[capacity];
		uint32 *_temptr = _valPtr, *_srcptr = source._valPtr;
		for (uint32* _comptr = _temptr + size; _temptr < _comptr; ++_temptr, ++_srcptr)	*_temptr = *_srcptr;
	}

	const BigInteger& BigInteger::operator=(const BigInteger& source)
	{
		if (this == &source) return *this;
		if (source.size > this->capacity) {
			delete[] this->_valPtr;
			this->_valPtr = new uint32[this->capacity = _getMinimumCapacity(source.size)];
		}
		negativity = source.negativity;
		size = source.size;
		uint32 *_temptr = _valPtr, *_srcptr = source._valPtr;
		for (uint32* _comptr = _temptr + size; _temptr < _comptr; ++_temptr, ++_srcptr)	*_temptr = *_srcptr;
		return *this;
	}

	const BigInteger& BigInteger::operator=(BigInteger&& source)
	{
		delete[] _valPtr;
		_valPtr = source._valPtr;
		negativity = source.negativity;
		capacity = source.capacity;
		size = source.size;
		source._valPtr = nullptr;
		source.capacity = source.size = 0;
		return *this;
	}

	BigInteger::BigInteger(BigInteger&& source)
	{
		_valPtr = source._valPtr;
		negativity = source.negativity;
		capacity = source.capacity;
		size = source.size;
		source._valPtr = nullptr;
		source.capacity = source.size = 0;
	}

	BigInteger::BigInteger(const std::string& str) :
		BigInteger((str.length() >= 2 && str[0] == '0' && (('\x5F' & str[1]) == 'X')), str) { }

	//things will go wrong if you _push 0 without _pushing a positive number in the end
	void BigInteger::_push(const uint32 value)
	{
		if (capacity > size)
			_valPtr[size++] = value;
		else {
			uint32* newptr = new uint32[capacity <<= 1];
			uint32* temptr = newptr + size;
			while (newptr < temptr)	*(newptr++) = *(_valPtr++);
			*newptr = value;
			delete[] (_valPtr -= size);
			_valPtr = newptr - size++;
		}
	}

	void BigInteger::_setSize(uint32 maxPossibleSize)
	{
		if ((size = maxPossibleSize))
			for (uint32* ptr = _valPtr + size; --ptr >= _valPtr; --size)
				if (*ptr) break;
	}

	unsigned char BigInteger::_requiredCapacityList[129] = 	{ 
		0, 1, 2, 4, 4, 8, 8, 8, 8, 16, 16, 16, 16, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 
		128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 };

	uint32 BigInteger::_calcMinimumCapacity(uint32 size_)
	{
		uint32 minCap = (size_ < sizeof(_requiredCapacityList)) ? 1 : 256;
		while (size_ > minCap) minCap <<= 1;
		return minCap;
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
		return negativity ? -result : result;
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
			uint32 *l_ptr, *r_ptr, *c_ptr;
			if (commonPiecesCount != rhs.size)
			{//rhs is longer
				if (capacity < rhs.size) {
					uint32* newptr = new uint32[capacity = _getMinimumCapacity(rhs.size)];
					for (c_ptr = (l_ptr = _valPtr) + size; l_ptr < c_ptr; ++newptr, ++l_ptr) *newptr = *l_ptr;
					delete[] _valPtr;
					_valPtr = newptr - size;
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
			*this = std::move(absGreaterOne);
		}
		return *this;
	}

	const BigInteger& BigInteger::operator-=(const BigInteger& rhs)
	{
		if (this == &rhs) { this->size = 0; return *this; }
		_SharedMemBigInteger signModified_rhs(rhs);
		signModified_rhs.negativity = !rhs.negativity;
		*this += signModified_rhs;
		return *this;
	}

	const BigInteger& BigInteger::operator>>=(const uint32 shift)
	{
		uint32 a = shift >> 5;
		if (a >= size) { this->size = 0; return *this; }
		else if (a) {
			uint32 i = 0;
			size -= a;
			do {
				_valPtr[i] = _valPtr[i + a];
				++i;
			} while (i < size);
			for (a += size; i < a; ++i) _valPtr[i] = 0;
		}
		a = shift & (BITS_OF_DWORD - 1);
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
			uint32 a = shift >> 5, b = shift & (BITS_OF_DWORD - 1);
			bool newMemAlloced = false;
			if (b) ++a;
			if (a) {
				uint32 *souptr, *tarptr, *comptr;
				if (capacity < (size += a)) {
					newMemAlloced = true;
					capacity = _getMinimumCapacity(size);
					tarptr = new uint32[capacity];
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
		BigInteger ret = BigInteger::_absMultiply(lhs, rhs);
		ret.negativity = (lhs.negativity != rhs.negativity);
		return ret;
	}

	void BigInteger::_seperateToHigherAndLowerHalfs(BigInteger* dst) const
	{
		uint32 halfCapacity = capacity >> 1;
		dst[0]._valPtr = _valPtr + halfCapacity;
		dst[0].size = (size < halfCapacity) ? 0 : size - halfCapacity;
		dst[0].capacity = _getMinimumCapacity(dst[0].size);
		dst[1]._valPtr = _valPtr;
		dst[1]._setSize((size < halfCapacity) ? size : halfCapacity);
		dst[1].capacity = _getMinimumCapacity(dst[1].size);
	}

	BigInteger BigInteger::_absMultiply(const BigInteger& lhs, const BigInteger& rhs)
	{
		if ((!lhs.size) | (!rhs.size)) return BigInteger();
#ifdef FFT_MULTIPLICATION
		uint32 sizeSum = lhs.size + rhs.size;
		if (sizeSum == 2) return BigInteger((unsigned long long)*lhs._valPtr * *rhs._valPtr);
		if (sizeSum <= FFT_MAX_N / (BITS_OF_DWORD / FFT_WORD_BITLEN)) {
			BigInteger ret;
			_FFTMultiply(ret, lhs, rhs);
			return ret;
		}
#endif
		if (lhs.capacity != rhs.capacity)
		{
			const BigInteger& longer = (lhs.capacity >= rhs.capacity) ? lhs : rhs;
			const BigInteger& shorter = (lhs.capacity >= rhs.capacity) ? rhs : lhs;
			unsigned char _sharedMemBigIntegers[2 * sizeof(BigInteger)];
			BigInteger* sharedMemeoryBigIntegers = reinterpret_cast<BigInteger*>(_sharedMemBigIntegers);
			longer._seperateToHigherAndLowerHalfs(sharedMemeoryBigIntegers);//initialize read only variables
			BigInteger ret = _absMultiply(sharedMemeoryBigIntegers[0], shorter);
			ret <<= (longer.capacity << 4);
			ret += _absMultiply(sharedMemeoryBigIntegers[1], shorter);
			return ret;
		}
		if (lhs.capacity > 1)
		{
			unsigned char _sharedMemBigIntegers[4 * sizeof(BigInteger)];
			BigInteger *lhsParts = reinterpret_cast<BigInteger*>(_sharedMemBigIntegers), *rhsParts = 2 + lhsParts;//read only variables
			lhs._seperateToHigherAndLowerHalfs(lhsParts);
			rhs._seperateToHigherAndLowerHalfs(rhsParts);//initialize read only variables
			BigInteger higher_part = _absMultiply(lhsParts[0], rhsParts[0]);
			BigInteger lower_part = _absMultiply(lhsParts[1], rhsParts[1]);
			BigInteger mid_part = higher_part + lower_part;
			mid_part += ((lhsParts[1] - lhsParts[0]) * (rhsParts[0] - rhsParts[1]));
			higher_part <<= (lhs.capacity << 5);
			mid_part <<= (lhs.capacity << 4);
			higher_part += (mid_part += lower_part);
			return higher_part;
		}
		return BigInteger((unsigned long long)*lhs._valPtr * *rhs._valPtr);
	}
	
	BigInteger BigInteger::absDivideAndSetThisToRemainder(const BigInteger& rhs)
	{//set *this to remainder and return the quotient
		if (this == &rhs) {	this->size = 0; return BigInteger(1); }
		int pDiff = this->size - rhs.size;
		if (0 == rhs.size) throw std::logic_error("Divide by zero");
		if (pDiff < 0) return BigInteger();
		int pow = ((pDiff + 1) << 5);
		bool* binaryList = new bool[pow];
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
		ret.size = 1 + ((uint32)leftMostBit / BITS_OF_DWORD);
		ret._valPtr = new uint32[ret.capacity = _getMinimumCapacity(ret.size)];
		for (uint32 piece = 0; pow < leftMostBit; ++piece)
		{
			ret._valPtr[piece] = 0;
			for (uint32 bi = 0; bi < BITS_OF_DWORD; ++bi)
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
		int ret = _valPtr[0] & 0x7FFFFFFF;
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
	{
		char* c_str = new char[(size << 3) + 2];
		char* c = c_str;
		if (size)
		{
			if (negativity) *(c++) = '-';
			uint32 *v_ptr;
			for (v_ptr = _valPtr + size; v_ptr > _valPtr; ) {
				uint32 piece = *(--v_ptr), shifting = BITS_OF_DWORD;
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
		size_t digitsSize = this->size * BITS_OF_DWORD;
		bool* ret = new bool[digitsSize];
		for (uint32 i = 0, pieceIndex = 0; pieceIndex < this->size; ++pieceIndex, i += BITS_OF_DWORD)
		{
			uint32 piece = this->_valPtr[pieceIndex];
			ret[i + 0] = (piece & (1 << 0)) != 0; ret[i + 1] = (piece & (1 << 1)) != 0;	ret[i + 2] = (piece & (1 << 2)) != 0; ret[i + 3] = (piece & (1 << 3)) != 0;
			ret[i + 4] = (piece & (1 << 4)) != 0; ret[i + 5] = (piece & (1 << 5)) != 0;	ret[i + 6] = (piece & (1 << 6)) != 0; ret[i + 7] = (piece & (1 << 7)) != 0;
			ret[i + 8] = (piece & (1 << 8)) != 0; ret[i + 9] = (piece & (1 << 9)) != 0; ret[i + 10] = (piece & (1 << 10)) != 0; ret[i + 11] = (piece & (1 << 11)) != 0;
			ret[i + 12] = (piece & (1 << 12)) != 0;	ret[i + 13] = (piece & (1 << 13)) != 0;	ret[i + 14] = (piece & (1 << 14)) != 0;	ret[i + 15] = (piece & (1 << 15)) != 0;
			static_assert(BITS_OF_DWORD == 32, "convertAbsToBinaryArray");
			ret[i + 16] = (piece & (1 << 16)) != 0;	ret[i + 17] = (piece & (1 << 17)) != 0;	ret[i + 18] = (piece & (1 << 18)) != 0;	ret[i + 19] = (piece & (1 << 19)) != 0;
			ret[i + 20] = (piece & (1 << 20)) != 0;	ret[i + 21] = (piece & (1 << 21)) != 0;	ret[i + 22] = (piece & (1 << 22)) != 0;	ret[i + 23] = (piece & (1 << 23)) != 0;
			ret[i + 24] = (piece & (1 << 24)) != 0;	ret[i + 25] = (piece & (1 << 25)) != 0;	ret[i + 26] = (piece & (1 << 26)) != 0;	ret[i + 27] = (piece & (1 << 27)) != 0;
			ret[i + 28] = (piece & (1 << 28)) != 0;	ret[i + 29] = (piece & (1 << 29)) != 0;	ret[i + 30] = (piece & (1 << 30)) != 0;	ret[i + 31] = (piece & (1 << 31)) != 0;
		}
		if (out_digitsSize) {
			do { --digitsSize; } while (!ret[digitsSize]);
			*out_digitsSize = ++digitsSize;
		}
		return ret;
	}

	size_t BigInteger::getBitLength() const
	{
		if (size == 0) return 0;
		size_t lowerBound = size * BITS_OF_DWORD - BITS_OF_DWORD;
		uint32 x = _valPtr[size - 1];
		while (x) { ++lowerBound; x >>= 1; }
		return lowerBound;
	}

	BigInteger BigInteger::pow(uint32 positive_exponent) const
	{
		if (0 == positive_exponent) return BigInteger(1);
		BigInteger t = this->pow(positive_exponent >> 1);
		BigInteger tSquared = _absMultiply(t, t);
		if (positive_exponent & 1) {
			BigInteger ret = _absMultiply(*this, tSquared);
			ret.negativity = this->negativity;
			return ret;
		}
		return tSquared;
	}

	//`m` must be a odd number and larger than BigInteger(2) to apply montgomeryModPow method
	template<bool montgomeryModPow>
	BigInteger BigInteger::modPow(const BigInteger& exponent, const BigInteger& m) const
	{
		if (0 == exponent.size) return BigInteger(1);
		if ((0 == m.size) | m.negativity) throw std::logic_error("modular for `modPow` function must be positive.");
		size_t digitsSizeOfExponent;
		bool* binaryArrayOfExponent = exponent.convertAbsToBinaryArray(&digitsSizeOfExponent);
		BigInteger base = exponent.negativity ? this->modInverse(m) : *this;
		if (base.compareAbsoluteValueTo(m) > 0) base.absDivideAndSetThisToRemainder(m);
		if (base.negativity) base += m;
		BigInteger result = !montgomeryModPow ?
			base._trivialModPow(binaryArrayOfExponent, digitsSizeOfExponent, m) :
			base._montgomeryModPow(binaryArrayOfExponent, digitsSizeOfExponent, m);
		delete[] binaryArrayOfExponent;
		return result;
	}

	BigInteger BigInteger::_trivialModPow(bool* binaryArrayOfExponent, size_t bitsOfExponent, const BigInteger& m) const
	{
		BigInteger result(1), newResult;
		BigInteger tSquared(*this), thisPow_i;
		for (size_t i = 0; i < bitsOfExponent; ++i) {
			thisPow_i = std::move(tSquared);
			if (binaryArrayOfExponent[i]) {
				newResult = _absMultiply(result, thisPow_i);
				newResult.absDivideAndSetThisToRemainder(m);
				result = std::move(newResult);
			}
			tSquared = _absMultiply(thisPow_i, thisPow_i);
			tSquared.absDivideAndSetThisToRemainder(m);
		}
		return result;
	}

	//http://www.hackersdelight.org/MontgomeryMultiplication.pdf
	BigInteger BigInteger::_montgomeryModPow(bool* binaryArrayOfExponent, size_t bitsOfExponent, const BigInteger& m) const
	{
		BigInteger r(1);
		r <<= m.size * BITS_OF_DWORD;
		BigInteger r_tmp(r), m_tmp(m);
		_EEAstruct eeaStruct = _extendedEuclid(&r_tmp, &m_tmp);
		BigInteger r_slash = std::move(*eeaStruct.x);
		BigInteger m_slash = std::move(*eeaStruct.y); m_slash.negativity = !m_slash.negativity;
		delete eeaStruct.d; delete eeaStruct.x; delete eeaStruct.y;

		BigInteger result_mf(r), newResult;
		BigInteger thisPow_2i(*this * r), thisPow_i;
		result_mf.absDivideAndSetThisToRemainder(m);
		thisPow_2i.absDivideAndSetThisToRemainder(m);
		for (size_t i = 0; i < bitsOfExponent; ++i) {
			thisPow_i = std::move(thisPow_2i);
			if (binaryArrayOfExponent[i]) {
				newResult = result_mf * thisPow_i;//t = a_ * b_
				BigInteger tm_slash = newResult * m_slash;//tm'
				if (tm_slash.size > m.size) tm_slash.size = m.size;//tm' mod r
				newResult += tm_slash * m;// (t + (tm' mod r)m) 
				newResult >>= BITS_OF_DWORD * m.size;//u = (t + (tm' mod r)m) / r
				if (newResult.compareAbsoluteValueTo(m) >= 0) newResult._absSubtract(m);//if (u >= m) return u - m;
				result_mf = std::move(newResult);
			}
			thisPow_2i = thisPow_i * thisPow_i;
			BigInteger tm_slash = thisPow_2i * m_slash;
			if (tm_slash.size > m.size) tm_slash.size = m.size;
			thisPow_2i += tm_slash * m;
			thisPow_2i >>= BITS_OF_DWORD * m.size;
			if (thisPow_2i.compareAbsoluteValueTo(m) >= 0) thisPow_2i._absSubtract(m);
		}

		BigInteger result = result_mf * r.modInverse(m);
		result.absDivideAndSetThisToRemainder(m);
		if (result < 0) result += m;
		return result;
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

	//Set `confidenceFactor` <= 0 to run Deterministic Robin-Miller Primality Test.
	//When testing integers contain more than 1000 bits, it makes it a LITTLE BIT faster to 
	//set `confidenceFactor` to a positive number at the expense of 
	//the probability of getting a wrong result is no more than 4^(-confidenceFactor)
	bool BigInteger::isPrime(int confidenceFactor) const
	{
		if (0 == size || negativity) return false;
		if (1 == size && _valPtr[0] <= 3) return (_valPtr[0] > 1);
		if ((_valPtr[0] & 1) == 0) return false;
		BigInteger thisMinus1(*this); thisMinus1 += BigInteger(-1);
		double ln_n = log(size * BITS_OF_DWORD);
		int maxTesterForDeterministicResult = (int)(2.0 * ln_n * ln_n);
		BigInteger a(1);
		if (size <= 2) goto qwordTest;
		if (maxTesterForDeterministicResult <= confidenceFactor) goto drmpt;
		if (confidenceFactor > 0)
		{
qwordTest:	const char kTestNumbers[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37 };
			int index = 0;
			do {
				a._valPtr[0] = kTestNumbers[index];
				if (a.compareAbsoluteValueTo(thisMinus1) >= 0) return true;
				if (!_MillerRabinPrimalityTest(a, thisMinus1)) return false;
			} while (++index < sizeof(kTestNumbers) / sizeof(char));
			if (size > 2) {
				confidenceFactor -= (sizeof(kTestNumbers) / sizeof(char));
				std::mt19937 mt(gRandomDevice());
				do {
					a._valPtr[0] = mt();
					if (a._valPtr[0] < 41) a._valPtr[0] += 41;
					if (!_MillerRabinPrimalityTest(a, thisMinus1)) return false;
				} while (--confidenceFactor > 0);
			}
		}
		else
		{
drmpt:		uint32 base = 2;
			do {
				a._valPtr[0] = base;
				if (!_MillerRabinPrimalityTest(a, thisMinus1)) return false;
			} while (++base <= (uint32)maxTesterForDeterministicResult);
		}
		return true;
	}

	//	http://www.sti15.com/nt/pseudoprimes.html
	bool BigInteger::weakerBailliePSWPrimeTest() const
	{
		if (0 == size || negativity) return false;
		if (1 == size && _valPtr[0] <= 3) return (_valPtr[0] > 1);
		if ((_valPtr[0] & 1) == 0) return false;
		BigInteger thisMinus1(*this); thisMinus1 += BigInteger(-1);
		BigInteger a(2);
		if (!_MillerRabinPrimalityTest(a, thisMinus1)) return false;
		if (_LucasPseudoprimeTest()) {
			a._valPtr[0] = 3;
			if (_MillerRabinPrimalityTest(a, thisMinus1)) return true;
			throw std::logic_error("Severe logic flaw detected during `weakerBailliePSWPrimeTest`, please report this issue.");
		}
		return false;
	}

	bool BigInteger::_MillerRabinPrimalityTest(const BigInteger& a, const BigInteger& thisMinus1) const
	{//logically wrong when *this <= 2
		const BigInteger kBigInteger_1(1);
		if (a.modPow<true>(thisMinus1, *this) != kBigInteger_1) return false;
		BigInteger u(thisMinus1);
		do { u >>= 1; } while ((u._valPtr[0] & 1) == 0);
		BigInteger x = a.modPow<true>(u, *this);
		do {
			BigInteger y = _absMultiply(x, x);
			y.absDivideAndSetThisToRemainder(*this);
			if (y == kBigInteger_1 && x != kBigInteger_1 && x != thisMinus1) return false;
			x = std::move(y);
			u <<= 1;
		} while (u < *this);
		if (x != kBigInteger_1) return false;
		return true;
	}

	//Calculating the Jacobi symbol according to the properties at https://en.wikipedia.org/wiki/Jacobi_symbol
	int BigInteger::JacobiSymbol(const BigInteger& upperArgument, const BigInteger& lowerArgument)
	{
		if ((!lowerArgument.testBit(0)) | lowerArgument.negativity)
			throw std::logic_error("lowerArgument of function `JacobiSymbol` must be a positive odd number.");
		BigInteger denominator(lowerArgument);
		BigInteger numerator(upperArgument);
		if (numerator.negativity) {
			numerator.absDivideAndSetThisToRemainder(denominator);
			numerator += denominator;
		}
		return _JacobiSymbolImpl(&numerator, &denominator);
	}

	//	http://math.fau.edu/richman/jacobi.htm
	int BigInteger::_JacobiSymbolImpl(BigInteger* numerator, BigInteger* denominator)
	{
		if (denominator->size == 1 && denominator->_valPtr[0] == 1) return 1;//"Following the normal convention for the empty product, (Z / 1) = 1 for all `Z` "
		if (numerator->size == 0) return 0;
		numerator->absDivideAndSetThisToRemainder(*denominator);
		unsigned countFactorsOf2 = 0;
		for (; false == numerator->testBit(0) && numerator->size != 0; *numerator >>= 1) ++countFactorsOf2;
		if (gcd(*numerator, *denominator) != 1) return 0;
		int ret = 1;
		if (countFactorsOf2 & 1) {
			uint32 m = denominator->_valPtr[0] % 8;
			ret = ((m == 1) | (m == 7)) ? 1 : -1;
		}
		if (denominator->_valPtr[0] % 4 == 3 && numerator->_valPtr[0] % 4 == 3) ret = -ret;
		return ret * _JacobiSymbolImpl(denominator, numerator);
	}

	//	http://stackoverflow.com/questions/38343738/lucas-probable-prime-test
	std::pair<BigInteger, BigInteger> BigInteger::_getModLucasSequence(const BigInteger& k, const BigInteger& D, const BigInteger& P, const BigInteger& Q) const
	{
		BigInteger U_n(1), V_n(P), Q_n(Q);
		if (V_n < 0) { V_n.absDivideAndSetThisToRemainder(*this); V_n += *this; }
		if (Q_n < 0) { Q_n.absDivideAndSetThisToRemainder(*this); Q_n += *this;	}
		BigInteger U(!k.testBit(0) ? 0 : 1);
		BigInteger V(!k.testBit(0) ? 2 : V_n);
		BigInteger n(k);
		n >>= 1;
		while (n.size)
		{
			BigInteger U2 = _absMultiply(U_n, V_n);
			U2.absDivideAndSetThisToRemainder(*this);
			BigInteger V2 = _absMultiply(V_n, V_n);
			V2 -= (Q_n << 1);
			V2.absDivideAndSetThisToRemainder(*this);
			if (V2 < 0) V2 += *this;
			BigInteger Q2 = _absMultiply(Q_n, Q_n);
			Q2.absDivideAndSetThisToRemainder(*this);
			if ((n._valPtr[0] & 1)) {
				BigInteger newU = _absMultiply(U, V2) + _absMultiply(V, U2);
				if (newU.testBit(0)) newU += *this;
				newU >>= 1;
				newU.absDivideAndSetThisToRemainder(*this);
				BigInteger newV = _absMultiply(V, V2) + _absMultiply(U, U2) * D;
				if (newV.testBit(0)) newV += *this;
				newV >>= 1;
				newV.absDivideAndSetThisToRemainder(*this);
				if (newV < 0) newV += *this;
				U = std::move(newU);
				V = std::move(newV);
			}
			U_n = std::move(U2);
			V_n = std::move(V2);
			Q_n = std::move(Q2);
			n >>= 1;
		}
		return std::pair<BigInteger, BigInteger>(U, V);
	}

	bool BigInteger::_LucasPseudoprimeTest() const
	{//logically wrong when *this is small
		int D = 5;
		do {
			if (gcd(D, *this) != 1) return 0;
			if (JacobiSymbol(D, *this) == -1) break;
			D = -D + ((D < 0) ? 2 : -2);
		} while (true);
		const BigInteger Q((1 - D) / 4);
		BigInteger deltaThis(*this); deltaThis += 1;
		std::pair<BigInteger, BigInteger> UV = this->_getModLucasSequence(deltaThis, D, 1, Q);
		return (UV.first.size == 0);
	}

	//Warnning: not suitable when *this has more than 10k bits
	BigInteger BigInteger::nextProbablePrime() const
	{
		if (this->negativity || this->size == 0) return BigInteger(2);
		BigInteger ret(*this);
		if (ret.size == 1) {
			while (true) {
				ret += 1;
				if (ret.isPrime()) return ret;
			}
		}
		if ((ret._valPtr[0] & 1) == 0) ++ret._valPtr[0];
		else ret += 2;
		const char kTestNumbers[] = { 3, 5, 7, 11, 13, 17, 19, 23 };
		BigInteger m(2);
		static_assert(sizeof(char) == 1, "");
		uint32 modList[sizeof(kTestNumbers)];
		for (int i = 0; i < sizeof(kTestNumbers); ++i) {
			m._valPtr[0] = kTestNumbers[i];
			BigInteger x(ret);
			x.absDivideAndSetThisToRemainder(m);
			modList[i] = x.size ? x._valPtr[0] : 0;
		}
		while (true)
		{
			bool multipleOfTestNumbers = false;
			for (int i = 0; i < sizeof(kTestNumbers); ++i) {
				if (0 == modList[i] % kTestNumbers[i]) multipleOfTestNumbers = true;
				modList[i] += 2;
			}
			if (!multipleOfTestNumbers && ret.weakerBailliePSWPrimeTest()) return ret;
			ret += 2;
		}
	}

	//for cryptography
	BigInteger::BigInteger(const void* dataPtr, size_t dataSize)
	{
		size = (dataSize + sizeof(uint32) - 1) / sizeof(uint32);
		_valPtr = new uint32[capacity = _getMinimumCapacity(size)];
		memcpy(_valPtr, dataPtr, dataSize);
		memset(reinterpret_cast<char*>(_valPtr) + dataSize, 0, capacity * sizeof(uint32) - dataSize);
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

	void BigInteger::setLowerBitsToRandom(uint32 bitLength)
	{
		if (bitLength == 0) return;
		std::mt19937 mt(gRandomDevice());
		uint32 newSize = (bitLength + BITS_OF_DWORD - 1) / BITS_OF_DWORD;
		if (newSize > size)	{
			size = newSize;
			if (size > capacity) _valPtr = new uint32[capacity = _getMinimumCapacity(size)];
			_valPtr[size - 1] = 0;
		}
		uint32 i = 0;
		while (bitLength >= BITS_OF_DWORD) {
			_valPtr[i++] ^= mt();
			bitLength -= BITS_OF_DWORD;
		}
		if (bitLength) _valPtr[i] ^= (mt() & ((1 << bitLength) - 1));
		_setSize(size);
	}

	bool BigInteger::testBit(uint32 bitPos) const
	{
		uint32 dwordIndex = bitPos / BITS_OF_DWORD;
		if (dwordIndex >= size) return false;
		uint32 bitIndex = bitPos % BITS_OF_DWORD;
		return 0 != (_valPtr[dwordIndex] & (1 << bitIndex));
	}

	BigInteger::_SharedMemBigInteger::_SharedMemBigInteger(uint32 size_, uint32* val)
	{
		_valPtr = val;
		_setSize(size_);
		capacity = _getMinimumCapacity(size);
	}

}//namespace UPMath

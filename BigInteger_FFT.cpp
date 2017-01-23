//Open source under MIT license. https://github.com/PrismTower/BigInteger

#include "BigInteger.h"

#ifdef FFT_MULTIPLICATION
namespace UPmath
{
	bool BigInteger::_isRootsOfUnitySet = false;

	namespace BigInteger_FFT_Precedures
	{
		struct Complex {
			double re, im;
			Complex(double re = 0.0, double im = 0.0) : re(re), im(im) {}
			static const Complex add(const Complex& lhs, const Complex& rhs) { return Complex(lhs.re + rhs.re, lhs.im + rhs.im); }
			void add(const Complex& rhs) { re += rhs.re; im += rhs.im; }
			static const Complex subtract(const Complex& lhs, const Complex& rhs) { return Complex(lhs.re - rhs.re, lhs.im - rhs.im); }
			static const Complex multiply(const Complex& lhs, const Complex& rhs) {	return Complex(lhs.re * rhs.re - lhs.im * rhs.im, lhs.re * rhs.im + rhs.re * lhs.im); }
		};

#define FFT_WORD_SIZE (FFT_WORD_BITLEN / 8)
		const union {
			uint32 uint_value;
			char char_value[4];
		} _EndianCheckUnion = { 0x11223344 };
		constexpr bool _IS_LITTLE_ENDIAN() { return _EndianCheckUnion.char_value[0] == 0x44; }

		struct Coefficients {
			unsigned short *start, *end;
			static_assert(sizeof(unsigned short) == FFT_WORD_SIZE, "Length of FFT_WORD is 16bits, which differs from sizeof(unsigned short).");
			uint32 interval;
			uint32 n;
			Coefficients(const BigInteger& src, uint32 n) : interval(1), n(n)
			{
				start = reinterpret_cast<unsigned short*>(src._valPtr);
				end = reinterpret_cast<unsigned short*>(src._valPtr + src.size);
			}
			inline const Complex getComplexNumber() const
			{
				if (_IS_LITTLE_ENDIAN())
					return Complex(start < end ? (double)*start : 0.0);
				else {//WARNING: no tests has been done for big endian machines.
					if (start >= end) return Complex();
					if ((uint32)start & FFT_WORD_SIZE) return *(start - FFT_WORD_SIZE);
					return *(start + FFT_WORD_SIZE);
				}
			}
		};

		struct ValueRepresentation {
			Complex* start;
			uint32 interval;
			uint32 n;
			ValueRepresentation(Complex* src, uint32 n) : interval(1), n(n), start(src) { }
			inline const Complex& getComplexNumber() const { return *start; }
		};

		Complex kRootsOfUnity[FFT_MAX_N];
		void initRootsOfUnity()
		{
			const double scale = 6.2831853071795864769 / FFT_MAX_N;
			for (int i = 0; i < FFT_MAX_N; ++i) {
				kRootsOfUnity[i].re = cos(i * scale);
				kRootsOfUnity[i].im = sin(i * scale);
			}
		}

		inline uint32 getIndexOfPower(uint32 omegaIndex, uint32 pow) { return (omegaIndex * pow) % FFT_MAX_N; }

		//`InputType` should be either `Coefficients` or `ValueRepresentation`
		template <typename InputType>
		void FFT(Complex* dst, Complex* buffer, const InputType& src, uint32 omegaIndex)
		{
			if (1 == src.n) { dst[0] = src.getComplexNumber(); return; }
			InputType A(src);
			A.interval <<= 1;
			A.n >>= 1;
			FFT(buffer, dst, A, getIndexOfPower(omegaIndex, 2));//A even
			A.start += src.interval;
			FFT(buffer + A.n, dst, A, getIndexOfPower(omegaIndex, 2));//A odd
			for (uint32 j = 0; j < A.n; ++j) {
				Complex wjAo = Complex::multiply(kRootsOfUnity[getIndexOfPower(omegaIndex, j)], (buffer + A.n)[j]);
				dst[j] = Complex::add(buffer[j], wjAo);
				(dst + A.n)[j] = Complex::subtract(buffer[j], wjAo);
			}
		}

	}//namespace BigInteger_FFT_Precedures


	void BigInteger::_FFTMultiply(BigInteger& dst, const BigInteger& lhs, const BigInteger& rhs, void* buffer)
	{
		if ((!lhs.size) | (!rhs.size)) { dst.clearToZero(); return; }
		const uint32 n = _getMinimumCapacity(lhs.size + rhs.size) * BITS_OF_DWORD / FFT_WORD_BITLEN;
		const uint32 omegaIndex = FFT_MAX_N / n;
		if (0 == n) { dst = lhs * rhs; return; }
		if (!_isRootsOfUnitySet) {//initialize  (1 + 0i) ^ (1/n)
			BigInteger_FFT_Precedures::initRootsOfUnity();
			_isRootsOfUnitySet = true;
		}
		bool newMemAlloced = (nullptr == buffer);
		if (newMemAlloced) buffer = operator new(n * 4 * sizeof(BigInteger_FFT_Precedures::Complex));
		BigInteger_FFT_Precedures::Complex* cBuffer = reinterpret_cast<BigInteger_FFT_Precedures::Complex*>(buffer);
		//multi-threading is beneficial here
		BigInteger_FFT_Precedures::Coefficients A(lhs, n);
		BigInteger_FFT_Precedures::Complex* lhsValues = cBuffer + n;
		BigInteger_FFT_Precedures::FFT(lhsValues, lhsValues - n, A, omegaIndex);
		BigInteger_FFT_Precedures::Coefficients B(rhs, n);
		BigInteger_FFT_Precedures::Complex* rhsValues = cBuffer + 2 * n;
		BigInteger_FFT_Precedures::FFT(rhsValues, rhsValues + n, B, omegaIndex);
		for (uint32 j = 0; j < n; ++j)
			cBuffer[j] = BigInteger_FFT_Precedures::Complex::multiply(*lhsValues++, *rhsValues++);
		BigInteger_FFT_Precedures::ValueRepresentation C(cBuffer, n);
		BigInteger_FFT_Precedures::Complex* products = cBuffer + n;
		BigInteger_FFT_Precedures::FFT(products, products + n, C, FFT_MAX_N - omegaIndex);

		uint32 newCapa = n / (BITS_OF_DWORD / FFT_WORD_BITLEN);
		if (newCapa > dst.capacity || !dst._exclusivelyMemoryAllocated) {
			if (dst._exclusivelyMemoryAllocated) delete[] dst._valPtr;
			dst._valPtr = new uint32[dst.capacity = newCapa];
			dst._exclusivelyMemoryAllocated = true;
		}
		unsigned long long t = 0;
		static_assert(sizeof(unsigned long long) > sizeof(uint32), "");
		for (uint32 pieceIndex = 0; pieceIndex < newCapa; ++pieceIndex)	{
			//double er = abs(round((products)->re / n) - (products)->re / n); if (er > __dbgMiniError) __dbgMiniError = er;
			t += (products++)->re / n + 0.5;
			dst._valPtr[pieceIndex] = t & ((1 << FFT_WORD_BITLEN) - 1);
			t >>= FFT_WORD_BITLEN;
			//er = abs(round((products)->re / n) - (products)->re / n); if (er > __dbgMiniError) __dbgMiniError = er;
			t += (products++)->re / n + 0.5;
			dst._valPtr[pieceIndex] |= uint32(t) << FFT_WORD_BITLEN;
			t >>= FFT_WORD_BITLEN;
		}
		dst._setSize(newCapa);
		
		if (newMemAlloced) operator delete(buffer);
	}


}//namespace UPMath

#endif // FFT_MULTIPLICATION

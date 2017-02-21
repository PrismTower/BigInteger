//Open source under MIT license. https://github.com/PrismTower/BigInteger

#include "BigInteger.h"

#ifdef FFT_MULTIPLICATION
namespace UPmath
{
	bool BigInteger::_isRootsOfUnityInitialized = false;

	namespace BigInteger_FFT_Precedures
	{
		struct Complex {
			double re, im;
			Complex(double _re = 0.0, double _im = 0.0) : re(_re), im(_im) { }
			static Complex add(const Complex& lhs, const Complex& rhs) { return Complex(lhs.re + rhs.re, lhs.im + rhs.im); }
			static Complex subtract(const Complex& lhs, const Complex& rhs) { return Complex(lhs.re - rhs.re, lhs.im - rhs.im); }
			static Complex multiply(const Complex& lhs, const Complex& rhs) { return Complex(lhs.re * rhs.re - lhs.im * rhs.im, lhs.re * rhs.im + lhs.im * rhs.re); }
		};

#define FFT_WORD_SIZE (FFT_WORD_BITLEN / 8)
		/*const union {
		uint32 uint_value;
		char char_value[4];
		} _EndianCheckUnion = { 0x11223344 };
		constexpr bool _IS_LITTLE_ENDIAN() { return _EndianCheckUnion.char_value[0] == 0x44; }*/

		struct Coefficients {
			unsigned short *begin, *end;
			static_assert(sizeof(unsigned short) == FFT_WORD_SIZE, "Length of FFT_WORD is 16bits, which differs from sizeof(unsigned short).");
			uint32 interval;
			uint32 n;
			Coefficients(const BigInteger& src, uint32 n) : interval(1), n(n) { src._FFTgetBeginAndEndOfVal(begin, end); }
			inline Complex getComplexNumber(uint32 i) const
			{
				if (_IS_LITTLE_ENDIAN())
					return Complex(begin + i < end ? (double)begin[i] : 0.0);
				else {//WARNING: no tests has been done for big endian machines.
					if (begin + i >= end) return Complex();
					if ((size_t)begin & FFT_WORD_SIZE) return (double)begin[i - 1];
					return (double)begin[i + 1];
				}
			}
		};

		struct ValueRepresentation {
			Complex* begin;
			uint32 interval;
			uint32 n;
			ValueRepresentation(Complex* src, uint32 n) : interval(1), n(n), begin(src) { }
			inline const Complex& getComplexNumber(uint32 i) const { return begin[i]; }
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

		//`InputType` should be either `Coefficients` (isReverse = false)  or  `ValueRepresentation` (isReverse = true)
		template <bool isReverse, typename InputType>
		void FFT(Complex* dst, Complex* buffer, const InputType& src, uint32 omegaIndex)
		{//Warning: src.n cannot be smaller than 4
			if (src.n <= 4) {
				Complex x = src.getComplexNumber(0);
				Complex y = src.getComplexNumber(src.interval * 2);
				buffer[0] = Complex::add(x, y);
				buffer[1] = Complex::subtract(x, y);
				x = src.getComplexNumber(src.interval);
				y = src.getComplexNumber(src.interval * 3);
				buffer[2] = Complex::add(x, y);
				buffer[3] = Complex::subtract(x, y);
				Complex wjAo = buffer[2];
				dst[0] = Complex::add(buffer[0], wjAo);
				dst[2] = Complex::subtract(buffer[0], wjAo);
				wjAo = !isReverse ? Complex(-buffer[3].im, buffer[3].re) : Complex(buffer[3].im, -buffer[3].re);
				dst[1] = Complex::add(buffer[1], wjAo);
				dst[3] = Complex::subtract(buffer[1], wjAo);
				return;
			}
			InputType A(src);
			A.interval <<= 1;
			A.n >>= 1;
			FFT<isReverse>(buffer, dst, A, getIndexOfPower(omegaIndex, 2));//A even
			A.begin += src.interval;
			FFT<isReverse>(buffer + A.n, dst, A, getIndexOfPower(omegaIndex, 2));//A odd
			for (uint32 j = 0; j < A.n; ++j) {
				Complex wjAo = Complex::multiply(kRootsOfUnity[getIndexOfPower(omegaIndex, j)], (buffer + A.n)[j]);
				dst[j] = Complex::add(buffer[j], wjAo);
				(dst + A.n)[j] = Complex::subtract(buffer[j], wjAo);
			}
		}

	}//namespace BigInteger_FFT_Precedures


	void BigInteger::_FFTMultiply(BigInteger& dst, const BigInteger& lhs, const BigInteger& rhs, void* buffer)
	{
		if ((!lhs.size) | (!rhs.size)) { dst.size = 0; return; }
		const uint32 n = _getMinimumCapacity(lhs.size + rhs.size) * BITS_OF_DWORD / FFT_WORD_BITLEN;
		const uint32 kOmegaIndex = FFT_MAX_N / n;
		if (0 == kOmegaIndex) { dst = _absMultiply(lhs, rhs); return; }
		if (!_isRootsOfUnityInitialized) {//initialize  (1 + 0i) ^ (1/n)
			BigInteger_FFT_Precedures::initRootsOfUnity();
			_isRootsOfUnityInitialized = true;
		}
		bool newMemAlloced = (nullptr == buffer);
		if (newMemAlloced) buffer = operator new(n * 3 * sizeof(BigInteger_FFT_Precedures::Complex));
		BigInteger_FFT_Precedures::Complex* _buffer = reinterpret_cast<BigInteger_FFT_Precedures::Complex*>(buffer);
		//multi-threading benefits little here
		BigInteger_FFT_Precedures::Coefficients A(lhs, n);
		BigInteger_FFT_Precedures::Complex* lhsValues = _buffer + n;
		BigInteger_FFT_Precedures::FFT<false>(lhsValues, _buffer, A, kOmegaIndex);
		BigInteger_FFT_Precedures::Coefficients B(rhs, n);
		BigInteger_FFT_Precedures::Complex* rhsValues = lhsValues + n;
		BigInteger_FFT_Precedures::FFT<false>(rhsValues, _buffer, B, kOmegaIndex);
		for (uint32 j = 0; j < n; ++j)
			_buffer[j] = BigInteger_FFT_Precedures::Complex::multiply(*lhsValues++, *rhsValues++);
		BigInteger_FFT_Precedures::ValueRepresentation C(_buffer, n);
		BigInteger_FFT_Precedures::Complex* products = _buffer + n;
		BigInteger_FFT_Precedures::FFT<true>(products, products + n, C, FFT_MAX_N - kOmegaIndex);

		uint32 newCapa = n / (BITS_OF_DWORD / FFT_WORD_BITLEN);
		if (newCapa > dst.capacity) {
			delete[] dst._valPtr;
			dst._valPtr = new uint32[dst.capacity = newCapa];
		}
		unsigned long long t = 0;
		static_assert(sizeof(unsigned long long) > sizeof(uint32), "");
		for (uint32 pieceIndex = 0; pieceIndex < newCapa; ++pieceIndex) {
			//double er = abs(round((products)->re / n) - (products)->re / n); if (er > __dbgFFTMaxError) __dbgFFTMaxError = er;
			t += (unsigned long long)((products++)->re / n + 0.5);
			dst._valPtr[pieceIndex] = t & ((1 << FFT_WORD_BITLEN) - 1);
			t >>= FFT_WORD_BITLEN;
			//er = abs(round((products)->re / n) - (products)->re / n); if (er > __dbgFFTMaxError) __dbgFFTMaxError = er;
			t += (unsigned long long)((products++)->re / n + 0.5);
			dst._valPtr[pieceIndex] |= uint32(t) << FFT_WORD_BITLEN;
			t >>= FFT_WORD_BITLEN;
		}
		dst._setSize(newCapa);

		if (newMemAlloced) operator delete(buffer);
	}


}//namespace UPMath

#endif // FFT_MULTIPLICATION

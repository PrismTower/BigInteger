#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
//for memory leak detection

#include "BigInteger.h"
using namespace std;
using namespace UPmath;

const BigInteger factorial(int n)
{
	if (n < 2) return BigInteger(1);
	else return factorial(n - 1) * BigInteger(n);
}

const BigInteger fibonacci_naive(int n)
{
	if (n < 2) return BigInteger(n);
	else return fibonacci_naive(n - 1) + fibonacci_naive(n - 2);
}

const BigInteger fibonacci(int n)
{
	if (n < 2) return BigInteger(n);
	else
	{
		BigInteger *f_n_minus1 = new BigInteger(0);
		BigInteger *f_n_minus2 = new BigInteger(1);
		for (int i = 1; i < n; ++i) {
			*f_n_minus2 += *f_n_minus1;
			std::swap(f_n_minus1, f_n_minus2);
		}
		BigInteger ret = std::move(*f_n_minus1);
		delete f_n_minus1;
		delete f_n_minus2;
		return ret;
	}
}

bool MersennePrimeTester(uint32 p)
{
	BigInteger base(1);
	base <<= p;
	base -= 1;
	std::cout << base << std::endl;
	bool testResult = base.isProbablePrime();
	std::cout << "This number is " << (testResult ? "" : "NOT ") << "a prime." << std::endl;
	return testResult;
}

int main()
{
	int n;
	cout << "To calculate fibonacci(n), please input n here: ";
	cin >> n;
	cout << "fibonacci(" << n << ") = " << fibonacci(n) << endl;

	cout << "To calculate factorial(n), please input n here: ";
	cin >> n;
	cout << "factorial(" << n << ") = " << factorial(n) << endl;

	_CrtDumpMemoryLeaks();
	system("pause");
	return 0;
}
void gcdCalc(unsigned long long x, unsigned long long y)
{
	
	unsigned long long remainder;
	// While y != 0
	while(y)
	{
		std::cout << "X: " << x << "\n";
		std::cout << "Y: " << y << "\n";
		std::cout << "Quotient: " << x/y << "\n";
		remainder = x % y;
		std::cout << "Remainder: " << remainder << "\n";
		x = y;
		y = remainder;
	}
	// Print the output gcd
	std::cout << "GCD is: " << x;
}

#include<iostream>
#include<math.h>
int main(void)
{
    unsigned long long fibbBuff;

    for(int i = 50; i > 0; i--)
    {
        fibbBuff = ((1/sqrt(5)) * (pow((((1 + sqrt(5))/2)),i) - pow((((1 - sqrt(5))/2)),i))); // Binets Formula
        std::cout << std::endl << fibbBuff;
    }
    return 0;
}

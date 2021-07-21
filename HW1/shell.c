#include<stdio.h>
#include<unistd.h>
  
int main()
{
	char filename[20];  //ShowPrimes.asm, Factorize.asm, BubbleSort.asm

	while(1){
		printf("\n\nPlease enter the filename (ShowPrimes.asm, Factorize.asm, or BubbleSort.asm)\n");

		scanf("%s", &filename);

		//it should call CreateProcess syscall and run entered program. I could not do this in C.
	}

    return 0;
}

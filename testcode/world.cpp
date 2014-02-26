#include <iostream>
using namespace std;
#include <stdint.h>
#include <stdio.h>
int main(){
	
	char* a = "Mayank";
	char b[10];
	printf("p %p\n",a);
	printf("p %p\n",b);
	syscall(403, 7, a, 0, 0);
	syscall(402, 7, b, 0, 0);
	cout<<"A here"<<a<<endl;
	cout<<"B here"<<b<<endl;	
	return 0;
}

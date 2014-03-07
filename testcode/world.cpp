#include <iostream>
using namespace std;
#include <stdint.h>
#include <stdio.h>
int main(){
	
	char* a = "Mayank";
	char b[10];
	printf("p %p\n",a);
	printf("p %p\n",b);
	printf("Beforr Write - %d\n\n", syscall(400));
	syscall(403, 7, a, 0, 0);
	printf("After Write - %d\n\n", syscall(400));
	printf("Before Read - %d\n\n", syscall(400));
	syscall(402, 7, b, 0, 0);
	printf("After Read - %d\n\n", syscall(400));
	cout<<"A here"<<a<<endl;
	cout<<"B here"<<b<<endl;	
	return 0;
}

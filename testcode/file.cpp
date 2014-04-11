#include <iostream>
using namespace std;

int main(){
	char dirname[500] = "root/mayank";
	char filename[500] = "root/file.c";
	syscall(408, dirname);
	int file = syscall(402, filename, 'w');
	syscall(405, file, "hello", 5);
	// syscall(403, file);

	// file = syscall(402, filename, 'r');
	syscall(406, file, 0);
	char data[500];
	syscall(404, file, data, 5);
	cout<<data<<"  here I am.  "<<file<<endl;
	syscall(403, file);
	return 0;
}
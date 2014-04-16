#include <iostream>
using namespace std;

int main(){
	syscall(408, "root/d1");
	syscall(408, "root/d2");
	syscall(408, "root/d3");
	syscall(408, "root/d4");
	syscall(408, "root/d5");
	int file = syscall(402, "root/d1/f1", 'w');
	syscall(405, file, "hello", 6);
	syscall(403, file);

	file = syscall(402, "root/d1/f2", 'w');
	syscall(405, file, "hello", 6);
	syscall(403, file);

	file = syscall(402, "root/d1/f3", 'w');
	syscall(405, file, "hello", 6);
	syscall(403, file);

	file = syscall(402, "root/d1/f4", 'w');
	syscall(405, file, "hello", 6);
	syscall(403, file);

	file = syscall(402, "root/d2/f5", 'w');
	syscall(405, file, "hello", 6);
	syscall(403, file);

	file = syscall(402, "root/d4/f6", 'w');
	syscall(405, file, "hello", 6);
	syscall(403, file);

	cout<< "WRITE DONE\n";
	file = syscall(402, "root/d1/f1", 'r');
	char data[500];
	syscall(404, file, data, 6);
	cout<<data<<"  here I am.  "<<file<<endl;
	syscall(403, file);
	
	return 0;
}
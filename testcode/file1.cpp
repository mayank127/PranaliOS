#include <iostream>
using namespace std;

int main(){
	syscall(408, "root/d2");
	int file = syscall(402, "root/d2/f1", 'w');
	if(file != -1){
		syscall(405, file, "file 1 write", 12);
		syscall(403, file);
	}

	file = syscall(402, "root/d2/f2", 'w');
	if(file != -1){
		syscall(405, file, "file 2 write", 12);
		syscall(403, file);
	}

	syscall(411, "root/d2/f2", 0);

	file = syscall(402, "root/d2/f3", 'w');
	if(file != -1){
		syscall(405, file, "file 3 write", 12);
		syscall(403, file);
	}

	syscall(411, "root/d2/f3", 1);
	return 0;
}

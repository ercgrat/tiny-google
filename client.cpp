using namespace std;
#include <iostream>
#include <limits>
#include <fstream>
#include <cstring>
#include <cmath>

int main() {
	cout << "uiShell for tiny-Google\n--------------------------" << endl;
	
	while(true) {
		cout << "Enter a number to choose one of the following options:" << endl;
		cout << "\t1) Index a document " << endl;
		cout << "\t2) Search indexed documents" << endl;
		cout << "\t3) Quit\n" << endl;
		
		int input = 0;
		if(!(cin >> input)) {
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Invalid input. Try again.\n" << endl;
			continue;
		}
		
		if(input == 1) {
		} else if(input == 2) {
		} else if(input == 3) {
			cout << "Good bye!" << endl;
			break;
		}
	}
	
	
}
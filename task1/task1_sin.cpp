#include <iostream>
#include <cmath>
#include <vector>

using namespace std;

int main() {
	int size = pow(10, 7);
	vector<ARRAY_TYPE> array(size);

	for (int i = 0; i < size; i++) {
		array[i] = sin(i * (2 * M_PI / size));
	}

	ARRAY_TYPE sum = 0;
	for (int i = 0; i < size; i++) {
		sum += array[i];
	}

	cout << "Sum: " << sum << endl;

	return 0;
}

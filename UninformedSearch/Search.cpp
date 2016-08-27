#include <algorithm>
#include <cassert>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

template<size_t N>
struct Puzzle {
	int grid[N][N] = {};

	decltype(grid[N])& operator[](size_t i)
	{
		return grid[i];
	}

	// we cant actually use this for anything but testing
	bool HasSolution()
	{
		int inversions = grid[0][0] - 1;
		const auto size = N*N;
		for (auto i = 1; i < size - 1; ++i)
		{
			const auto cell = &grid[i / N][i % N];
			const auto val = *cell;

			size_t matches = count_if(cell+1, &grid[N-1][N-1], [&val](const int& v) {
				return v && v < val;
			});
			assert(matches < size - i);
			inversions += matches;
		}
		return inversions % 2;
	}
};

using Puzzle8 = Puzzle<3>;

int main()
{
	const int SIZE = 3;
	string file_name = "";
	cout << "Enter a file name to read a puzzle from." << endl;
	if (!(cin >> file_name))
	{
		cout << "\nThe file name was invalid." << endl;
		return 1;
	}

	ifstream in(file_name);
	if (!in)
	{
		cout << "The file was not found or could not be opened." << endl;
		return 1;
	}

	// Creates a 3x3 array to hold the puzzle
	int puzzle[SIZE][SIZE];
	// The location of the zero (i.e. empty) tile.
	int zero[2];
	int count = 0;
	while (in && count <= 8)
	{
		char temp = '\0';
		if (in >> temp)
		{
			if (temp == '_')
			{
				puzzle[count / SIZE][count % SIZE] = 0;
				zero[0] = count / SIZE;
				zero[1] = count++ % SIZE;
			}
			else
			{
				puzzle[count / SIZE][count % SIZE] = temp - '0';
				++count;
			}
		}
	};

	int solution[SIZE][SIZE] = {
		{ 1, 2, 3 }, 
		{ 4, 5, 6 }, 
		{ 7, 8, 0 } 
	};


	

	



	return 0;
}
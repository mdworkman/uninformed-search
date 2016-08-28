#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <queue>

using namespace std;

template<size_t N>
class Puzzle {
public:
	struct PuzzleState {
		static constexpr auto n = N;
		static constexpr auto size = N*N;

		int state[N][N] = {};

		decltype(state[N])& operator[](size_t i)
		{
			return state[i];
		}

		bool operator==(const PuzzleState& rhs) const
		{
			for (auto i = 0; i < size; ++i)
			{
				if (*nth(i) != rhs.nth(i)) {
					return false;
				}
			}
			return true;
		}

		using iterator = decltype(&state[0][0]);

		// providing iterators so we can easily navigate the 2d array as a flat structure
		iterator begin()
		{
			return &state[0][0];
		}

		iterator end()
		{
			return &state[N-1][N-1];
		}

		iterator nth(size_t n)
		{
			return &state[n / N][n % N];
		}

		friend ostream& operator<<(ostream& os, typename Puzzle<N>::PuzzleState& state)
		{
			for (auto i = 0; i < state.size; ++i)
			{
				if (i % state.n == 0) {
					cout << endl;
				}
				cout << *state.nth(i);
			}
			cout << endl;
			return os;
		}
	};

private:
	PuzzleState state;
	PuzzleState goal;

public:
	Puzzle(PuzzleState initial, PuzzleState goal)
	: state(initial), goal(goal) {}

	auto operator[](size_t i) -> decltype(state[i])
	{
		return state[i];
	}

	// we cant actually use this for anything but testing
	bool HasSolution()
	{
		int inversions = *state.begin() - 1;
		for (auto i = 1; i < state.size - 1; ++i)
		{
			const auto cell = state.nth(i);
			const auto val = *cell;

			// no tile is smaller than 1 (since blank tile is *always* ignored in this calculation
			if (val > 1) {
				size_t matches = count_if(cell+1, state.end(), [&val](const int& v) {
					return v && v < val;
				});
				assert(matches < state.size - i);
				inversions += matches;
			}
		}
		return inversions % 2 == 0;
	}

	bool Solve(/* probably need to pass a strategy */)
	{
		// not sure about any of this yet

		struct Node {

		};

		queue<Node> frontier;
		unordered_set<Node> explored;
	}

	bool IsSolved()
	{
		return state == goal;
	}

	bool MoveUp()
	{

	}

	bool MoveDown()
	{

	}

	bool MoveLeft()
	{

	}

	bool MoveRight()
	{

	}

	friend ostream& operator<<(ostream& os, Puzzle<N>& puzzle)
	{
		os << puzzle.state;
		os << puzzle.goal;
		return os;
	}
};

using Puzzle8 = Puzzle<3>;

int main()
{
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
	Puzzle8::PuzzleState state;
	auto size = state.n;
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
				state[count / size][count % size] = 0;
				zero[0] = count / size;
				zero[1] = count++ % size;
			}
			else
			{
				state[count / size][count % size] = temp - '0';
				++count;
			}
		}
	};

	Puzzle8::PuzzleState goal {{
		{ 1, 2, 3 },
		{ 4, 5, 6 },
		{ 7, 8, 0 }
	}};
	Puzzle8 puzzle(state, goal);

	cout << puzzle << endl;
	cout << puzzle.HasSolution() << endl;



	return 0;
}

#include <algorithm>
#include <cassert>
#include <cmath>
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

		PuzzleState() = default;

		PuzzleState(initializer_list<decltype(state)> l)
		{
			const int* start = &l.begin()[0][0][0];
			copy(start, start + size, begin());
		}

		PuzzleState(const PuzzleState& p)
		{
			copy(p.begin(), p.end(), begin());
		}

		PuzzleState& operator=(const PuzzleState& p)
		{
			if (&p != this) {
				copy(p.begin(), p.end(), begin());
			}
			return *this;
		}

		decltype(state[N])& operator[](size_t i)
		{
			return state[i];
		}

		bool operator==(const PuzzleState& rhs) const
		{
			// we dont have to check *all* the elements
			// since each puzzle alwyas has the same numbers we only have to check n - 1
			for (auto i = 0; i < size - 1; ++i)
			{
				if (*nth(i) != *rhs.nth(i)) {
					return false;
				}
			}
			return true;
		}

		size_t hash() const
		{
			static const size_t lg = log2(size); // calculate only once

			size_t h = 0;
			for (auto i = 0; i < size; ++i)
			{
				h |= i << ((*nth(i)) * lg);
			}
			return h;
		}

		using iterator = decltype(&state[0][0]);
		using const_iterator = const int*;

		// providing iterators so we can easily navigate the 2d array as a flat structure
		iterator begin()
		{
			return &state[0][0];
		}

		const_iterator begin() const
		{
			return &state[0][0];
		}

		iterator end()
		{
			return &state[N-1][N-1] + 1;
		}

		const_iterator end() const
		{
			return &state[N-1][N-1] + 1;
		}

		iterator nth(size_t n)
		{
			return &state[n / N][n % N];
		}

		const_iterator nth(size_t n) const
		{
			return &state[n / N][n % N];
		}

		friend ostream& operator<<(ostream& os, const typename Puzzle<N>::PuzzleState& state)
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

	int* blank = nullptr;

	void Swap(int* ptr)
	{
		swap(*ptr, *blank);
		blank = ptr;
		ptr = nullptr;
	}

public:
	Puzzle(const PuzzleState& initial, const PuzzleState& goal)
	: state(initial), goal(goal)
	{
		auto it = find_if(begin(state), end(state), bind2nd(equal_to<int>(), 0));
		assert(it != end(state)); // we must have a blank tile!
		blank = &(*it);
		assert(*blank == 0);
		// these asserts dont catch if there is more than one 0 so our puzzle may still be invalid
	}

	const PuzzleState& State() const
	{
		return state;
	}

	auto operator[](size_t i) -> decltype(state[i])
	{
		return state[i];
	}

	// we cant actually use this for anything but testing
	bool HasSolution() const
	{
		// FIXME: technically we should examine our goal state
		// this is hardcoded for our particular goal state :(

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
		enum MOVE {
			NONE = 0,
			UP,
			DOWN,
			LEFT,
			RIGHT
		};

		struct Node {
			Puzzle& puzzle;
			PuzzleState mystate;
			const Node* parent = nullptr;
			const size_t cost = 1;
			const MOVE action;

			// constructor for root node
			Node(Puzzle& puzzle)
			: puzzle(puzzle), mystate(puzzle.State()), action(MOVE::NONE) {}

			// constructor for child nodes
			Node(const Node& parent, MOVE action)
			: puzzle(parent.puzzle), parent(&parent), cost(parent.cost + 1), action(action)
			{
				assert(action); // dont accept NONE as a valid sequence
				switch (action)
				{
					case UP:
						puzzle.MoveUp();
						break;
					case DOWN:
						puzzle.MoveDown();
						break;
					case LEFT:
						puzzle.MoveLeft();
						break;
					case RIGHT:
						puzzle.MoveRight();
						break;
					default:
						throw logic_error("Invalid movement command attempted");
				}
				mystate = puzzle.State();
			}

			bool operator==(const Node& rhs) const
			{
				return mystate == rhs.mystate;
			}

			size_t hash() const
			{
				// we consider equality based on state alone
				return mystate.hash();
			}
		};

		// initialize the frontier with the start state
		queue<Node> frontier( { Node(*this) } );
		//unordered_set<Node> explored;

		while (!IsSolved() && !frontier.empty()) {
			Node& current = frontier.front();
			frontier.pop();
		}
		return false;
	}

	bool IsSolved() const
	{
		return state == goal;
	}

	bool MoveUp()
	{
		int* swp = blank + state.n;
		if (swp < state.end()) {
			Swap(swp);
			return true;
		}
		return false;
	}

	bool MoveDown()
	{
		int* swp = blank - state.n;
		if (swp >= state.begin()) {
			Swap(swp);;
			return true;
		}
		return false;
	}

	bool MoveLeft()
	{
		int* swp = blank + 1;
		if (swp < state.end() && (swp - state.begin()) % state.n != 0) {
			Swap(swp);
			return true;
		}
		return false;
	}

	bool MoveRight()
	{
		int* swp = blank - 1;
		if (swp >= state.begin() && (state.end() - swp) % state.n != 1) {
			Swap(swp);
			return true;
		}
		return false;
	}

	friend ostream& operator<<(ostream& os, const Puzzle<N>& puzzle)
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

	// testing garbage
	cout << puzzle << endl;
	cout << "Move up" << endl;
	puzzle.MoveUp();
	cout << puzzle << endl;
	cout << "Move up x2 then down" << endl;
	puzzle.MoveUp();
	puzzle.MoveUp();
	puzzle.MoveDown();
	cout << puzzle << endl;
	cout << "Move left x2" << endl;
	puzzle.MoveLeft();
	puzzle.MoveLeft();
	cout << puzzle << endl;
	cout << "Move right x4" << endl;
	puzzle.MoveRight();
	puzzle.MoveRight();
	puzzle.MoveRight();
	puzzle.MoveRight();
	cout << puzzle << endl;

	cout << puzzle.HasSolution() << endl;

	puzzle.Solve();

	return 0;
}

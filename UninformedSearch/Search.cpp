#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <unordered_set>


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
				if (i && i % state.n == 0) {
					cout << endl;
				}
				cout << *state.nth(i);
			}
			cout << endl;
			return os;
		}
	};

private:
	enum MOVE {
		NONE = 0,
		UP,
		DOWN,
		LEFT,
		RIGHT
	};

	PuzzleState state;
	PuzzleState goal;

	int* blank = nullptr;

	void Swap(int* ptr)
	{
		swap(*ptr, *blank);
		blank = ptr;
		ptr = nullptr;

		//cout << "new state:" << endl << state;
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

	int* GetMoveUp()
	{
		int* swp = blank + state.n;
		return (swp < state.end()) ? swp : nullptr;
	}

	int* GetMoveDown()
	{
		int* swp = blank - state.n;
		return (swp >= state.begin()) ? swp : nullptr;
	}

	int* GetMoveLeft()
	{
		int* swp = blank + 1;
		return (swp < state.end() && (swp - state.begin()) % state.n != 0) ? swp : nullptr;
	}

	int* GetMoveRight()
	{
		int* swp = blank - 1;
		return (swp >= state.begin() && (state.end() - swp) % state.n != 1) ? swp : nullptr;
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
		struct Node {
			PuzzleState state;
			shared_ptr<const Node> parent;
			const size_t cost = 1;
			const MOVE action;

			// constructor for root node
			Node(const PuzzleState& state)
			: state(state), action(MOVE::NONE) {}

			// constructor for child nodes
			Node(const Node& parent, MOVE action)
			: parent(make_shared<const Node>(parent)), cost(parent.cost + 1), action(action)
			{
				assert(action); // dont accept NONE as a valid sequence
				// less than ideal, but create a new puzzle with the parent state
				// manipulate it, then destroy it
				Puzzle puzzle(parent.state, parent.state);
				puzzle.Move(action);
				state = puzzle.State();
			}

			bool operator==(const Node& rhs) const
			{
				// we consider equality based on state alone
				return state == rhs.state;
			}

			void Trace() const
			{
				if (parent) {
					parent->Trace();

					static const char* symbols[] = {"", "↑","↓","←","→"};
					cout << symbols[action] << endl;

				} else {
					cout << "Path taken to solve:" << endl;
				}
			}
		};


		using NodePtr = shared_ptr<Node>;
		// initialize the frontier with the start state
		queue<NodePtr> frontier( {  make_shared<Node>(Node(state)) } );

		auto hasher=[](const NodePtr& nodeptr){
			return nodeptr->state.hash();
		};

		// FIXME: I've no idea what a good size table is
		unordered_set<NodePtr, decltype(hasher)> explored(1000, hasher);

		cout << "Attempting to solve puzzle:" << endl << state << endl;
		cout << "Beginning timer" << endl;
		chrono::steady_clock::time_point begin = chrono::steady_clock::now();

		NodePtr current;
		while (!frontier.empty()) {
			current = frontier.front();
			frontier.pop();

			explored.insert(current);

			if (current->state == goal) break;

			#define CHECK_NODE(dir) \
			if (CheckValidMove(dir)) { \
				auto node_##dir_ptr = make_shared<Node>(Node(*current, dir)); \
				if (node_##dir_ptr->state == goal) { \
					current = node_##dir_ptr; \
					break; \
				} \
				if (explored.count(node_##dir_ptr) == 0) { \
					frontier.push(node_##dir_ptr); \
				} \
			}

			// counterclockwise iteration?
			CHECK_NODE(UP);
			CHECK_NODE(LEFT);
			CHECK_NODE(DOWN);
			CHECK_NODE(RIGHT);

			#undef CHECK_NODE
		}

		cout << "Finished: stopping timer." << endl;
		chrono::steady_clock::time_point end = chrono::steady_clock::now();
		cout << "Time taken: " << chrono::duration_cast<chrono::milliseconds>(end - begin).count() << "ms" << endl;
		// FIXME: not sure what "expanded nodes" means
		cout << "Nodes explored:" << explored.size() << endl;

		// update puzzle to current state (even if not solved)
		state = current->state;

		if (IsSolved()) {
			// output the steps
			current->Trace();
			return true;
		}
		return false;
	}

	bool IsSolved() const
	{
		return state == goal;
	}

	// scramble the puzzle by i moves
	// great for loading a solved puzzle and scrambling for testing
	void Scramble(size_t i)
	{
		while (i--) {
			random_device rd;     // only used once to initialise (seed) engine
			mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
			uniform_int_distribution<int> uni(UP, RIGHT); // guaranteed unbiased

			Move(static_cast<MOVE>(uni(rng)));
		}
	}

	bool CheckValidMove(MOVE m)
	{
		switch (m)
		{
			case UP:
				return GetMoveUp();
			case DOWN:
				return GetMoveDown();
			case LEFT:
				return GetMoveLeft();
			case RIGHT:
				return GetMoveRight();
			default:
				throw logic_error("Invalid movement command attempted");
		}
	}

	bool Move(MOVE m)
	{
		switch (m)
		{
			case UP:
				return MoveUp();
			case DOWN:
				return MoveDown();
			case LEFT:
				return MoveLeft();
			case RIGHT:
				return MoveRight();
			default:
				throw logic_error("Invalid movement command attempted");
		}
	}

	bool MoveUp()
	{
		//cout << "move up" << endl;
		if (int* swp = GetMoveUp()) {
			Swap(swp);
			return true;
		}
		return false;
	}

	bool MoveDown()
	{
		//cout << "move down" << endl;
		if (int* swp = GetMoveDown()) {
			Swap(swp);;
			return true;
		}
		return false;
	}

	bool MoveLeft()
	{
		//cout << "move left" << endl;
		if (int* swp = GetMoveLeft()) {
			Swap(swp);
			return true;
		}
		return false;
	}

	bool MoveRight()
	{
		//cout << "move right" << endl;
		if (int* swp = GetMoveRight()) {
			Swap(swp);
			return true;
		}
		return false;
	}

	friend ostream& operator<<(ostream& os, const Puzzle<N>& puzzle)
	{
		return os << puzzle.state << endl;// << puzzle.goal;
	}
};

using Puzzle8 = Puzzle<3>;

void AnalyzePuzzle(Puzzle8& puzzle)
{
	bool hasSolution = puzzle.HasSolution();
	cout << "Puzzle has solution?: " << hasSolution << endl;
	bool solved = puzzle.Solve();
	cout << "Solved Puzzle:" << endl << puzzle << endl;
	assert(solved == hasSolution);
}

void Tests(const Puzzle8::PuzzleState& goal)
{
	cout << "Running tests" << endl;
	cout << goal << endl;
	Puzzle8 testPuzzle(goal, goal);
	testPuzzle.Scramble(10);
	assert(testPuzzle.HasSolution());

	AnalyzePuzzle(testPuzzle);
}

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

	Tests(goal);

	Puzzle8 puzzle(state, goal);
	AnalyzePuzzle(puzzle);

	return 0;
}

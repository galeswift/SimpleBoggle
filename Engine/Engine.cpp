// Engine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <SFML/Graphics.hpp>
#define NUM_ROWS (5)
#define NUM_COLS (5)
#define NUM_DICE (NUM_ROWS * NUM_COLS)
#define NUM_LETTERS (26)
#define MIN_WORD_LENGTH (3)
#define GAME_LENGTH (3*60)
#define DICE_SIZE (120)
#define DICE_MARGIN (10)
#define LETTER_SIZE (DICE_SIZE)
#define LETTER_ADJUSTMENT (sf::Vector2f(0,-40))

class Random
{
public:
	Random()
	{
		std::random_device rd;
		m_gen = new std::mt19937(rd());
	}

	int Rand(int min, int max)
	{
		std::uniform_int_distribution<> dis(min, max);
		return dis(*m_gen);
	}
	std::mt19937* m_gen;
};
Random g_rand;

class Dice
{
public:
	Dice(char a, char b, char c, char d, char e, char f)
	{
		m_letters[0] = a;
		m_letters[1] = b;
		m_letters[2] = c;
		m_letters[3] = d;
		m_letters[4] = e;
		m_letters[5] = f;
	}

	char Roll()
	{
		m_current = m_letters[g_rand.Rand(0, 5)];
		return m_current;
	}
	std::vector<Dice*> m_neighbors;
	char m_current;
	char m_letters[6];
};

class TrieNode
{
public:
	TrieNode(char val)
		: m_char(val)
		, m_isWord(false)
	{
		for (int i = 0; i < NUM_LETTERS; i++)
		{
			m_children[i] = NULL;
		}
	}

	void AddWord(const char* word, int length)
	{
		if (length <= 0)
		{
			m_isWord = true;
			return;
		}

		int idx = (int)word[0] - 'a';
		if (idx >= 0 && idx < NUM_LETTERS)
		{
			if (m_children[idx] == NULL)
			{
				m_children[idx] = new TrieNode(word[0]);
			}
			
			m_children[idx]->AddWord(word + 1, length - 1);
		}
	}

	void FindWords(Dice* dice, std::vector<std::string>& result, std::string currentString, std::vector<Dice*> usedDice) const
	{		
		std::vector<Dice*> availableDice;
		if (std::find(usedDice.begin(), usedDice.end(), dice) == usedDice.end())
		{
			availableDice.push_back(dice);
		}
		for (int j = 0; j < dice->m_neighbors.size(); j++)
		{
			Dice* diceNeighbor = dice->m_neighbors[j];
			if (std::find(usedDice.begin(), usedDice.end(), diceNeighbor) == usedDice.end())
			{
				availableDice.push_back(diceNeighbor);
			}
		}

		// go through each of our children
		for (int i = 0; i < NUM_LETTERS; i++)
		{
			TrieNode* child = m_children[i];
			if (child != NULL)
			{				
				for (int j = 0; j < availableDice.size(); j++)
				{
					Dice* currentDice = availableDice[j];
					if (child->m_char == tolower(currentDice->m_current))
					{
						currentString += toupper(child->m_char);
						if (child->m_isWord)
						{
							if (std::find(result.begin(), result.end(), currentString) == result.end())
							{																
								result.push_back(currentString);
							}
						}					
						usedDice.push_back(currentDice);
						child->FindWords(currentDice, result, currentString, usedDice);
						usedDice.pop_back();
						currentString.pop_back();
					}
				}				
			}		
		}		
	}
	bool IsWord(const char* word, int length) const
	{
		if (length <= 0)
		{
			return m_isWord;			
		}

		int idx = (int)word[0] - 'a';
		if (idx >= 0 && idx < NUM_LETTERS)
		{
			if (m_children[idx] == NULL)
			{
				return false;
			}

			return m_children[idx]->IsWord(word + 1, length - 1);
		}

		return false;
	}

	bool m_isWord;
	char m_char;
	TrieNode* m_children[NUM_LETTERS];
};

class Trie
{
public:
	Trie(const std::vector<std::string>& wordList) 
		: m_root(NULL)
	{
		for (auto str : wordList)
		{
			if (str.length() > MIN_WORD_LENGTH)
			{
				AddWord(str);
			}
		}
	}

	void AddWord(const std::string& word)
	{
		if (m_root == NULL)
		{
			m_root = new TrieNode(' ');
		}

		TrieNode* current = m_root;
		current->AddWord(word.c_str(), word.length());		
	}

	bool IsWord(const std::string& word) const
	{
		if (m_root == NULL)
		{
			return false;
		}
		return m_root->IsWord(word.c_str(), word.length());
	}

	void FindWords(Dice* current, std::vector<std::string>& result) const
	{
		if (m_root == NULL)
		{
			return;
		}

		std::string currentString = "";
		std::vector<Dice*> usedDice;			
		m_root->FindWords(current, result, currentString, usedDice);
	}

	TrieNode* m_root;
};

class Dictionary
{
public:
	Dictionary(std::string path)
	{
		std::ifstream fs;
		fs.open(path.c_str());

		m_words.reserve(40000);
		std::string current;
		current.reserve(100);
		while (std::getline(fs, current))
		{
			m_words.push_back(current);
		}
	}

	std::vector<std::string> m_words;
};
class Board
{
public:
	Board()
		:m_trie(NULL)
		, m_solutionIndex(0)
	{

	}
	void Init()
	{				
		for (int i = 0; i < NUM_ROWS * NUM_COLS; i++)
		{
			m_allDice.push_back(new Dice('A', 'A', 'E', 'E', 'G', 'N'));
			m_allDice.push_back(new Dice('E', 'L', 'R', 'T', 'T', 'Y'));
			m_allDice.push_back(new Dice('A', 'O', 'O', 'T', 'T', 'W'));
			m_allDice.push_back(new Dice('A', 'B', 'B', 'J', 'O', 'O'));
			m_allDice.push_back(new Dice('E', 'H', 'R', 'T', 'V', 'W'));
			m_allDice.push_back(new Dice('C', 'I', 'M', 'O', 'T', 'U'));
			m_allDice.push_back(new Dice('D', 'I', 'S', 'T', 'T', 'Y'));
			m_allDice.push_back(new Dice('E', 'I', 'O', 'S', 'S', 'T'));
			m_allDice.push_back(new Dice('D', 'E', 'L', 'R', 'V', 'Y'));
			m_allDice.push_back(new Dice('A', 'C', 'H', 'O', 'P', 'S'));
			m_allDice.push_back(new Dice('H', 'I', 'M', 'N', 'Q', 'U'));
			m_allDice.push_back(new Dice('E', 'E', 'I', 'N', 'S', 'U'));
			m_allDice.push_back(new Dice('E', 'E', 'G', 'H', 'N', 'W'));
			m_allDice.push_back(new Dice('A', 'F', 'F', 'K', 'P', 'S'));
			m_allDice.push_back(new Dice('H', 'L', 'N', 'N', 'R', 'Z'));
			m_allDice.push_back(new Dice('D', 'E', 'I', 'L', 'R', 'X'));
		}
		m_font.loadFromFile("Fonts/Elevation.ttf");
	}	

	void InitBoard()
	{
		sf::Clock clock;

		// Shuffle the dice
		std::shuffle(m_allDice.begin(), m_allDice.end(), *g_rand.m_gen);
		for (int i = 0; i < NUM_COLS; i++)
		{
			for (int j = 0; j <  NUM_ROWS; j++)
			{
				m_board[i][j] = m_allDice[i * NUM_ROWS + j];				
				m_board[i][j]->Roll();
			}
		}

		// Assign neighbors
		for (int i = 0; i < NUM_COLS; i++)
		{
			for (int j = 0; j < NUM_ROWS; j++)
			{
				Dice* current = m_board[i][j];
				current->m_neighbors.clear();

				// Four cardinal directions
				if (j - 1 >= 0)						current->m_neighbors.push_back(m_board[i][j - 1]);
				if (j + 1 < NUM_ROWS)				current->m_neighbors.push_back(m_board[i][j + 1]);
				if (i - 1 >= 0)						current->m_neighbors.push_back(m_board[i-1][j]);
				if (i + 1 < NUM_COLS)				current->m_neighbors.push_back(m_board[i+1][j]);

				// Four diagonal
				if (j - 1 >= 0 && i-1 >= 0)					current->m_neighbors.push_back(m_board[i-1][j - 1]);
				if (j + 1 < NUM_ROWS && i-1 >= 0)			current->m_neighbors.push_back(m_board[i-1][j + 1]);
				if (j - 1 >= 0 && i + 1 < NUM_ROWS)			current->m_neighbors.push_back(m_board[i+1][j - 1]);
				if (j + 1 < NUM_ROWS && i + 1 < NUM_ROWS)	current->m_neighbors.push_back(m_board[i+1][j + 1]);

			}
		}

		m_timeRemaining = GAME_LENGTH;
		m_drawSolution = false;
		Solve();
	}

	void Scroll(int amount)
	{
		m_solutionIndex += amount;
		if (m_solutionIndex < 0)
		{
			m_solutionIndex = 0;
		}
		else if (m_solutionIndex >= m_wordList.size())
		{
			m_solutionIndex = m_wordList.size() - 1;
		}
	}

	void Draw(sf::RenderWindow& window)
	{
		static const int margin = DICE_MARGIN;
		static const int size = DICE_SIZE;
		sf::Vector2f currentPos = m_origin;
		sf::Text currentText;
		currentText.setFont(m_font);
		currentText.setColor(sf::Color(50,50,255));		
		currentText.setCharacterSize(LETTER_SIZE);

		sf::Text wordText;
		wordText.setFont(m_font);
		wordText.setCharacterSize(20);
		wordText.setColor(sf::Color::White);

		sf::Text timeText;
		timeText.setFont(m_font);
		timeText.setColor(sf::Color::White);		
		for (int i = 0; i < NUM_COLS; i++)
		{
			for (int j = 0; j < NUM_ROWS; j++)
			{
				sf::RectangleShape shape(sf::Vector2f(size,size));
				shape.setPosition(currentPos);	
				shape.setOutlineColor(sf::Color(50, 50, 255));
				shape.setOutlineThickness(2);
				shape.setOrigin(size * 0.5f, size * 0.5f);
				shape.setFillColor(sf::Color::White);
				currentText.setString(m_board[i][j]->m_current);
				currentText.setPosition(currentPos + LETTER_ADJUSTMENT);
				currentText.setOrigin(sf::Vector2f(currentText.getGlobalBounds().width * 0.5f, currentText.getGlobalBounds().height * 0.5));
				window.draw(shape);
				window.draw(currentText);
				currentPos.x += size + margin;
			}
			currentPos.x = m_origin.x;
			currentPos.y += size + margin;
		}
		
		if (m_drawSolution)
		{
			for (int i = m_solutionIndex; i < m_wordList.size(); i++)
			{
				wordText.setString(m_wordList[i].c_str());
				wordText.setPosition(window.getSize().x - 350, m_origin.y - 50 + (i - m_solutionIndex) * 25);
				window.draw(wordText);
			}
		}

		std::ostringstream os;
		os << "TIME: " << std::setprecision(5) << m_timeRemaining;
		timeText.setPosition(window.getSize().x - 250, m_origin.y + NUM_ROWS * 0.5f * size);
		if (m_timeRemaining > 0)
		{
			timeText.setString(os.str());
		}
		else
		{
			timeText.setString("TIME'S UP!");
			timeText.setColor(sf::Color::Red);
		}
		window.draw(timeText);
	}

	void AddDiceToVisit(Dice* dice, std::vector<Dice*>& usedList, std::vector<Dice*>& visitList)
	{
		for (int i = 0; i < usedList.size(); i++)
		{
			if (dice == usedList[i])
			{
				return;
			}
		}

		visitList.push_back(dice);
	}

	void Solve()
	{		
		m_wordList.clear();
		for (int i = 0; i < NUM_COLS; i++)
		{
			for (int j = 0; j < NUM_ROWS; j++)
			{
				Dice* current = m_board[i][j];
				m_trie->FindWords(current, m_wordList);
			}			
		}

		std::sort(m_wordList.begin(), m_wordList.end(), [](std::string a, std::string b) { return a < b; });
	}
	const Trie* m_trie;
	float m_timeRemaining;
	int m_solutionIndex;
	bool m_drawSolution;
	sf::Font m_font;
	sf::Vector2f m_origin;
	std::vector<Dice*> m_allDice;
	Dice* m_board[NUM_ROWS][NUM_COLS];
	std::vector<std::string> m_wordList;
};

int main()
{
	sf::Clock clock;
	sf::Time timing = clock.restart();
	Dictionary dict("Words/words.txt");
	timing = clock.restart();
	printf("Time to populate dictionary [%dms]\n", timing.asMilliseconds());

	Trie trie(dict.m_words);
	timing = clock.restart();
	printf("Time to populate trie[%dms]\n", timing.asMilliseconds());

	Board boggleBoard;
	boggleBoard.m_trie = &trie;	
	
	boggleBoard.Init();	
	timing = clock.restart();
	printf("Time to init board [%dms]\n", timing.asMilliseconds());
	
	boggleBoard.InitBoard();
	timing = clock.restart();
	printf("Time to solve board [%dms]\n", timing.asMilliseconds());

	boggleBoard.m_origin = sf::Vector2f(100, 100);
	

	sf::RenderWindow window(sf::VideoMode(1920, 1080), "Welcome to SDVA 203!");
	sf::CircleShape shape(100.f);	
	shape.setFillColor(sf::Color::Green);
	while (window.isOpen())
	{
		sf::Time dt = clock.restart();
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Space)
				{
					if (!boggleBoard.m_drawSolution)
					{
						boggleBoard.m_drawSolution = true;
					}
					else
					{
						boggleBoard.InitBoard();
					}
				}
				if (event.key.code == sf::Keyboard::PageDown)
				{
					boggleBoard.Scroll(1);
				}
				if (event.key.code == sf::Keyboard::PageUp)
				{
					boggleBoard.Scroll(-1);
				}
			}
		}

		boggleBoard.m_timeRemaining -= dt.asSeconds();

		window.clear();
		boggleBoard.Draw(window);
		window.display();
	}

	return 0;
}

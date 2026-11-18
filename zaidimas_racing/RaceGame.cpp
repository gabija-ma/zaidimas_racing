#include "RaceGame.h"

#include <conio.h>
#include <windows.h>

#include <algorithm>
#include <fstream>
#include <iostream>

namespace
{
	void SetCursorPosition(short x, short y)
	{
		HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD position{ x, y };
		SetConsoleCursorPosition(output, position);
	}

	void ClearScreen()
	{
		HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(output, &info);

		DWORD written = 0;
		DWORD cellCount = static_cast<DWORD>(info.dwSize.X) * static_cast<DWORD>(info.dwSize.Y);
		FillConsoleOutputCharacterW(output, L' ', cellCount, { 0, 0 }, &written);
		FillConsoleOutputAttribute(output, info.wAttributes, cellCount, { 0, 0 }, &written);
		SetCursorPosition(0, 0);
	}
}

void RaceGame::Run()
{
	SetCursorVisible(false);
	LoadHighScore();

	while (running)
	{
		ShowMenu();
	}

	SetCursorVisible(true);
}

void RaceGame::SetCursorVisible(bool visible)
{
	HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursor;
	GetConsoleCursorInfo(output, &cursor);
	cursor.bVisible = visible;
	SetConsoleCursorInfo(output, &cursor);
}

void RaceGame::ShowMenu()
{
	ClearScreen();
	std::cout << "================ RACING =================\n\n";
	std::cout << "A / <-  - pavaziuoti i kaire\n";
	std::cout << "D / ->  - pavaziuoti i desine\n";
	std::cout << "P       - pauze\n";
	std::cout << "Esc     - isjungti zaidima\n\n";
	std::cout << "X       - kliutis, prarandi gyvybe\n";
	std::cout << "$       - moneta, +10 tasku\n\n";
	std::cout << "Geriausias rezultatas: " << highScore << "\n\n";
	std::cout << "Paspausk ENTER, kad pradetum...";

	while (true)
	{
		char key = _getch();
		if (key == '\r')
		{
			StartGame();
			return;
		}
		if (key == 27)
		{
			running = false;
			return;
		}
	}
}

void RaceGame::StartGame()
{
	playerLane = 1;
	lives = StartLives;
	score = 0;
	coins = 0;
	invulnerabilityFrames = 0;
	elapsedMove = std::chrono::milliseconds{0};
	elapsedSpawn = std::chrono::milliseconds{0};
	objects.clear();
	gameOver = false;
	paused = false;

	std::random_device device;
	random.seed(device());

	auto previousTime = std::chrono::steady_clock::now();

	while (running)
	{
		if (_kbhit())
		{
			char key = _getch();
			Update(key, std::chrono::milliseconds{0});

			if (gameOver)
			{
				while (running)
				{
					char answer = _getch();
					if (answer == '\r')
					{
						StartGame();
						return;
					}
					if (answer == 27)
					{
						running = false;
						return;
					}
				}
			}

			if (!running)
			{
				return;
			}
		}

		auto now = std::chrono::steady_clock::now();
		auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - previousTime);
		previousTime = now;

		if (!paused)
		{
			elapsedMove += frameTime;
			elapsedSpawn += frameTime;

			if (elapsedMove >= std::chrono::milliseconds(CurrentSpeed()))
			{
				for (Object& object : objects)
				{
					++object.row;
				}

				for (auto it = objects.begin(); it != objects.end();)
				{
					if (it->row >= BoardRows - 1)
					{
						if (it->lane == playerLane && invulnerabilityFrames == 0)
						{
							if (it->isCoin)
							{
								++coins;
								score += 10;
							}
							else
							{
								--lives;
								invulnerabilityFrames = 30;
								if (lives <= 0)
								{
									gameOver = true;
									if (score > highScore)
									{
										highScore = score;
										SaveHighScore();
									}
								}
							}
						}
						it = objects.erase(it);
					}
					else
					{
						++it;
					}
				}

				elapsedMove = std::chrono::milliseconds{0};
				if (invulnerabilityFrames > 0)
				{
					--invulnerabilityFrames;
				}
			}

			if (elapsedSpawn >= SpawnInterval())
			{
				SpawnObject();
				elapsedSpawn = std::chrono::milliseconds{0};
			}

			score += 1;
		}

		Draw();
		Sleep(20);

		if (gameOver)
		{
			Draw();
		}
	}
}

void RaceGame::Update(char key, std::chrono::milliseconds)
{
	if (key == 27)
	{
		running = false;
		return;
	}

	if (key == 'p' || key == 'P')
	{
		paused = !paused;
		return;
	}

	if (paused)
	{
		return;
	}

	if (key == 'a' || key == 'A' || key == -32 || static_cast<unsigned char>(key) == 224)
	{
		if (key == -32 || static_cast<unsigned char>(key) == 224)
		{
			char arrow = _getch();
			if (arrow == 'K')
			{
				MovePlayer(-1);
			}
			else if (arrow == 'M')
			{
				MovePlayer(1);
			}
		}
		else
		{
			MovePlayer(-1);
		}
	}
	else if (key == 'd' || key == 'D')
	{
		MovePlayer(1);
	}
}

bool RaceGame::ReadKey(char& key)
{
	if (!_kbhit())
	{
		return false;
	}

	key = _getch();
	return true;
}

void RaceGame::MovePlayer(int direction)
{
	playerLane = std::clamp(playerLane + direction, 0, LaneCount - 1);
}

void RaceGame::SpawnObject()
{
	std::uniform_int_distribution<int> laneDistribution(0, LaneCount - 1);
	std::uniform_int_distribution<int> coinDistribution(0, 4);

	Object object;
	object.lane = laneDistribution(random);
	object.row = 0;
	object.isCoin = coinDistribution(random) == 0;

	if (std::none_of(objects.begin(), objects.end(), [](const Object& item)
		{
			return item.row == 0;
		}))
	{
		objects.push_back(object);
	}
}

int RaceGame::CurrentSpeed() const
{
	int speed = 150 - score / 10 * 2;
	return std::clamp(speed, 70, 150);
}

std::chrono::milliseconds RaceGame::SpawnInterval() const
{
	int interval = 900 - score / 8 * 8;
	return std::chrono::milliseconds(std::clamp(interval, 420, 900));
}

void RaceGame::Draw()
{
	SetCursorPosition(0, 0);

	std::cout << "RACING  Taskai: " << score
		<< "  Monetos: " << coins
		<< "  Gyvybes: " << lives
		<< "  Geriausias: " << highScore << '\n';

	std::cout << "      Lenktinis: A/D arba rodykles\n";

	for (int row = 0; row < BoardRows; ++row)
	{
		std::cout << "  |";
		for (int lane = 0; lane < LaneCount; ++lane)
		{
			char symbol = ' ';

			if (lane == playerLane && row == BoardRows - 1)
			{
				symbol = invulnerabilityFrames > 0 ? '*' : 'A';
			}

			for (const Object& object : objects)
			{
				if (object.row == row && object.lane == lane)
				{
					symbol = object.isCoin ? '$' : 'X';
					break;
				}
			}

			std::cout << ' ' << symbol << " |";
		}
		std::cout << '\n';
	}

	if (paused)
	{
		std::cout << "\n          PAUZe - P, kad testi\n";
	}
	else if (gameOver)
	{
		std::cout << "\n          Zaidimas baigtas! Rezultatas: " << score << '\n';
		std::cout << "          ENTER - naujas zaidimas | ESC - iseiti\n";
	}
	else
	{
		std::cout << "\n          P - pauze | ESC - iseiti\n";
	}
}

void RaceGame::LoadHighScore()
{
	std::ifstream file("highscore.txt");
	if (file)
	{
		file >> highScore;
		if (!file || highScore < 0)
		{
			highScore = 0;
		}
	}
}

void RaceGame::SaveHighScore()
{
	std::ofstream file("highscore.txt", std::ios::trunc);
	if (file)
	{
		file << highScore;
	}
}

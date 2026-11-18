#pragma once

#include <chrono>
#include <random>
#include <string>
#include <vector>

struct Object
{
	int lane = 1;
	int row = 0;
	bool isCoin = false;
};

class RaceGame
{
public:
	void Run();

private:
	static constexpr int LaneCount = 3;
	static constexpr int BoardRows = 14;
	static constexpr int StartLives = 3;

	bool running = true;
	bool paused = false;
	bool gameOver = false;
	int playerLane = 1;
	int lives = StartLives;
	int score = 0;
	int coins = 0;
	int highScore = 0;
	int invulnerabilityFrames = 0;
	std::chrono::milliseconds elapsedMove{0};
	std::chrono::milliseconds elapsedSpawn{0};
	std::vector<Object> objects;
	std::mt19937 random;

	void ShowMenu();
	void StartGame();
	void Update(char key, std::chrono::milliseconds frameTime);
	void Draw();
	void SpawnObject();
	void LoadHighScore();
	void SaveHighScore();
	int CurrentSpeed() const;
	std::chrono::milliseconds SpawnInterval() const;
	void MovePlayer(int direction);
	bool ReadKey(char& key);
	void SetCursorVisible(bool visible);
};

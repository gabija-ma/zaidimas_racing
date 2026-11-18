#pragma once

#include <SFML/Graphics.hpp>
#include <random>
#include <string>
#include <vector>

class SFMLGame
{
public:
	void Run();

private:
	struct GameObject
	{
		int lane = 1;
		float y = -100.f;
		bool coin = false;
	};

	enum class State
	{
		Menu,
		Playing,
		Paused,
		GameOver
	};

	static constexpr int LaneCount = 3;
	static constexpr float LaneWidth = 200.f;
	static constexpr float RoadLeft = 100.f;
	static constexpr int StartLives = 3;

	sf::RenderWindow window;
	sf::Font font;
	sf::Texture carTexture;
	sf::Texture obstacleTexture;
	sf::Texture coinTexture;
	sf::Sprite carSprite;
	sf::Sprite obstacleSprite;
	sf::Sprite coinSprite;

	State state = State::Menu;
	std::mt19937 random;
	std::vector<GameObject> objects;
	sf::Clock clock;
	sf::Clock spawnClock;

	int playerLane = 1;
	int lives = StartLives;
	int score = 0;
	int coins = 0;
	int highScore = 0;
	float invulnerability = 0.f;

	void LoadResources();
	void StartGame();
	void ProcessEvents();
	void Update();
	void Draw();
	void SpawnObject();
	float LaneCenter(int lane) const;
	float ObjectSpeed() const;
	void LoadHighScore();
	void SaveHighScore();
};

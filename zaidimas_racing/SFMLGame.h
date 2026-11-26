#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <filesystem>
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
		std::size_t textureIndex = 0;
	};

	enum class State
	{
		Menu,
		Playing,
		Paused,
		GameOver
	};

	static constexpr int SoloLaneCount = 3;
	static constexpr int MultiplayerLaneCount = 6;
	static constexpr float LaneWidth = 200.f;
	static constexpr float PlayerCarY = 555.f;
	static constexpr float ObstacleWidth = 150.f;
	static constexpr float ObstacleHeight = 180.f;
	static constexpr float MinimumObjectGap = 320.f;
	static constexpr int StartLives = 3;

	struct Player
	{
		int lane = 1;
		int lives = StartLives;
		int score = 0;
		int coins = 0;
		float stunTimer = 0.f;
		float blinkTimer = 0.f;
		float invulnerability = 0.f;
	};

	enum class GameMode
	{
		Solo,
		Multiplayer
	};

	sf::RenderWindow window;
	sf::Font font;
	std::filesystem::path assetDirectory;
	std::vector<sf::Texture> carTextures;
	std::vector<sf::Texture> obstacleTextures;
	sf::Texture coinTexture;
	sf::Sprite carSprite;
	sf::Sprite obstacleSprite;
	sf::Sprite coinSprite;

	State state = State::Menu;
	GameMode gameMode = GameMode::Solo;
	std::mt19937 random;
	std::vector<GameObject> objects;
	sf::Clock clock;
	sf::Clock spawnClock;

	std::array<Player, 2> players{};
	std::array<std::size_t, 2> selectedCars{};
	std::array<int, 2> lastSpawnedLanes{ -1, -1 };
	int highScore = 0;
	float gameElapsedSeconds = 0.f;

	void LoadResources();
	void StartGame();
	void ProcessEvents();
	void Update();
	void Draw();
	void SpawnObject();
	void HandleObstacleHit(Player& player);
	int ActiveLaneCount() const;
	int GetPlayerLane(int playerIndex) const;
	float RoadLeft() const;
	float LaneCenter(int lane) const;
	float ObjectSpeed() const;
	float SpawnInterval() const;
	void LoadHighScore();
	void SaveHighScore();
};

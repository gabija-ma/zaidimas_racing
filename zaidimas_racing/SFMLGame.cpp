#define NOMINMAX
#include <Windows.h>

#include "SFMLGame.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

void SFMLGame::Run()
{
	window.create(sf::VideoMode(1400, 850), "Racing Game", sf::Style::Close);
	window.setFramerateLimit(60);
	LoadResources();
	LoadHighScore();

	while (window.isOpen())
	{
		ProcessEvents();
		Update();
		Draw();
	}
}

void SFMLGame::LoadResources()
{
	std::vector<std::filesystem::path> assetDirectories;
	assetDirectories.push_back(std::filesystem::current_path() / "assets");

	std::wstring executablePath(32768, L'\0');
	const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
	if (pathLength > 0 && pathLength < executablePath.size())
	{
		executablePath.resize(pathLength);
		const std::filesystem::path executableDirectory = std::filesystem::path(executablePath).parent_path();
		assetDirectories.push_back(executableDirectory / "assets");
		assetDirectories.push_back(executableDirectory.parent_path() / "zaidimas_racing" / "assets");
	}

	for (const auto& directory : assetDirectories)
	{
		if (std::filesystem::is_regular_file(directory / "ChildHood.ttf") &&
			std::filesystem::is_regular_file(directory / "car1.png") &&
			std::filesystem::is_regular_file(directory / "obs1.png"))
		{
			assetDirectory = directory;
			break;
		}
	}

	if (assetDirectory.empty())
	{
		throw std::runtime_error("Neradau assets aplanko su ChildHood.ttf, car1.png ir obs1.png. "
			"Patikrink zaidimas_racing\\assets aplanka.");
	}

	if (!font.loadFromFile((assetDirectory / "ChildHood.ttf").string()))
	{
		throw std::runtime_error("Nepavyko uzkrauti assets\\ChildHood.ttf srifto.");
	}
	if (!coinTexture.loadFromFile((assetDirectory / "coin.png").string()))
	{
		throw std::runtime_error("Nepavyko uzkrauti assets\\coin.png.");
	}

	for (std::size_t index = 1; ; ++index)
	{
		const auto file = assetDirectory / ("car" + std::to_string(index) + ".png");
		if (!std::filesystem::is_regular_file(file))
		{
			break;
		}
		carTextures.emplace_back();
		if (!carTextures.back().loadFromFile(file.string()))
		{
			throw std::runtime_error("Nepavyko uzkrauti " + file.string());
		}
	}

	for (std::size_t index = 1; ; ++index)
	{
		const auto file = assetDirectory / ("obs" + std::to_string(index) + ".png");
		if (!std::filesystem::is_regular_file(file))
		{
			break;
		}
		obstacleTextures.emplace_back();
		if (!obstacleTextures.back().loadFromFile(file.string()))
		{
			throw std::runtime_error("Nepavyko uzkrauti " + file.string());
		}
	}

	if (carTextures.empty() || obstacleTextures.empty())
	{
		throw std::runtime_error("Aplanke assets reikia bent car1.png ir obs1.png failu.");
	}
	if (!carTextures.empty())
	{
		carSprite.setTexture(carTextures.front());
	}
	obstacleSprite.setScale(0.55f, 0.55f);
	coinSprite.setTexture(coinTexture);
	const sf::Vector2u coinSize = coinTexture.getSize();
	const float coinScale = 110.f / static_cast<float>(std::max(coinSize.x, coinSize.y));
	coinSprite.setScale(coinScale, coinScale);

	random.seed(static_cast<unsigned>(std::random_device{}()));
}

void SFMLGame::LoadHighScore()
{
	std::ifstream input("highscore.txt");
	if (input)
	{
		input >> highScore;
		if (!input || highScore < 0)
		{
			highScore = 0;
		}
	}
}

void SFMLGame::SaveHighScore()
{
	std::ofstream output("highscore.txt", std::ios::trunc);
	if (output)
	{
		output << highScore;
	}
}

void SFMLGame::StartGame()
{
	state = State::Playing;
	players = {};
	players[0].lane = 1;
	players[1].lane = 4;
	lastSpawnedLanes = { -1, -1 };
	gameElapsedSeconds = 0.f;
	objects.clear();
	clock.restart();
	spawnClock.restart();
}

void SFMLGame::ProcessEvents()
{
	sf::Event event;
	while (window.pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
		{
			window.close();
		}

		if (event.type != sf::Event::KeyPressed)
		{
			continue;
		}

			if (event.key.code == sf::Keyboard::Escape)
			{
				if (state == State::Playing || state == State::Paused)
				{
					state = State::Menu;
				}
				else
				{
					window.close();
				}
			}
		else if (state == State::Menu)
			{
			if (event.key.code == sf::Keyboard::Enter)
			{
				StartGame();
			}
			else if (event.key.code == sf::Keyboard::Num1)
			{
				gameMode = GameMode::Solo;
			}
			else if (event.key.code == sf::Keyboard::Num2)
			{
				gameMode = GameMode::Multiplayer;
			}
			else if (event.key.code == sf::Keyboard::A)
			{
				selectedCars[0] = (selectedCars[0] + carTextures.size() - 1) % carTextures.size();
			}
			else if (event.key.code == sf::Keyboard::D)
			{
				selectedCars[0] = (selectedCars[0] + 1) % carTextures.size();
			}
			else if (gameMode == GameMode::Multiplayer && event.key.code == sf::Keyboard::Left)
			{
				selectedCars[1] = (selectedCars[1] + carTextures.size() - 1) % carTextures.size();
			}
			else if (gameMode == GameMode::Multiplayer && event.key.code == sf::Keyboard::Right)
			{
				selectedCars[1] = (selectedCars[1] + 1) % carTextures.size();
			}
		}
			else if (state == State::GameOver && event.key.code == sf::Keyboard::Enter)
			{
				StartGame();
			}
			else if (state == State::Playing)
			{
			if (event.key.code == sf::Keyboard::P)
				{
				state = State::Paused;
				continue;
				}

			if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::D)
			{
				Player& player = players[0];
				if (player.lives > 0 && player.stunTimer <= 0.f)
				{
					player.lane += event.key.code == sf::Keyboard::A ? -1 : 1;
					player.lane = std::clamp(player.lane, 0, SoloLaneCount - 1);
				}
				}
			else if (gameMode == GameMode::Multiplayer &&
				(event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::Right))
			{
				Player& player = players[1];
				if (player.lives > 0 && player.stunTimer <= 0.f)
				{
					player.lane += event.key.code == sf::Keyboard::Left ? -1 : 1;
					player.lane = std::clamp(player.lane, SoloLaneCount, MultiplayerLaneCount - 1);
				}
				}
			}
			else if (state == State::Paused && event.key.code == sf::Keyboard::P)
			{
				state = State::Playing;
			}
	}
}

int SFMLGame::ActiveLaneCount() const
{
	return gameMode == GameMode::Multiplayer ? MultiplayerLaneCount : SoloLaneCount;
		}

int SFMLGame::GetPlayerLane(int playerIndex) const
{
	return players[playerIndex].lane;
	}

float SFMLGame::RoadLeft() const
{
	return (static_cast<float>(window.getSize().x) - LaneWidth * ActiveLaneCount()) * 0.5f;
}

float SFMLGame::LaneCenter(int lane) const
{
	return RoadLeft() + LaneWidth * (lane + 0.5f);
}

float SFMLGame::ObjectSpeed() const
{
	const int speedLevel = static_cast<int>(gameElapsedSeconds / 8.f);
	return std::min(700.f, 220.f + speedLevel * 40.f);
}

float SFMLGame::SpawnInterval() const
{
	const int speedLevel = static_cast<int>(gameElapsedSeconds / 8.f);
	return std::min(2.5f, 1.1f + speedLevel * 0.2f);
}

void SFMLGame::SpawnObject()
{
	std::uniform_int_distribution<int> kind(0, 4);
	std::uniform_int_distribution<std::size_t> obstacle(0, obstacleTextures.size() - 1);
	const int playerCount = gameMode == GameMode::Multiplayer ? 2 : 1;

	for (int playerIndex = 0; playerIndex < playerCount; ++playerIndex)
	{
		const int firstLane = playerIndex == 0 ? 0 : SoloLaneCount;
		const int lastLane = playerIndex == 0 ? SoloLaneCount - 1 : MultiplayerLaneCount - 1;
	GameObject object;
		if (lastSpawnedLanes[playerIndex] >= firstLane && lastSpawnedLanes[playerIndex] <= lastLane)
		{
			std::uniform_int_distribution<int> laneOffset(0, lastLane - firstLane - 1);
			object.lane = firstLane + laneOffset(random);
			if (object.lane >= lastSpawnedLanes[playerIndex])
			{
				++object.lane;
			}
		}
		else
		{
			std::uniform_int_distribution<int> lane(firstLane, lastLane);
	object.lane = lane(random);
		}
	object.y = -150.f;
	object.coin = kind(random) == 0;
		if (!object.coin)
	{
			object.textureIndex = obstacle(random);
		}
		objects.push_back(object);
		lastSpawnedLanes[playerIndex] = object.lane;
	}
	}

void SFMLGame::HandleObstacleHit(Player& player)
{
	--player.lives;
	player.stunTimer = 0.45f;
	player.blinkTimer = 1.2f;
	player.invulnerability = 1.2f;
}

void SFMLGame::Update()
{
	if (state != State::Playing)
	{
		return;
	}

	const float elapsed = std::min(clock.restart().asSeconds(), 0.1f);
	gameElapsedSeconds += elapsed;
	for (Player& player : players)
	{
		player.stunTimer = std::max(0.f, player.stunTimer - elapsed);
		player.blinkTimer = std::max(0.f, player.blinkTimer - elapsed);
		player.invulnerability = std::max(0.f, player.invulnerability - elapsed);
	}

	for (GameObject& object : objects)
	{
		object.y += ObjectSpeed() * elapsed;
	}

	for (auto it = objects.begin(); it != objects.end();)
	{
		bool objectCollectedOrHit = false;
		for (int playerIndex = 0; playerIndex < (gameMode == GameMode::Multiplayer ? 2 : 1); ++playerIndex)
		{
			Player& player = players[playerIndex];
			if (player.lives <= 0 || player.lane != it->lane)
		{
				continue;
			}

			const float noseX = LaneCenter(player.lane);
			const float noseY = PlayerCarY;
			bool noseTouchedObject = false;
			if (it->coin)
			{
				const float coinRadius = coinSprite.getGlobalBounds().width * 0.5f;
				const float dx = noseX - LaneCenter(it->lane);
				const float dy = noseY - (it->y + coinRadius);
				noseTouchedObject = dx * dx + dy * dy <= coinRadius * coinRadius;
			}
			else if (player.invulnerability <= 0.f)
			{
				const float obstacleLeft = LaneCenter(it->lane) - ObstacleWidth * 0.5f;
				noseTouchedObject = noseX >= obstacleLeft && noseX <= obstacleLeft + ObstacleWidth &&
					noseY >= it->y && noseY <= it->y + ObstacleHeight;
			}

			if (noseTouchedObject)
				{
				if (it->coin)
					{
					++player.coins;
					player.score += 10;
					}
				else
				{
					HandleObstacleHit(player);
				}
				objectCollectedOrHit = true;
				break;
			}
		}

		if (objectCollectedOrHit || it->y > 850.f)
		{
			it = objects.erase(it);
		}
		else
		{
			++it;
		}
	}

	if (spawnClock.getElapsedTime().asSeconds() >= SpawnInterval() &&
		std::none_of(objects.begin(), objects.end(), [](const GameObject& object)
		{
			return object.y < MinimumObjectGap;
		}))
	{
		SpawnObject();
		spawnClock.restart();
	}

	for (Player& player : players)
	{
		if (player.lives > 0)
		{
			++player.score;
			highScore = std::max(highScore, player.score);
		}
	}

	const bool gameFinished = gameMode == GameMode::Solo
		? players[0].lives <= 0
		: players[0].lives <= 0 && players[1].lives <= 0;
	if (gameFinished)
	{
		state = State::GameOver;
		SaveHighScore();
	}
}

void SFMLGame::Draw()
{
	window.clear(sf::Color(18, 24, 38));

	const float roadLeft = RoadLeft();
	const float roadWidth = LaneWidth * ActiveLaneCount();
	sf::RectangleShape road(sf::Vector2f(roadWidth, static_cast<float>(window.getSize().y)));
	road.setPosition(roadLeft, 0.f);
	road.setFillColor(sf::Color(55, 55, 65));
	window.draw(road);

	for (int lane = 1; lane < ActiveLaneCount(); ++lane)
	{
		sf::RectangleShape divider(sf::Vector2f(lane == SoloLaneCount && gameMode == GameMode::Multiplayer ? 10.f : 5.f,
			static_cast<float>(window.getSize().y)));
		divider.setPosition(roadLeft + LaneWidth * lane, 0.f);
		divider.setFillColor(lane == SoloLaneCount && gameMode == GameMode::Multiplayer
			? sf::Color(245, 190, 65) : sf::Color(220, 220, 220));
		window.draw(divider);
	}

	sf::Text text;
	text.setFont(font);
	text.setCharacterSize(23);
	text.setFillColor(sf::Color::White);

	if (state == State::Playing || state == State::Paused)
	{
		std::ostringstream firstPlayerStatus;
		firstPlayerStatus << "P1  Score: " << players[0].score << "  Coins: " << players[0].coins
			<< "  Lives: " << players[0].lives;
		text.setString(firstPlayerStatus.str());
		text.setPosition(20.f, 10.f);
		window.draw(text);

		if (gameMode == GameMode::Multiplayer)
		{
			std::ostringstream secondPlayerStatus;
			secondPlayerStatus << "P2  Score: " << players[1].score << "  Coins: " << players[1].coins
				<< "  Lives: " << players[1].lives;
			text.setString(secondPlayerStatus.str());
			text.setPosition(780.f, 10.f);
			window.draw(text);
		}
	}

	for (const GameObject& object : objects)
	{
		if (object.coin)
		{
			coinSprite.setPosition(LaneCenter(object.lane) - coinSprite.getGlobalBounds().width / 2.f, object.y);
			window.draw(coinSprite);
		}
		else
		{
			obstacleSprite.setTexture(obstacleTextures[object.textureIndex]);
			const sf::FloatRect bounds = obstacleSprite.getLocalBounds();
			obstacleSprite.setScale(ObstacleWidth / bounds.width, ObstacleHeight / bounds.height);
			obstacleSprite.setPosition(LaneCenter(object.lane) - obstacleSprite.getGlobalBounds().width / 2.f, object.y);
			window.draw(obstacleSprite);
		}
	}

	const int playerCount = gameMode == GameMode::Multiplayer ? 2 : 1;
	for (int playerIndex = 0; playerIndex < playerCount; ++playerIndex)
	{
		const Player& player = players[playerIndex];
		if (player.lives <= 0 || (player.blinkTimer > 0.f && static_cast<int>(player.blinkTimer * 12.f) % 2 == 0))
		{
			continue;
	}

		carSprite.setTexture(carTextures[selectedCars[playerIndex]]);
		const sf::FloatRect carBounds = carSprite.getLocalBounds();
		const float carScale = std::min(145.f / carBounds.width, 225.f / carBounds.height);
		carSprite.setScale(carScale, carScale);
		carSprite.setPosition(LaneCenter(GetPlayerLane(playerIndex)) - carSprite.getGlobalBounds().width / 2.f, PlayerCarY);
		carSprite.setColor(sf::Color::White);
	window.draw(carSprite);
	}

	if (state == State::Menu)
	{
		sf::RectangleShape panel(sf::Vector2f(920.f, 610.f));
		panel.setPosition(240.f, 115.f);
		panel.setFillColor(sf::Color(10, 10, 20, 235));
		window.draw(panel);

		text.setCharacterSize(46);
		text.setPosition(520.f, 155.f);
		text.setString("RACING GAME");
		window.draw(text);

		text.setCharacterSize(24);
		text.setPosition(410.f, 235.f);
		text.setString("1 - vienas zaidejas     2 - du zaidejai");
		window.draw(text);

		text.setPosition(410.f, 280.f);
		text.setString("Pasirinktas rezimas: " + std::string(gameMode == GameMode::Solo ? "SOLO" : "MULTIPLAYER"));
		window.draw(text);

		text.setCharacterSize(22);
		text.setPosition(390.f, 340.f);
		text.setString("P1 pasirinkimas (A/D): masina " + std::to_string(selectedCars[0] + 1) +
			" / " + std::to_string(carTextures.size()));
		window.draw(text);

		carSprite.setTexture(carTextures[selectedCars[0]]);
		sf::FloatRect previewBounds = carSprite.getLocalBounds();
		float previewScale = std::min(100.f / previewBounds.width, 150.f / previewBounds.height);
		carSprite.setScale(previewScale, previewScale);
		carSprite.setPosition(520.f, 385.f);
		carSprite.setColor(sf::Color::White);
		window.draw(carSprite);

		if (gameMode == GameMode::Multiplayer)
		{
			text.setPosition(760.f, 340.f);
			text.setString("P2 pasirinkimas (rodykles): masina " + std::to_string(selectedCars[1] + 1) +
				" / " + std::to_string(carTextures.size()));
			window.draw(text);

			carSprite.setTexture(carTextures[selectedCars[1]]);
			previewBounds = carSprite.getLocalBounds();
			previewScale = std::min(100.f / previewBounds.width, 150.f / previewBounds.height);
			carSprite.setScale(previewScale, previewScale);
			carSprite.setPosition(930.f, 385.f);
			window.draw(carSprite);
		}

		text.setCharacterSize(22);
		text.setPosition(490.f, 600.f);
		text.setString("P1: A/D   |   P2: rodykles\nENTER - pradeti   |   ESC - iseiti");
		window.draw(text);
	}
	else if (state == State::Paused)
	{
		sf::RectangleShape panel(sf::Vector2f(420.f, 150.f));
		panel.setPosition((window.getSize().x - 420.f) * 0.5f, 340.f);
		panel.setFillColor(sf::Color(10, 10, 20, 230));
		window.draw(panel);
		text.setCharacterSize(46);
		text.setPosition(window.getSize().x * 0.5f - 85.f, 380.f);
		text.setString("PAUZE - P");
		window.draw(text);
	}
	else if (state == State::GameOver)
	{
		sf::RectangleShape panel(sf::Vector2f(760.f, 390.f));
		panel.setPosition(320.f, 220.f);
		panel.setFillColor(sf::Color(10, 10, 20, 240));
		window.draw(panel);

		text.setCharacterSize(42);
		text.setPosition(465.f, 255.f);
		text.setString("ZAIDIMAS BAIGTAS");
		window.draw(text);

		text.setCharacterSize(28);
		if (gameMode == GameMode::Multiplayer)
		{
			const std::string result = players[0].score == players[1].score
				? "Lygiosios!"
				: (players[0].score > players[1].score ? "Laimetoja P1!" : "Laimetoja P2!");
			text.setPosition(545.f, 335.f);
			text.setString(result);
			window.draw(text);

			text.setCharacterSize(24);
			text.setPosition(440.f, 400.f);
			text.setString("P1 taskai: " + std::to_string(players[0].score) +
				"     P2 taskai: " + std::to_string(players[1].score));
			window.draw(text);
		}
		else
		{
			text.setPosition(530.f, 370.f);
			text.setString("Tavo rezultatas: " + std::to_string(players[0].score));
			window.draw(text);
		}

		text.setCharacterSize(23);
		text.setPosition(465.f, 485.f);
		text.setString("ENTER - zaisti dar karta   |   ESC - meniu");
		window.draw(text);
	}

	window.display();
}

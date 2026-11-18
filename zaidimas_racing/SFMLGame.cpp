#include "SFMLGame.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

void SFMLGame::Run()
{
	window.create(sf::VideoMode(800, 800), "Racing Game", sf::Style::Close);
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
	font.loadFromFile("assets/tuffy.ttf");
	carTexture.loadFromFile("assets/car.png");
	obstacleTexture.loadFromFile("assets/obstacle.png");
	coinTexture.loadFromFile("assets/coin.png");

	carSprite.setTexture(carTexture);
	carSprite.setScale(0.9f, 0.9f);
	obstacleSprite.setTexture(obstacleTexture);
	obstacleSprite.setScale(0.9f, 0.9f);
	coinSprite.setTexture(coinTexture);
	coinSprite.setScale(0.75f, 0.75f);

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
	playerLane = 1;
	lives = StartLives;
	score = 0;
	coins = 0;
	invulnerability = 0.f;
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

		if (event.type == sf::Event::KeyPressed)
		{
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
			else if (state == State::Menu && event.key.code == sf::Keyboard::Enter)
			{
				StartGame();
			}
			else if (state == State::GameOver && event.key.code == sf::Keyboard::Enter)
			{
				StartGame();
			}
			else if (state == State::Playing)
			{
				if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left)
				{
					playerLane = std::max(0, playerLane - 1);
				}
				else if (event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right)
				{
					playerLane = std::min(LaneCount - 1, playerLane + 1);
				}
				else if (event.key.code == sf::Keyboard::P)
				{
					state = State::Paused;
				}
			}
			else if (state == State::Paused && event.key.code == sf::Keyboard::P)
			{
				state = State::Playing;
			}
		}
	}
}

float SFMLGame::LaneCenter(int lane) const
{
	return RoadLeft + LaneWidth * (lane + 0.5f);
}

float SFMLGame::ObjectSpeed() const
{
	return std::min(520.f, 230.f + score * 0.08f);
}

void SFMLGame::SpawnObject()
{
	std::uniform_int_distribution<int> lane(0, LaneCount - 1);
	std::uniform_int_distribution<int> kind(0, 4);

	GameObject object;
	object.lane = lane(random);
	object.y = -150.f;
	object.coin = kind(random) == 0;

	if (std::none_of(objects.begin(), objects.end(), [](const GameObject& item)
		{
			return item.y < 40.f;
		}))
	{
		objects.push_back(object);
	}
}

void SFMLGame::Update()
{
	if (state != State::Playing)
	{
		return;
	}

	float elapsed = clock.restart().asSeconds();
	if (invulnerability > 0.f)
	{
		invulnerability -= elapsed;
	}

	for (GameObject& object : objects)
	{
		object.y += ObjectSpeed() * elapsed;
	}

	for (auto it = objects.begin(); it != objects.end();)
	{
		if (it->y > 710.f && it->lane == playerLane)
		{
			if (it->coin)
			{
				++coins;
				score += 10;
			}
			else if (invulnerability <= 0.f)
			{
				--lives;
				invulnerability = 1.2f;
				if (lives <= 0)
				{
					state = State::GameOver;
					if (score > highScore)
					{
						highScore = score;
						SaveHighScore();
					}
				}
			}
			it = objects.erase(it);
		}
		else if (it->y > 850.f)
		{
			it = objects.erase(it);
		}
		else
		{
			++it;
		}
	}

	if (spawnClock.getElapsedTime().asSeconds() > std::max(0.38f, 0.95f - score * 0.001f))
	{
		SpawnObject();
		spawnClock.restart();
	}

	score += 1;
}

void SFMLGame::Draw()
{
	window.clear(sf::Color(18, 24, 38));

	sf::RectangleShape road(sf::Vector2f(600.f, 800.f));
	road.setPosition(RoadLeft, 0.f);
	road.setFillColor(sf::Color(55, 55, 65));
	window.draw(road);

	for (int lane = 0; lane < LaneCount - 1; ++lane)
	{
		sf::RectangleShape divider(sf::Vector2f(8.f, 800.f));
		divider.setPosition(RoadLeft + LaneWidth * (lane + 1), 0.f);
		divider.setFillColor(sf::Color(220, 220, 220));
		window.draw(divider);
	}

	sf::Text title;
	title.setFont(font);
	title.setCharacterSize(26);
	title.setFillColor(sf::Color::White);

	std::ostringstream status;
	status << "Score: " << score << "  Coins: " << coins << "  Lives: " << lives;
	title.setString(status.str());
	title.setPosition(20.f, 10.f);
	window.draw(title);

	for (const GameObject& object : objects)
	{
		sf::Sprite& sprite = object.coin ? coinSprite : obstacleSprite;
		sprite.setPosition(LaneCenter(object.lane) - sprite.getGlobalBounds().width / 2.f, object.y);
		window.draw(sprite);
	}

	carSprite.setPosition(LaneCenter(playerLane) - carSprite.getGlobalBounds().width / 2.f, 640.f);
	carSprite.setColor(invulnerability > 0.f ? sf::Color(255, 255, 255, 100) : sf::Color::White);
	window.draw(carSprite);

	if (state == State::Menu)
	{
		sf::RectangleShape panel(sf::Vector2f(620.f, 340.f));
		panel.setPosition(90.f, 180.f);
		panel.setFillColor(sf::Color(10, 10, 20, 230));
		window.draw(panel);

		title.setCharacterSize(42);
		title.setPosition(230.f, 220.f);
		title.setString("RACING GAME");
		window.draw(title);

		title.setCharacterSize(24);
		title.setPosition(180.f, 310.f);
		title.setString("A/D arba < > - valdymas\nP - pauze\nEsc - atsijungti");
		window.draw(title);

		title.setPosition(210.f, 450.f);
		title.setString("ENTER - pradeti zaidima");
		window.draw(title);
	}
	else if (state == State::Paused)
	{
		title.setCharacterSize(46);
		title.setPosition(290.f, 350.f);
		title.setString("PAUZE");
		window.draw(title);
	}
	else if (state == State::GameOver)
	{
		sf::RectangleShape panel(sf::Vector2f(620.f, 320.f));
		panel.setPosition(90.f, 220.f);
		panel.setFillColor(sf::Color(10, 10, 20, 230));
		window.draw(panel);

		title.setCharacterSize(42);
		title.setPosition(230.f, 260.f);
		title.setString("ZAIDIMAS BAIGTAS");
		window.draw(title);

		title.setCharacterSize(26);
		title.setPosition(230.f, 350.f);
		title.setString("Rezultatas: " + std::to_string(score));
		window.draw(title);

		title.setPosition(190.f, 420.f);
		title.setString("ENTER - kartoti | ESC - menu");
		window.draw(title);
	}

	window.display();
}

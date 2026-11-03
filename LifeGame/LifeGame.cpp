#include <iostream>
#include <vector>
#include <utility>
#include <random>
#include <chrono>
#include <thread>
#include <conio.h>
#include <fstream>

enum class BoundaryMode {
	Bounded, //Мертвые края
	Toroidal //Тор / бублик / один край является продолжением противоположного
};

//Класс "Поле" (модель). 
//Отвечает за данные и математику игры
class Field {
private:
	int width;
	int height;
	std::vector<uint8_t> current_field;
	std::vector<uint8_t> next_field;
	BoundaryMode boundary_mode;
	std::mt19937 rng;

public:
	Field(int w, int h, BoundaryMode mode = BoundaryMode::Bounded)
		: width(w),
		height(h),
		boundary_mode(mode),
		current_field(w* h, 0),
		next_field(w* h, 0),
		rng(std::random_device{}()) {}
		
	//Геттеры ширины и высоты

	//Получить ширину поля
	int getWidth() const { return width; }
	//Получить высоту поля
	int getHeight() const { return height; }

	//Проверяет: живая ли клетка сейчас?
	bool isCellAlive(int x, int y) const {
		return current_field[y * width + x] == 1;
	}

	//Изменяет клетку
	void setCell(int x, int y, uint8_t value) {
		current_field[y * width + x] = value;
	}

	//Изменяет диапазон клеток
	void setCells(int start_x, int start_y, int end_x, int end_y, uint8_t value) {
		for (int y = start_y; y <= end_y; y++) {
			for (int x = start_x; x <= end_x; x++) {
				setCell(x, y, value);
			}
		}
	}

	//Сохраняет паттерн в файл
	bool savePatternToFile(const std::string& name) {
		std::string filename = name + ".lfgm";
		std::ofstream stream(filename);
		if (stream.is_open()) {
			std::string line;
			for (int y = 0; y < height; ++y) {
				line = "";
				for (int x = 0; x < width; ++x) {
					if (isCellAlive(x, y))
						line += "O";
					else
						line += ".";
				}
				stream << line << '\n';
			}
			return true;
		}
		return false;
	}

	//Инициализация паттернов

	//Загружает паттерн из файла
	bool loadPatternFromFile(const std::string& filename) {
		std::string line;
		std::ifstream stream(filename);

		if (stream.is_open()) {
			std::vector<std::string> pattern;
			int pattern_width = 0, pattern_height = 0;
			while (std::getline(stream, line)) {
				if (line.empty() || line[0] == '#')
					continue;
				pattern_height++;
				if (line.length() > pattern_width)
					pattern_width = line.length();
				pattern.push_back(line);
			}

			int start_x = ((width - pattern_width) / 2 >= 0) ? ((width - pattern_width) / 2) : 0;
			int start_y = ((height - pattern_height) / 2 >= 0) ? ((height - pattern_height) / 2) : 0;

			for (int y = 0; y < pattern.size() && y < height; ++y) {
				for (int x = 0; x < pattern[y].length() && x < width; ++x) {
					if (pattern[y][x] == 'O') {
						setCell(start_x + x, start_y + y, 1);
					}
				}
			}
			return true;
		}
		return false;
	}

	//Стартовая "Мигалка"
	void initializeBlinker() {
		if (width >= 5 && height >= 5) {
			setCells(1, 1, 1, 3, 1);
		}
	}

	//Стартовый "Глайдер"
	void initializeGlider() {
		if (width >= 5 && height >= 5) {
			setCell(1, 0, 1);
			setCell(2, 1, 1);
			setCell(0, 2, 1);
			setCell(1, 2, 1);
			setCell(2, 2, 1);
		}
	}

	//Стартовый рандом
	void initializeRandom(float probability = 0.25f) {
		std::uniform_real_distribution<> dis(0.0, 1.0);
		for (size_t i = 0; i < current_field.size(); ++i) {
			current_field[i] = (dis(rng) < probability) ? 1 : 0;
		}
	}

	//Пустое поле
	void initializeClear() {
		std::fill(current_field.begin(), current_field.end(), 0);
	}

	//Логика игры

	//Расчет следующего поколения
	void nextGeneration() {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				next_field[y * width + x] = predictCell(x, y);
			}
		}
		std::swap(current_field, next_field);
		std::fill(next_field.begin(), next_field.end(), 0);
	}

private:

	//Рассчитывает клетку на следующем поколении
	uint8_t predictCell(int x, int y) const {
		int alive_counter = countAliveNeighbors(x, y, boundary_mode);
		bool is_alive = isCellAlive(x, y);

		if (is_alive && (alive_counter == 2 || alive_counter == 3)) return 1; //Правило выживания
		if (!is_alive && alive_counter == 3) return 1; //Правило размножения

		return 0; //Все остальные случаи
	}

	//Проверка соседей
	int countAliveNeighbors(int x, int y, BoundaryMode mode = BoundaryMode::Bounded) const {
		int alive_counter = 0;
		for (int dx = -1; dx <= 1; dx++) {
			for (int dy = -1; dy <= 1; dy++) {
				if (dx == 0 && dy == 0)
					continue;

				int nx = x + dx;
				int ny = y + dy;

				if (mode == BoundaryMode::Bounded) {
					if ((nx >= 0 && nx < width) && (ny >= 0 && ny < height)) {
						if (isCellAlive(nx, ny))
							alive_counter++;
					}
				}
				else {
					nx = (nx % width + width) % width;
					ny = (ny % height + height) % height;

					if (isCellAlive(nx, ny))
						alive_counter++;
				}
			}
		}

		return alive_counter;
	}
};

//Класс "Игра" (Контроллер/Представление)
//Отвечает за взаимодействие с пользователем, отрисовку и цикл игры
class Game {
private:
	Field field;
	bool is_auto_mode = false; //Флаг, определяющий режим отрисовки

	void clearScreen() const {
		// \033[2J очищает экран, \033[H перемещает курсор в левый верхний угол
		std::cout << "\033[2J\033[H";
	}
	
public:
	Game(int width, int height, BoundaryMode mode = BoundaryMode::Bounded)
		: field(width, height, mode){}

	//Рисует поле
	void draw() const {
		for (int y = 0; y < field.getHeight(); ++y) {
			for (int x = 0; x < field.getWidth(); ++x) {
				if (field.isCellAlive(x, y)) {
					std::cout << "\033[32m#\033[0m "; //Живая клетка
				}
				else {
					std::cout << "\033[90m-\033[0m "; //Мертвая клетка
				}
			}
			std::cout << "\n";
		}
	}

	//Основной цикл игры
	void run() {
		//Начальная настройка
		field.initializeClear();

		while (true) {
			if (is_auto_mode) {
				clearScreen();
				draw();
				std::cout << "Controls:\t[Esc/q] - exit, [Space/p] - pause, [r] - random, [c] - clear\n";

				field.nextGeneration();

				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				if (_kbhit()) {
					char key = _getch();
					if (key == 'p' || key == ' ') {
						is_auto_mode = false;
					}
					else if (key == 'q' || key == 27) //key == 27 - ESC
						return;
					else if (key == 'r') 
						field.initializeRandom(0.25f); 
					else if (key == 'c') 
						field.initializeClear();
				}
			}
			else {
				clearScreen();
				draw();

				std::cout << "Controls:\t[Enter] - next step, [q] - exit, [a] - auto mode\n";
				std::cout << "Patterns:\t[r] - random seed, [g] - glider, [b] - blinker, [c] - clear, [l] - load user .lfgm, [s] - save to .lfgm\n";
				std::cout << "Reset:\t\t[1] - Bounded mode, [2] - Toroidal mode\n> ";
				std::string input;
				std::getline(std::cin, input);

				if (input == "q") {
					break;
				}
				else if (input == "a") {
					is_auto_mode = true;
					continue;
				}
				else if (input == "r") {
					field.initializeRandom(0.25f);
					continue;
				}
				else if (input == "g") {
					field.initializeGlider();
					continue;
				}
				else if (input == "b") {
					field.initializeBlinker();
					continue;
				}
				else if (input == "c") {
					field.initializeClear();
					continue;
				}
				else if (input == "l") {
					std::cout << "Input file name (example: glider.lfgm)\n> ";
					std::string filename;
					std::getline(std::cin, filename);

					if (field.loadPatternFromFile(filename)){
						std::cout << "Pattern loaded successfully!\n";
					}
					else{
						std::cout << "Error: file doesn't exist or corrupted!\n";
					}
					std::cout << "Press any key to continue...\n";
					_getch();
					continue;
				}
				else if (input == "s") {
					std::cout << "Input pattern name\n> ";
					std::string name;
					std::getline(std::cin, name);

					if (field.savePatternToFile(name)) {
						std::cout << "Pattern saved successfully!\n";
					}
					else {
						std::cout << "Error while saving the pattern!\n";
					}
					std::cout << "Press any key to continue...\n";
					_getch();
					continue;
				}
				else if (input == "1") {
					field = Field(field.getWidth(), field.getHeight(), BoundaryMode::Bounded);
					field.initializeRandom(0.25f);
					continue;
				}
				else if (input == "2") {
					field = Field(field.getWidth(), field.getHeight(), BoundaryMode::Toroidal);
					field.initializeRandom(0.25f);
					continue;
				}

				field.nextGeneration();
			}
		}
	}
};

int main()
{
	int w; int h;
	std::cout << "Input field width: ";
	std::cin >> w;
	std::cout << "Input field height: ";
	std::cin >> h;
	std::cin.ignore();
	Game game(w, h, BoundaryMode::Bounded);
	game.run();

	return 0;
}
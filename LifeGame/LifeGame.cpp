#include <iostream>
#include <vector>

class Field {
private:
	int width;
	int height;
	std::vector<uint8_t> field;
	std::vector<uint8_t> next_gen_field;
public:
	Field(int w, int h) {
		width = w;
		height = h;
		this->field = std::vector<uint8_t>(w * h, 0);
	}

	void draw_base_field() {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				std::cout << (int)field[j * width + i] << " ";
			}
			std::cout << "\n";
		}
	}

	void initialize_middle_life_cell() {
		field[height / 2 * width + height / 2] = 1;
	}

	void count_next_gen() {

	}

	uint8_t new_cell(int x, int y) {

	}
	bool is_alone();
	bool is_survival();
	bool is_full();
	bool is_birth();
	bool is_edge();
};

int main()
{
	Field game = Field(3, 3);
	game.initialize_middle_life_cell();
	game.draw_base_field();
}
#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

    void load_assets();
    void setup();
    void generate_fly();

    // Const game variables
    static constexpr float player_speed = 40.0f;

	//----- game state -----

    glm::vec2 fly;
    glm::vec2 fly_speed;

    glm::vec2 player;

    glm::vec2 paddle;

    bool paddle_exists;

    int player_dir = 0;
    float paddle_countdown;

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};

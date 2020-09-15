#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>

#include "convert_assets.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"
#include "Load.hpp"

PlayMode::PlayMode() {
    load_assets();

    setup();
}

PlayMode::~PlayMode() {
}

void PlayMode::load_assets() {
    int tile_index = 0;

    // Load each hardcoded asset
    for(uint8_t i = 0; i < asset_names.size(); i++) {
        // Read file
        std::vector<uint8_t> raw_data;
        std::string asset_loc = data_path("assets/" + asset_names[i] + ASSET_EXTENSION);
        std::ifstream asset_file(asset_loc, std::ios::binary);
        read_chunk(asset_file, MAGIC, &raw_data);
        asset_file.close();

        uint32_t data_idx = 0;

        // Load palette
        for(int j = 0; j < 4; j++) {
            ppu.palette_table[i][j].r = raw_data[data_idx++];
            ppu.palette_table[i][j].g = raw_data[data_idx++];
            ppu.palette_table[i][j].b = raw_data[data_idx++];
            ppu.palette_table[i][j].a = raw_data[data_idx++];
        }

        while(data_idx < raw_data.size()) {
            for(int j = 0; j < 8; j++) {
                ppu.tile_table[tile_index].bit0[j] = raw_data[data_idx++];
            }
            for(int j = 0; j < 8; j++) {
                ppu.tile_table[tile_index].bit1[j] = raw_data[data_idx++];
            }

            tile_index++;
        }
    }
}

void PlayMode::setup() {
	static std::mt19937 mt; //mersenne twister pseudo-random number generator

    generate_fly();

    // Initialize all tiles to black
    for(size_t i = 0; i < ppu.background.size(); i++) {
        ppu.background[i] = 0;
    }

    player.x = ppu.ScreenWidth / 2;
    player.y = ppu.ScreenHeight / 2;

    paddle.y = 240;
    paddle_exists = false;

    // Setup constant sprites information
    // Paddle
    ppu.sprites[1].index = 6;
    ppu.sprites[1].attributes = 1;

    // Bug
    ppu.sprites[2].index = (int) ((mt() / float(mt.max())) * 3 + 7);
    ppu.sprites[2].attributes = 2;
}

void PlayMode::generate_fly() {
	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	fly.x = mt() / float(mt.max()) * ppu.BackgroundWidth;
	fly.y = mt() / float(mt.max()) * ppu.BackgroundHeight;

	fly_speed.x = mt() / float(mt.max()) * 120 - 60;
	fly_speed.y = mt() / float(mt.max()) * 120 - 60;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
            right.pressed = false;
            up.pressed = false;
            down.pressed = false;
            player_dir = 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
            left.pressed = false;
            up.pressed = false;
            down.pressed = false;
            player_dir = 3;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
            left.pressed = false;
            right.pressed = false;
            down.pressed = false;
            player_dir = 0;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
            left.pressed = false;
            right.pressed = false;
            up.pressed = false;
            player_dir = 2;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
            if(paddle_exists) return true;

            paddle_exists = true;
            paddle_countdown = 0.8f;
            if(player_dir == 0) {
                paddle.x = player.x;
                paddle.y = player.y + 8;
            } else if(player_dir == 1) {
                paddle.x = player.x - 8;
                paddle.y = player.y;
            } else if(player_dir == 2) {
                paddle.x = player.x;
                paddle.y = player.y - 8;
            } else if(player_dir == 3) {
                paddle.x = player.x + 8;
                paddle.y = player.y;
            }

            if(abs(paddle.x - fly.x) <= 7 && abs(paddle.y - fly.y) <= 7) {
                int tilex = (fly.x + 4) / 8;
                int tiley = (fly.y + 4) / 8;
                ppu.background[tilex + tiley * ppu.BackgroundWidth] =
                    (ppu.background[tilex + tiley * ppu.BackgroundWidth] & 0xff00) |
                    1;

                generate_fly();
            }
            return true;
        } else if (evt.key.keysym.sym == SDLK_r) {
            setup();
        }
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	static std::mt19937 mt; //mersenne twister pseudo-random number generator

    if(paddle_exists) {
        paddle_countdown -= elapsed;
        if(paddle_countdown < 0) {
            paddle_exists = false;
            paddle.y = 240;
        }
    }

    // Fly movement logic
    fly.x += fly_speed.x * elapsed;
    fly.y += fly_speed.y * elapsed;

    if(fly.x < 0 || fly.x > 248) {
        fly_speed.x = -fly_speed.x;
        fly_speed.y += mt() / float(mt.max()) * 5 - 2.5;
    }
    if(fly.y < 0 || fly.y > 232) {
        fly_speed.y = -fly_speed.y;
        fly_speed.x += mt() / float(mt.max()) * 5 - 2.5;
    }
    
    if(!paddle_exists) {
        if (left.pressed) {
            player.x -= player_speed * elapsed;

            if(player.x < 0) {
                player.x = 0;
            } else {
                int nearx = player.x / 8;

                int neary = player.y / 8;

                if((ppu.background[nearx + neary * ppu.BackgroundWidth] & 0xff) == 1 ||
                        (ppu.background[nearx + (neary + 1) * ppu.BackgroundWidth] & 0xff) == 1) {
                    player.x = nearx * 8 + 8;
                }
            }
        } else if (right.pressed) {
            player.x += player_speed * elapsed;
            
            if(player.x > 248) {
                player.x = 248;
            } else {
                int nearx = (player.x + 7) / 8;

                int neary = player.y / 8;

                if((ppu.background[nearx + neary * ppu.BackgroundWidth] & 0xff) == 1 ||
                        (ppu.background[nearx + (neary + 1) * ppu.BackgroundWidth] & 0xff) == 1) {
                    player.x = nearx * 8 - 8;
                }
            }
        } else if (down.pressed) {
            player.y -= player_speed * elapsed;

            if(player.y < 0) {
                player.y = 0;
            } else {
                int nearx = player.x / 8;

                int neary = player.y / 8;

                if((ppu.background[nearx + neary * ppu.BackgroundWidth] & 0xff) == 1 ||
                        (ppu.background[(nearx + 1) + neary * ppu.BackgroundWidth] & 0xff) == 1) {
                    player.y = neary * 8 + 8;
                }
            }
        } else if (up.pressed) {
            player.y += player_speed * elapsed;

            if(player.x > 248) {
                player.x = 248;
            } else {
                int nearx = player.x / 8;

                int neary = (player.y + 7) / 8;

                if((ppu.background[nearx + neary * ppu.BackgroundWidth] & 0xff) == 1 ||
                        (ppu.background[(nearx + 1) + neary * ppu.BackgroundWidth] & 0xff) == 1) {
                    player.y = neary * 8 - 8;
                }
            }
        }
    }

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

    // Player sprite
    ppu.sprites[0].x = int32_t(player.x);
    ppu.sprites[0].y = int32_t(player.y);
    ppu.sprites[0].index = 2 + player_dir;
    ppu.sprites[0].attributes = 1;

    // Paddle sprite
    ppu.sprites[1].x = int32_t(paddle.x);
    ppu.sprites[1].y = int32_t(paddle.y);

    // Fly sprite
    ppu.sprites[2].x = int32_t(fly.x);
    ppu.sprites[2].y = int32_t(fly.y);

	//--- actually draw ---
	ppu.draw(drawable_size);
}

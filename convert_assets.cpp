#include "convert_assets.hpp"

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <array>

#include <glm/glm.hpp>
#include "load_save_png.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"
#include "PPU466.hpp"

int convert_spritesheet(std::string png_loc) {
    // Load png data
    glm::uvec2 size;
    std::vector<glm::u8vec4> png_data;
    try {
        std::cerr << "Loading sheet at " << png_loc << std::endl;
        load_png(png_loc, &size, &png_data, UpperLeftOrigin);
    } catch(std::exception const& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    // Generate converted names
    std::string output_loc = png_loc.substr(0, png_loc.find_last_of(".")) +
        ASSET_EXTENSION;
    std::ofstream output_file(output_loc, std::ios::binary);

    // Scan png to convert to tiles & palette
    std::array<glm::u8vec4, 4> palette_data;
    std::vector<PPU466::Tile> tile_data;

    int num_colors = 0;

    for(uint8_t index_x = 0; index_x < size.x / SPRITE_WIDTH; index_x++) {
        for(uint8_t index_y = 0; index_y < size.y / SPRITE_HEIGHT; index_y++) {
            PPU466::Tile tile;
            for(int i = 0; i < 8; i++) {
                tile.bit0[i] = 0;
                tile.bit1[i] = 0;
            }

            for(uint8_t sprite_x = 0; sprite_x < SPRITE_WIDTH; sprite_x++) {
                for(uint8_t sprite_y = 0; sprite_y < SPRITE_HEIGHT; sprite_y++) {
                    // Calculate actual indices relative to spritesheet
                    uint16_t real_x = index_x * SPRITE_WIDTH + sprite_x;
                    uint16_t real_y = index_y * SPRITE_HEIGHT + sprite_y;
                    uint16_t real_index = real_y * size.x + real_x;

                    // color equality check
                    auto color_equal = [](glm::u8vec4 c1, glm::u8vec4 c2) {
                        return c1.r == c2.r &&
                            c1.g == c2.g &&
                            c1.b == c2.b &&
                            c1.a == c2.a;
                    };

                    // Check for color in palette
                    int color = -1;
                    for(int i = 0; i < num_colors; i++) {
                        //TODO
                        if(color_equal(png_data.at(real_index), palette_data[i])) {
                            color = i;
                            break;
                        }
                    }

                    // Check if color needs to be added to palette
                    if(color == -1) {
                        // Check if we run out of colors for the palette
                        if(num_colors == 4) {
                            std::cerr << "Error: Maximum color limit reached" << std::endl;
                            return -1;
                        }
                        // Add new color
                        palette_data[num_colors].r = png_data.at(real_index).r;
                        palette_data[num_colors].g = png_data.at(real_index).g;
                        palette_data[num_colors].b = png_data.at(real_index).b;
                        palette_data[num_colors].a = png_data.at(real_index).a;
                        color = num_colors;
                        num_colors++;
                    }

                    // Write colors data
                    tile.bit0[SPRITE_HEIGHT - sprite_y - 1] |= (color % 2) << sprite_x;
                    tile.bit1[SPRITE_HEIGHT - sprite_y - 1] |= (color / 2) << sprite_x;
                }
            }

            tile_data.emplace_back(tile);
        }
    }

    // Write all 4 colors even if they don't exist
    std::vector<uint8_t> output_data;
    for(int i = 0; i < 4; i++) {
        output_data.emplace_back(palette_data[i].r);
        output_data.emplace_back(palette_data[i].g);
        output_data.emplace_back(palette_data[i].b);
        output_data.emplace_back(palette_data[i].a);
    }

    // Write tiles data
    for(PPU466::Tile tile : tile_data) {
        for(int i = 0; i < 8; i++) {
            output_data.emplace_back(tile.bit0[i]);
        }
        for(int i = 0; i < 8; i++) {
            output_data.emplace_back(tile.bit1[i]);
        }
    }

    write_chunk(MAGIC, output_data, &output_file);

    output_file.close();

    return 0;
}

int main(int argc, char** argv) {
    // Load each argument
    for(int i = 1; i < argc; i++) {
        convert_spritesheet(argv[i]);
    }

    return 0;
}

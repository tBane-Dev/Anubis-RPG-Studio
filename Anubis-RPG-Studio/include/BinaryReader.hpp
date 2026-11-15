#pragma once
#include <iostream>
#include <fstream>
#include <SFML/Graphics.hpp>

// TO-DO - add namespace Anubis

class BinaryReader {
    std::istream& is;

public:
    BinaryReader(std::istream& is);

    std::wstring read_wstring();
    uint32_t read_uint32();
    int32_t read_int32();
    int16_t read_int16();
    int8_t read_int8();
	float read_float();
    sf::Vector2i read_Vector2i();
    sf::Image read_Image();
    std::shared_ptr<std::vector<uint8_t>> read_data();
};

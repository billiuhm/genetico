#include "images.hpp"

#include <iostream>
#include <string>

int main() {
    imager img;

    image x = img.get_image(0);
    std::cout << std::to_string(x.access_pixel(0, 0).r) << " " << std::to_string(x.access_pixel(0, 0).g) << " " << std::to_string(x.access_pixel(0, 0).b) << x.name;

    return 0;
}
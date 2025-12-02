#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "tpool.hpp"
#include <future>
#include <thread>
#include <filesystem>

struct rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    rgb() : r(0), g(0), b(0) {}
    rgb(int r, int g, int b) : r(r), g(g), b(b) {} 
};

struct image {
    int width;
    std::string name;
    std::vector<rgb> pixels;

    inline rgb access_pixel(int x, int y) {
        return pixels[y * width + x];
    }
};

class imager {
public:
    imager() {
        std::filesystem::path current_path = std::filesystem::current_path();
        std::vector<std::future<image>> fts;
        size_t idx = 0;

        for (std::filesystem::path p : std::filesystem::recursive_directory_iterator(current_path / "training")) {
            fts.emplace_back(tpool.addTask([p, this]() -> image { return pull(p); }));
            ++idx;
        }

        for (auto& fut : fts) {
            image i = fut.get();

            if (i.name != "/404/") {
                all_images.emplace_back(std::move(i));
            }
        }
    }

    inline image get_image(size_t i) { return all_images[i]; }
    inline size_t image_cardinal() { return all_images.size(); }

private:
    threadPool tpool{std::thread::hardware_concurrency()};
    std::vector<image> all_images;

    image pull(std::filesystem::path p) {
        std::string ext = p.extension().string();
        if (ext != ".jpg" && ext != ".jpeg" && ext != ".png" && ext != ".bmp" && ext != ".tga") { image i; i.name = "/404/"; return i; }

        int width, height, channels;
        unsigned char* data = stbi_load(p.string().c_str(), &width, &height, &channels, 3);
        if (!data) { image i; i.name = "/404/"; stbi_image_free(data); return i; }

        image output;
        output.pixels.resize(width * height);
        for (int i = 0; i < width * height; ++i) {
            output.pixels[i] = rgb(data[i*3+0], data[i*3+1], data[i*3+2]);
        }

        output.width = width;
        output.name = p.filename().string();

        stbi_image_free(data);
        return output;

        error_case:
        image i; 
        i.name = "/404/"; 
        stbi_image_free(data); 
        return i;
    }
};

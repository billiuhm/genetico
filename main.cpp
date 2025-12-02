#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <cmath>
#include <future>
#include <thread>

#include "tpool.hpp"
#include "images.hpp"

struct unordered_map_val {
    std::string s;
    uint64_t i;

    bool operator==(const unordered_map_val& other) const {
        return s == other.s && i == other.i;
    }
};

threadPool tp(std::thread::hardware_concurrency());

class NeuralNetwork {
public:
    int layers;
    std::vector<int> sizes;
    std::vector<std::vector<std::vector<float>>> weights;
    std::vector<std::vector<float>> biases;

    NeuralNetwork(int numLayers, const std::vector<int>& layerSizes) : layers(numLayers), sizes(layerSizes) {
        weights.resize(layers - 1);
        biases.resize(layers - 1);

        for (int i = 0; i < layers - 1; ++i) {
            int in = sizes[i];
            int out = sizes[i + 1];

            weights[i].resize(out * in);
            biases[i].resize(out);
        }
    }

    std::vector<float> forward(std::vector<float>& input) {
        biases[0] = input;

    }
};

int main() {
    // Genetic Values
    constexpr uint16_t start_size = 100;
    constexpr uint8_t litter_size = 5;
    constexpr uint8_t survivor_percent = 10;

    // Neural Net Values
    constexpr uint16_t neurons_per_layer = 10;
    constexpr uint8_t layer_count = 5;

    // Get Images
    imager img;

    // Training & Tokens combined into 1 hashmap
    std::unordered_map<unordered_map_val, uint64_t> tokens_training = {
        // Input,              Output,               Hash
        {{ "grow",    static_cast<uint64_t>(+5)    },  0 },
        {{ "shrink",  static_cast<uint64_t>(-5)    },  1 },
        {{ "die",     static_cast<uint64_t>(-1000) },  2 },
        {{ "godmode", static_cast<uint64_t>(+1000) },  3 }
    };

    

    // Begin Neural Networks
    std::vector<NeuralNetwork> beings(start_size);
    std::vector<int> npl(layer_count);
    for (int i = 0; i < layer_count; ++i) npl.emplace_back(neurons_per_layer);
    for (int i = 0; i < start_size; ++i) beings.emplace_back(NeuralNetwork(layer_count, npl));

    // Save memory
    for (int i = 0; i < layer_count; ++i) npl.pop_back();
    npl.shrink_to_fit();

    // Begin randomising weights & biases
    std::vector<std::future<void>> joins(start_size);

    for (int idx = 0; idx < beings.size(); ++idx) {
        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        joins.emplace_back(tp.addTask([&]{
            for (int i = 0; i < beings[idx].layers - 1; ++i) {
                for (int j = 0; j < beings[idx].weights[i].size(); ++j) {
                    for (auto& w : beings[idx].weights[i][j]) w = dist(rng);
                }
                for (auto& b : beings[idx].biases[i]) b = dist(rng);
            }
        }));
    }


}
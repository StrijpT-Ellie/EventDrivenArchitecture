#ifndef CONTOURWALL_HPP
#define CONTOURWALL_HPP

#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <cstdint>

extern "C" {
    struct ContourWallCore;

    ContourWallCore* new(uint32_t baud_rate);
    ContourWallCore* new_with_ports(const char* port1, const char* port2, const char* port3, const char* port4, const char* port5, const char* port6, uint32_t baud_rate);
    ContourWallCore* single_new_with_port(const char* com_port, uint32_t baud_rate);
    void configure_threadpool(uint8_t threads);
    void show(ContourWallCore* cw);
    void update_all(ContourWallCore* cw, const uint8_t* frame_buffer_ptr, bool optimize);
    void solid_color(ContourWallCore* cw, uint8_t red, uint8_t green, uint8_t blue);
    void drop(ContourWallCore* cw);
}

class ContourWall {
public:
    ContourWall(uint32_t baud_rate) {
        instance = new(baud_rate);
    }

    ContourWall(const std::vector<std::string>& com_ports, uint32_t baud_rate) {
        if (com_ports.size() != 6) {
            throw std::invalid_argument("Six COM ports are required.");
        }
        const char* ports[6];
        for (size_t i = 0; i < 6; ++i) {
            ports[i] = com_ports[i].c_str();
        }
        instance = new_with_ports(ports[0], ports[1], ports[2], ports[3], ports[4], ports[5], baud_rate);
    }

    ContourWall(const std::string& com_port, uint32_t baud_rate) {
        instance = single_new_with_port(com_port.c_str(), baud_rate);
    }

    ~ContourWall() {
        drop(instance);
    }

    void configure_threadpool(uint8_t threads) {
        ::configure_threadpool(threads);
    }

    void show() {
        ::show(instance);
    }

    void update_all(const std::vector<uint8_t>& frame_buffer, bool optimize) {
        ::update_all(instance, frame_buffer.data(), optimize);
    }

    void solid_color(uint8_t red, uint8_t green, uint8_t blue) {
        ::solid_color(instance, red, green, blue);
    }

private:
    ContourWallCore* instance;
};

bool check_comport_existence(const std::vector<std::string>& com_ports) {
    for (const auto& port : com_ports) {
        if (!std::any_of(serial.tools.list_ports.comports().begin(), serial.tools.list_ports.comports().end(), [&](const auto& p) { return p.device == port; })) {
            return false;
        }
    }
    return true;
}

std::tuple<int, int, int> hsv_to_rgb(int hue, float saturation, float value) {
    hue = float(hue) / 100.0;
    saturation /= 100;
    value /= 100;
    
    if (saturation == 0.0) return {int(value * 255), int(value * 255), int(value * 255)};
    
    int i = int(hue * 6.0);
    float f = (hue * 6.0) - i;
    i = i % 6;
    
    int p = int((value * (1.0 - saturation)) * 255);
    int q = int((value * (1.0 - saturation * f)) * 255);
    int t = int((value * (1.0 - saturation * (1.0 - f))) * 255);
    int v = int(value * 255);
    
    switch (i) {
        case 0: return {v, t, p};
        case 1: return {q, v, p};
        case 2: return {p, v, t};
        case 3: return {p, q, v};
        case 4: return {t, p, v};
        case 5: return {v, p, q};
        default: return {0, 0, 0};
    }
}

#endif // CONTOURWALL_HPP

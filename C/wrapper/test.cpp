#include "contourwall.hpp"

int main() {
    try {
        // Example with single tile mode
        ContourWall cw_single("COM3", 2000000);
        cw_single.solid_color(255, 0, 255); // Set to purple
        cw_single.show();

        // Example with full wall mode
        std::vector<std::string> com_ports = {"COM3", "COM4", "COM5", "COM6", "COM7", "COM8"};
        ContourWall cw_full(com_ports, 2000000);
        std::vector<uint8_t> frame_buffer(7200, 255); // Example frame buffer
        cw_full.update_all(frame_buffer, false);
        cw_full.show();

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}



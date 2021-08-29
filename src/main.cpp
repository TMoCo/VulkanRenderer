///////////////////////////////////////////////////////
// Main function for the application
///////////////////////////////////////////////////////

// reporting and propagating exceptions
#include <iostream> 
#include <stdexcept>
#include <cstdlib>

// include the application
#include <app/Application.h>
#include <app/AppConstants.h>

int main(int argc, char* argv[]) {
    Application app;
    try {
        if (argc == 2) {
            app.run(argv[1]);
        }
        else {
            app.run(DEFAULT_MODEL.c_str());
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
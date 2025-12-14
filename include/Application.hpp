#ifndef APPLICATION_HPP
#define APPLICATION_HPP
#include <SDL3/SDL.h>
//window constraints
#include <constraints.hpp>

#include <iostream>
#include <vector>
#include <algorithm>

//components
#include <component.hpp>
#include <input_switch.hpp>
#include <output_light.hpp>
#include <gate_and.hpp>
#include <gate_or.hpp>
#include <gate_not.hpp>


class Application{
    public:
        Application();
        ~Application();

        bool init();
        void run();
    
    private:
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        TTF_Font* font = nullptr;
        bool isRunning = true;

        std::vector<Component*> components;

        //interaction
        Component* selectedComponent = nullptr;
        Component* wiringSource = nullptr;
        bool isWiring = false;
        float mouseX = 0;
        float mouseY = 0;

        //helper functions
        void handleEvents();
        void update();
        void render();
        void cleanup();
};
#endif // APPLICATION_HPP
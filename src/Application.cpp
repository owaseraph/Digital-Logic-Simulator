#include <Application.hpp>
#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

Application::Application()
{
    // Constructor (Variables are already initialized in header)
}

Application::~Application()
{
    cleanup();
}

bool Application::init()
{
    // 1. Init SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init Failed: %s", SDL_GetError());
        return false;
    }

    // 2. Init Fonts
    if (!TTF_Init())
    {
        SDL_Log("TTF_Init Failed: %s", SDL_GetError());
        return false;
    }

    // 3. Create Window (Resizable)
    window = SDL_CreateWindow("Digital Logic Simulator", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window)
        return false;

    // 4. Create Renderer
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
        return false;

    // 5. Logical Presentation (Auto-scaling)
    SDL_SetRenderLogicalPresentation(renderer, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // 6. Load Font
    font = TTF_OpenFont("font.ttf", 20);
    if (!font)
    {
        SDL_Log("Warning: font.ttf not found. Text will not show.");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // allow keyboard nav
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark(); // use dark mode

    // setup renderer backend
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    return true;
}

void Application::run()
{
    if (!init())
        return;
    while (isRunning)
    {
        handleEvents();
        update();
        render();
    }
}

void Application::handleEvents()
{
    SDL_Event event;
    float rawMouseX, rawMouseY;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT)
        {
            isRunning = false;
        }


        //stop main frame if mouse on UI
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse) continue;

        SDL_GetMouseState(&rawMouseX, &rawMouseY);

        // set render coord
        SDL_RenderCoordinatesFromWindow(renderer, rawMouseX, rawMouseY, &mouseX, &mouseY);

        // check for mouse click
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {

            bool clickedSomething = false; // to track if we hit anything
            // loop through components to see if we clicked one
            for (Component *comp : components)
            {
                HitZone zone = comp->getHitZone(mouseX, mouseY);

                if (zone == HIT_OUTPUT)
                {
                    // start wiring
                    isWiring = true;
                    wiringSource = comp;
                    clickedSomething = true;

                    // note: we dont change selection when wiring, usually better UX
                    break; // break so we cannot click on 2 things
                }
                else if (zone == HIT_BODY)
                {
                    // select it
                    selectedComponent = comp;
                    clickedSomething = true;

                    // start dragging
                    comp->isDragging = true;
                    comp->dragOffsetX = mouseX - comp->x;
                    comp->dragOffsetY = mouseY - comp->y;

                    // toggle switch (on or off)
                    Input_Switch *sw = dynamic_cast<Input_Switch *>(comp);
                    if (sw)
                    {
                        sw->toggle();
                    }
                    break;
                }
            }

            // if we click on nothing deselect
            if (!clickedSomething)
            {
                selectedComponent = false;
            }
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            // if we were dragging a wire, try dropping it
            if (isWiring && wiringSource != nullptr)
            {
                bool connectionMade = false;

                for (Component *comp : components)
                {
                    HitZone zone = comp->getHitZone(mouseX, mouseY);

                    // case 1 : dropped on top input
                    if (zone == HIT_INPUT1)
                    {
                        // and gate
                        if (auto gate = dynamic_cast<And_Gate *>(comp))
                        {
                            gate->attachInput1(wiringSource);
                            connectionMade = true;
                        }
                        // or gate
                        else if (auto gate = dynamic_cast<Or_Gate *>(comp))
                        {
                            gate->attachInput1(wiringSource);
                            connectionMade = true;
                        }
                        // not gate
                        else if (auto gate = dynamic_cast<Not_Gate *>(comp))
                        {
                            gate->attach(wiringSource);
                            connectionMade = true;
                        }
                        // light bulb
                        else if (auto light = dynamic_cast<Output_Light *>(comp))
                        {
                            light->attach(wiringSource);
                            connectionMade = true;
                        }
                    }

                    // case 2: dropped on bottom inpu
                    else if (zone == HIT_INPUT2)
                    {
                        // and gate
                        if (auto gate = dynamic_cast<And_Gate *>(comp))
                        {
                            gate->attachInput2(wiringSource);
                            connectionMade = true;
                        }
                        // or gate
                        else if (auto gate = dynamic_cast<Or_Gate *>(comp))
                        {
                            gate->attachInput2(wiringSource);
                            connectionMade = true;
                        }
                    }

                    if (connectionMade)
                        break;
                }
            }

            // reset states
            isWiring = false;
            wiringSource = nullptr;
            for (Component *comp : components)
                comp->isDragging = false;
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            for (Component *comp : components)
            {
                if (comp->isDragging)
                {   
                    float rawX = mouseX - comp->dragOffsetX;
                    float rawY = mouseY - comp->dragOffsetY;

                    comp->x = (float)((int)rawX/GRID_SIZE)*GRID_SIZE;
                    comp->y = (float)((int)rawY/GRID_SIZE)*GRID_SIZE;
                }
            }
        }

        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            // check for delete and backspace
            if (event.key.key == SDLK_DELETE || event.key.key == SDLK_BACKSPACE)
            {
                if (selectedComponent != nullptr)
                {
                    // safety: disconnect incoming wires
                    // we iterate through all components to see if any are connected to the one we r deleting
                    for (Component *other : components)
                    {
                        // check and gate
                        if (auto gate = dynamic_cast<And_Gate *>(other))
                        {
                            if (gate->input1 == selectedComponent)
                                gate->input1 = nullptr;
                            if (gate->input2 == selectedComponent)
                                gate->input2 = nullptr;
                        }
                        // check or gate
                        else if (auto gate = dynamic_cast<Or_Gate *>(other))
                        {
                            if (gate->input1 == selectedComponent)
                                gate->input1 = nullptr;
                            if (gate->input2 == selectedComponent)
                                gate->input2 = nullptr;
                        }
                        // check not gate
                        else if (auto gate = dynamic_cast<Not_Gate *>(other))
                        {
                            if (gate->source == selectedComponent)
                                gate->source = nullptr;
                        }
                        // check light input
                        else if (auto light = dynamic_cast<Output_Light *>(other))
                        {
                            if (light->source == selectedComponent)
                                light->source = nullptr;
                        }
                    }

                    // safety: if we are currently wiring from this object
                    // stop wiring
                    if (wiringSource == selectedComponent)
                    {
                        isWiring = false;
                        wiringSource = nullptr;
                    }

                    // remove from the list
                    // find component in vector and delete
                    auto it = std::find(components.begin(), components.end(), selectedComponent);
                    if (it != components.end())
                    {
                        components.erase(it);
                    }

                    // delete memory
                    delete selectedComponent;
                    selectedComponent = nullptr;
                }
            }
        }
    }
}

void Application::update()
{
    for (Component *comp : components)
    {
        comp->calculate();
    }
}

void Application::render()
{
    //start imgui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    //define toolbox window
    ImGui::SetNextWindowPos(ImVec2(0,0),ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(SCREEN_WIDTH,60));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoScrollbar;


    //start window named toolbox
    ImGui::Begin("Toolbox", nullptr, window_flags);

    //button: and gate
    if (ImGui::Button("AND Gate")) {
        //spawn in middle
        And_Gate* newGate = new And_Gate(640, 320);
        newGate->labelText = "AND";
        newGate->createLabelTexture(renderer, font);
        components.push_back(newGate);
    }
    ImGui::SameLine();

    //button: or gate
    if (ImGui::Button("OR Gate")) {
        Or_Gate* newGate = new Or_Gate(640, 320);
        newGate->labelText = "OR";
        newGate->createLabelTexture(renderer, font);
        components.push_back(newGate);
    }
    ImGui::SameLine();
    
    //button: not gate
    if (ImGui::Button("NOT Gate")) {
        Not_Gate* newGate = new Not_Gate(640, 320);
        newGate->labelText = "NOT";
        newGate->createLabelTexture(renderer, font);
        components.push_back(newGate);
    }
    ImGui::SameLine();

    //button: switch
    if (ImGui::Button("Switch")) {
        Input_Switch* newSw = new Input_Switch(640, 320);
        newSw->labelText = "Input";
        newSw->createLabelTexture(renderer, font);
        components.push_back(newSw);
    }
    ImGui::SameLine();

    //button: light
    if (ImGui::Button("Light")) {
        Output_Light* newLight = new Output_Light(640, 320);
        newLight->labelText = "Light";
        newLight->createLabelTexture(renderer, font);
        components.push_back(newLight);
    }
    //finish logic
    ImGui::End();

    //render window
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 70,70,70,255);
    for(int x=0;x<SCREEN_WIDTH;x+=GRID_SIZE){
        SDL_RenderLine(renderer,x,0,x,SCREEN_HEIGHT);
    }
    for(int y=0;y<SCREEN_HEIGHT;y+=GRID_SIZE){
        SDL_RenderLine(renderer,0,y,SCREEN_WIDTH,y);
    }

    for (Component *comp : components)
    {
        comp->draw(renderer);
        comp->drawLabel(renderer);

        //draw selection box
        if (comp == selectedComponent)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_FRect selRect = {comp->x - 3, comp->y - 3, (float)comp->width + 6, (float)comp->height + 6};
            SDL_RenderRect(renderer, &selRect);
        }
    }

    //draw wiring line
    if (isWiring && wiringSource)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderLine(renderer, wiringSource->x + wiringSource->width, wiringSource->y + wiringSource->height / 2, mouseX, mouseY);
    }
    //redner the imgui on top
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void Application::cleanup()
{
    //delete all components
    for (Component *comp : components)
    {
        delete comp;
    }
    components.clear();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    //destroy sdl objects
    if (font)
        TTF_CloseFont(font);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    
    TTF_Quit();
    SDL_Quit();
}
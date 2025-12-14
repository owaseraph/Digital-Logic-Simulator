#include <Application.hpp>
#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <fstream> //file reading and writing as to save the progress
#include <json.hpp>

using json = nlohmann::json;

Application::Application()
{
    //constructor (variables are already initialized in header file)
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

void Application::saveCircuit(const std::string& filename){
    json j_scene = json::array(); //root array

    //serialize every component
    for(int i=0; i<components.size();i++){
        Component* comp = components[i];
        json j_comp;

        j_comp["id"] = i; //save index
        j_comp["type"] = comp->getType(); //save type
        j_comp["x"] = comp->x;
        j_comp["y"] = comp->y;


        //handle wiring
        //helper lambda to find index of source pointer
        auto getIndex = [&](Component* target) -> int{
            if(!target){
                return -1;
            }
            auto it = std::find(components.begin(), components.end(), target);
            if(it != components.end()){
                return std::distance(components.begin(), it);
            }
            return -1;
        };

        if (auto g = dynamic_cast<And_Gate*>(comp)) {
            j_comp["in1"] = getIndex(g->input1);
            j_comp["in2"] = getIndex(g->input2);
        }
        else if (auto g = dynamic_cast<Or_Gate*>(comp)) {
            j_comp["in1"] = getIndex(g->input1);
            j_comp["in2"] = getIndex(g->input2);
        }
        else if (auto g = dynamic_cast<Not_Gate*>(comp)) {
            j_comp["src"] = getIndex(g->source);
        }
        else if (auto l = dynamic_cast<Output_Light*>(comp)) {
            j_comp["src"] = getIndex(l->source);
        }

        j_scene.push_back(j_comp);
    }

    //write to file
    std::ofstream file(filename);
    if(file.is_open()){
        file<<j_scene.dump(4);//4 space indent
        file.close();
        std::cout<<"Saved to: "<<filename<<std::endl;
    }
}

void Application::loadCircuit(const std::string& filename){
    std::ifstream file(filename);
    if(!file.is_open()){
        std::cout<<"Failed to open: "<<filename<<std::endl;
        return;
    }

    json j_scene;
    file>>j_scene; //parse JSON

    //clear old scene
    for(Component* c: components){
        delete c;
    }
    components.clear();
    selectedComponent = nullptr;
    wiringSource = nullptr;
    isWiring = false;


    //create objects (no wiring)
    for(const auto& item: j_scene){
        std::string type = item["type"];
        float x = item["x"];
        float y = item["y"];

        Component* newComp = nullptr;

        if (type == "AND") newComp = new And_Gate(x, y);
        else if (type == "OR") newComp = new Or_Gate(x, y);
        else if (type == "NOT") newComp = new Not_Gate(x, y);
        else if (type == "SWITCH") newComp = new Input_Switch(x, y);
        else if (type == "LIGHT") newComp = new Output_Light(x, y);

        if(newComp){
            //re-create the label texture
            newComp->labelText = (type == "SWITCH" ? "Input" : type);
            if(type == "LIGHT"){
                newComp->labelText = "Light";
            }
            newComp->createLabelTexture(renderer,font);

            components.push_back(newComp);
        }
    }

    //reconnect wires
    for(int i=0; i<j_scene.size(); i++){
        json item = j_scene[i];

        Component* comp = components[i];

        if (auto g = dynamic_cast<And_Gate*>(comp)) {
            int idx1 = item.value("in1", -1); // value() gets key or default -1
            int idx2 = item.value("in2", -1);
            if (idx1 >= 0 && idx1 < components.size()) g->input1 = components[idx1];
            if (idx2 >= 0 && idx2 < components.size()) g->input2 = components[idx2];
        }
        else if (auto g = dynamic_cast<Or_Gate*>(comp)) {
            int idx1 = item.value("in1", -1);
            int idx2 = item.value("in2", -1);
            if (idx1 >= 0 && idx1 < components.size()) g->input1 = components[idx1];
            if (idx2 >= 0 && idx2 < components.size()) g->input2 = components[idx2];
        }
        else if (auto g = dynamic_cast<Not_Gate*>(comp)) {
            int src = item.value("src", -1);
            if (src >= 0 && src < components.size()) g->source = components[src];
        }
        else if (auto l = dynamic_cast<Output_Light*>(comp)) {
            int src = item.value("src", -1);
            if (src >= 0 && src < components.size()) l->source = components[src];
        }
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
    ImGui::SameLine();
    float spacing = ImGui::GetContentRegionAvail().x - 120; // 120 is approx width of 2 buttons
    if (spacing > 0) ImGui::SameLine(ImGui::GetCursorPosX() + spacing);

    if (ImGui::Button("SAVE")) {
        saveCircuit("circuit.json");
    }
    ImGui::SameLine();
    if (ImGui::Button("LOAD")) {
        loadCircuit("circuit.json");
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
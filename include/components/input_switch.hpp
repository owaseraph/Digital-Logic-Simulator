#ifndef INPUT_SWITCH_HPP
#define INPUT_SWITCH_HPP

#include <component.hpp>
#include <SDL3/SDL.h>

// @brief
// input switch class (source of logic)

class Input_Switch : public Component {
    public:
        Input_Switch(float x, float y):Component(x,y){
            width = 40; //make switches smaller than gates
            height = 40;
        }

        //toggle state when clicked
        void toggle(){
            outputState = !outputState;
        }

        //for a switch, calculate() just ensures the state remains the same
        void calculate() override{
            //the output stays the same, no need to type anything in here
        }

        //drawing the switch
        void draw(SDL_Renderer* renderer)override{
            //choose color based on state
            if(outputState){
                SDL_SetRenderDrawColor(renderer, 0, 255,0,255);//green for on
            }
            else{
                SDL_SetRenderDrawColor(renderer, 200,0,0,255);//red for off
            }

            //define rectangle area and fill said area
            SDL_FRect rect = {x,y,(float)width, (float)height};
            SDL_RenderFillRect(renderer, &rect);

            //white border
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderRect(renderer, &rect);
        }

        HitZone getHitZone(float mx, float my) override {
            // only output and body, no input
            if (mx >= x + width - 10 && mx <= x + width + 10 &&
                my >= y + height/2 - 10 && my <= y + height/2 + 10) {
                return HIT_OUTPUT;
            }
            
            if (mx >= x && mx <= x + width && my >= y && my <= y + height) {
                return HIT_BODY;
            }
            
            return HIT_NONE;
    }
};

#endif // INPUT_SWITCH_HPP
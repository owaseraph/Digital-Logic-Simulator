#ifndef GATE_NOT_HPP
#define GATE_NOT_HPP

#include <component.hpp>
#include <SDL.h>

// @brief
//class that defines a not gate
class Not_Gate: public Component{
    public:
        Component *source;

        Not_Gate(float x, float y):Component(x,y), source(nullptr){
            width = 60;
            height = 40;
        }

        void attach(Component* s){
            source = s;
        }

        void calculate()override{
            if(source!=nullptr){
                this->outputState = !source->outputState;
            }
            else{
                this->outputState = false;
            }
        }
        HitZone getHitZone(float mx, float my) override {
            if (mx >= x + width - 10 && mx <= x + width + 10 &&
                my >= y + height/2 - 10 && my <= y + height/2 + 10) {
                return HIT_OUTPUT;
            }

            if (mx >= x - 10 && mx <= x + 10 &&
                my >= y + height/2 - 10 && my <= y + height/2 + 10) {
                return HIT_INPUT1;
            }

            if (mx >= x && mx <= x + width && my >= y && my <= y + height) {
                return HIT_BODY;
            }

            return HIT_NONE;
        }

        void draw(SDL_Renderer* renderer) override{
            //first, draw wires
            if(source!=nullptr){
                SDL_SetRenderDrawColor(renderer, 150,150,150,255);//gray wires
                if(source->outputState){
                    SDL_SetRenderDrawColor(renderer, 0,255,0,255);
                }    
                else{
                    SDL_SetRenderDrawColor(renderer, 100,0,0,255);
                }
                //calculate centers
                int startX = source->x + source->width;
                int startY = source->y + (source->height/2);
                int endX = x;
                int endY = y + (height/2);
                
                SDL_RenderLine(renderer,startX,startY,endX,endY);
            }
            //add connection nodes (white)
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); 
            SDL_FRect nodeIn= { x - 5, y + height/2 - 5, 10, 10 };
            SDL_RenderFillRect(renderer, &nodeIn);
            //output node (right center)
            SDL_FRect nodeOut = { x + width - 5, y + height/2 - 5, 10, 10 };
            SDL_RenderFillRect(renderer, &nodeOut);

            SDL_SetRenderDrawColor(renderer, 0,100,255,255);
            SDL_FRect rect = {x,y,(float)width, (float)height};
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderRect(renderer, &rect);
        }

};
#endif // GATE_NOT_HPP
#ifndef GATE_OR_HPP
#define GATE_OR_HPP

#include <component.hpp>
#include <SDL.h>

// @brief
// class defining an or gate
class Or_Gate: public Component{
    public:
        Component *input1;
        Component *input2;

        Or_Gate(float x, float y):Component(x,y), input1(nullptr), input2(nullptr){
            width = 60;
            height = 40;
        }

        void attachInput1(Component* s){
            input1 = s;
        }
        void attachInput2(Component* s){
            input2 = s;
        }

        void calculate()override{
            //get state of input1
            bool val1 = (input1 != nullptr) ? input1->outputState : false;
            bool val2 = (input2 != nullptr) ? input2->outputState : false;

            this->outputState = val1 || val2;
        }

        void draw(SDL_Renderer* renderer) override{
            //first, draw wires
            SDL_SetRenderDrawColor(renderer, 150,150,150,255);//gray wires

            if(input1!=nullptr){
                if(input1->outputState){
                    SDL_SetRenderDrawColor(renderer, 0,255,0,255);
                }
                else{
                    SDL_SetRenderDrawColor(renderer, 100,0,0,255);
                }

                SDL_RenderLine(renderer, input1->x+input1->width,input1->y+input1->height/2,
                    x,y+10);
            }

            if(input2!=nullptr){
                if(input2->outputState){
                    SDL_SetRenderDrawColor(renderer, 0,255,0,255);
                }
                else{
                    SDL_SetRenderDrawColor(renderer, 100,0,0,255);
                }

                SDL_RenderLine(renderer, input2->x+input2->width,input2->y+input2->height/2,
                    x,y+30);
            }
            //add connection nodes (white)
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            //input node 1 (left)
            SDL_FRect nodeIn1 = { x - 5, y + 10 - 5, 10, 10 };
            SDL_RenderFillRect(renderer, &nodeIn1);
            //input node 2 (left)
            SDL_FRect nodeIn2 = { x - 5, y + height - 10 - 5, 10, 10 };
            SDL_RenderFillRect(renderer, &nodeIn2);
            //output node (right center)
            SDL_FRect nodeOut = { x + width - 5, y + height/2 - 5, 10, 10 };
            SDL_RenderFillRect(renderer, &nodeOut);

            SDL_SetRenderDrawColor(renderer, 0,100,255,255);
            SDL_FRect rect = {x,y,(float)width, (float)height};
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderRect(renderer, &rect);
        }

        std::string getType() override{
            return "NOT";
        }

};
#endif // GATE_OR_HPP
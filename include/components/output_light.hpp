#ifndef OUTPUT_LIGHT_HPP
#define OUTPUT_LIGHT_HPP
#include <component.hpp>
#include <SDL.h>
// @brief
// class that declares a light bulb component
class Output_Light : public Component{
    public:
        // declare where this is connected to
        Component* source;
        
        Output_Light(float x, float y):Component(x,y), source(nullptr){
            width = 30;
            height =30;
        }

        //connect the wire
        void attach(Component *s){
            source = s;
        }

        void calculate()override{
            //if connected, copy the state
            if(source!=nullptr){
                this->outputState = source->outputState;
            }
            //else, floating
            else{
                this->outputState = false;
            }
        }

        HitZone getHitZone(float mx, float my) override {
            // Lights ONLY have Input 1 (the wire connection) and Body.
            
            // Check Input 1 (We treat the single input as INPUT1)
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
            //draw the wire (visual only)
            if(source!=nullptr){
                //pick wire based on signal
                if(outputState){
                    SDL_SetRenderDrawColor(renderer, 0,255,0,255);//green
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


            //draw the light bulb

            if(outputState){
                SDL_SetRenderDrawColor(renderer, 255,255,0,255);
            }
            else SDL_SetRenderDrawColor(renderer, 50,50,50,255);

            SDL_FRect rect = {x,y,float(width),float(height)};
            SDL_RenderFillRect(renderer,&rect);

            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderRect(renderer, &rect);
        }
};
#endif // OUTPUT_LIGHT_HPP
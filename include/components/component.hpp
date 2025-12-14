#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include <SDL3/SDL.h>
#include <SDL_ttf.h>
#include <string>
struct Pin
{
    int x, y;
    bool *val;
};

// @brief
// this allows us to identify which part of the component we are clicking on
enum HitZone{
    HIT_NONE,
    HIT_BODY,
    HIT_INPUT1,
    HIT_INPUT2,
    HIT_OUTPUT
};
// @brief
// Components class that defines a simple component, all of this will be implemented in different gates
class Component
{
public:
    float x, y;
    int width, height;
    bool outputState;
    bool isDragging;
    float dragOffsetX, dragOffsetY;

    std::string labelText;
    SDL_Texture* labelTexture = nullptr;//the image of the text
    int labelWidth = 0;
    int labelHeight = 0;


    Component(float startX, float startY, std::string labelText="") : x(startX), y(startY),
                                            width(60), height(40), outputState(false),
                                            isDragging(false), dragOffsetX(0), dragOffsetY(0) {};

    virtual ~Component() {
        if(labelTexture){
            SDL_DestroyTexture(labelTexture);
        }
    };

    // core logic

    // 1. brain : updates outputState based on inputs
    // it is "= 0" because a generic component doesn't know how to calculate.
    // @brief
    // function to calculate the output state of a component
    virtual void calculate() = 0;

    // 2. the face : draws specific shape
    virtual void draw(SDL_Renderer *renderer) = 0;

    // 3. hit detection
    // @brief
    // function to detect the zone hit by the mouse when pressing click
    virtual HitZone getHitZone(float mx, float my) {
        // check output pin
        if (mx >= x + width - 10 && mx <= x + width + 10 &&
            my >= y + height/2 - 10 && my <= y + height/2 + 10) {
            return HIT_OUTPUT;
        }

        // check input1
        if (mx >= x - 10 && mx <= x + 10 &&
            my >= y + 10 - 10 && my <= y + 10 + 10) {
            return HIT_INPUT1;
        }

        // check input2
        if (mx >= x - 10 && mx <= x + 10 &&
            my >= y + height - 10 - 10 && my <= y + height - 10 + 10) {
            return HIT_INPUT2;
        }

        // check body
        if (mx >= x && mx <= x + width && my >= y && my <= y + height) {
            return HIT_BODY;
        }

        return HIT_NONE;
    }

    //4. label generation

    // @brief
    // helper to generate text image
    void createLabelTexture(SDL_Renderer* renderer, TTF_Font* font){
        if(labelText.empty() || !font){
            return;
        }

        //destroy old texture if it exists
        if(labelTexture) SDL_DestroyTexture(labelTexture);

        //render text to a temporary surface
        SDL_Color color = {255,255,255,255}; //white text color;
        SDL_Surface* surface = TTF_RenderText_Blended(font, labelText.c_str(),0,color);

        if(surface){
            //convert surface to gpu texture
            labelTexture = SDL_CreateTextureFromSurface(renderer,surface);
            labelWidth = surface->w;
            labelHeight = surface->h;
            SDL_DestroySurface(surface);//free the CPU surface
        }
    }



    // @brief
    // function to draw the text of each component
    void drawLabel(SDL_Renderer* renderer){
        if(labelTexture){
            //center the text
            float textX = x + (width-labelWidth)/2;
            float textY = y -labelHeight -5; //5 pixels above

            SDL_FRect dstRect = {textX, textY, (float)labelWidth, (float)labelHeight};
            SDL_RenderTexture(renderer, labelTexture, NULL, &dstRect);
        }
    }


    // @brief
    // function to return a status json of each component implemented
    // must be pure virtual, each child must implement their own version based on necessities
    virtual std::string getType() = 0;
};

#endif // COMPONENT_HPP
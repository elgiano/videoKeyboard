//
//  fontPlayer.hpp
//  fontTest
//
//  Created by Gnlc Elia on 24/11/2018.
//

#ifndef fontPlayer_hpp
#define fontPlayer_hpp

#include <stdio.h>
#include <string>
#include "ofMain.h"

class FontPlayer{
    
    
public:
    
    constexpr static const float LETTERS_PER_S = 16;
    
    enum Alignment{
        START,
        CENTER,
        END
    };
    
    enum AnimationType{
        SLIDE,
        WORDFADE
    };
    
    Alignment xAlign = Alignment::START;
    Alignment yAlign = Alignment::CENTER;
    
    AnimationType animationType = AnimationType::WORDFADE;
    
    ofColor color;
    
    float margin = 120;
    
    bool load(std::string text);
    bool load(std::string text,int size);

    
    std::string setFontSize(int size);
    
    ofTexture *getTexture();
    
    void play();
    void stop();
    void update();
    void nextFrame();
    
    float getWidth() const;
    float getHeight() const;
    
    float getPosition() const;
    float setPosition(float pct);
    float getDuration() const;
    
    bool  isPlaying() const;
    void  setSpeed(float speed);
    
private:
    ofTrueTypeFont font;
    ofTexture texture;
    ofFbo fbo;
    ofFbo maskFbo;

    std::string text;
    std::string wrappedText;
    std::string currentLineText;
    
    int fontSize=120;
    
    bool reverse = false;
    float animationSpeed = 1.0f;
    float animationCurrPos = 0;
    float currentLetter = 0;
    bool playing;
    
    int completedLineChars = 0;
    int currentLine = 0;
    
    float lastUpdateTime;
    
    void slideAnimation(int x,int y);
    void wordFadeAnimation(int x,int y);
    void wordFadeAnimationReverse(int x,int y);

    ofRectangle showCompletedLines(int x,int y);
    ofRectangle showCompletedLettersInCurrentLine(int x,int y);
    ofRectangle getTextBoxToCurrentLine(int x,int y, int additionalChars);
    ofRectangle getTextBoxToCurrentLine(int x,int y, int additionalChars,string txt);

    void updateCurrentLineCount();
    std::string wrapText();
    
};

#endif /* fontPlayer_hpp */

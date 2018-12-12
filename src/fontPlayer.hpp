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
        WORDFADE,
        TARGETWORD
    };
    
    enum SpreadMode{
        TOGETHER,
        SPREAD
    };

    Alignment xAlign = Alignment::CENTER;
    Alignment yAlign = Alignment::CENTER;

    float lettersPerSecond = LETTERS_PER_S;

    AnimationType animationType = AnimationType::WORDFADE;
    SpreadMode spreadMode = SpreadMode::TOGETHER;
    
    std::vector<ofRectangle> constellation;

    ofColor color;

    float margin = 120;
    float marginY = 120;

    float widthRatio=1.0;
    float heightRatio=1.0;
    
    float fontScale = 1.0;

    bool autoResize = false;


    bool load(std::string text);
    bool load(std::string text,int size);
    std::string setFontSize(int size);
    void setFontScale(float scale);

    std::vector<int> targetWords;


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
    vector <string> words;
    vector <string> lines;


    int fontSize=20;//24;

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
    void wordFadeAnimationConstellation();
    void targetWordAnimation(int x,int y);
    void constellationTrembleAnimation();

    
    void drawConstellation();
    void drawConstellationMask();
    void drawConstellationWord(int i);



    void showCompletedLines(int x,int y);
    ofRectangle showCompletedLettersInCurrentLine(int x,int y);
    ofRectangle getTextBoxToCurrentLine(int x,int y, int additionalChars);
    ofRectangle getTextBoxToCurrentLine(int x,int y, int additionalChars,string txt);
    ofRectangle getWordBoundingBox(int wordIndex,int x, int y);
    int showCompletedWords();


    void updateCurrentLineCount();
    std::string wrapText();
    std::string parseText(std::string text);
    void parseTargetWords();

    std::vector<std::string> getWords(std::string text);
    void makeConstellation();


    void clearFbos();

};

#endif /* fontPlayer_hpp */

#pragma once

class Animation
{
public:
    Animation();
    ~Animation();

    void Animate(int animationOffset, int animationDistance, double pixelsPerSecond);
    bool IsAnimating();
    void Tick();
    int Position();
private:
    LONG CurrentMilliseconds();

    bool isAnimating;
    int distance;
    double location;
    int offset;
    double speed;
    LONG lastMilliseconds;
};

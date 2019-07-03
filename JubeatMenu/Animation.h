#pragma once

#define SPEED_REDUCE_PERCENTAGE 0.6
#define SPEED_REDUCE_LOCATION 0.5
#define MAX_SPEED_CHANGES 8

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
    double speedChangeLocation;
    int speedChanges;
    int offset;
    double speed;
    LONG lastMilliseconds;
};

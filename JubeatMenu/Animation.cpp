#include <windows.h>
#include "Animation.h"

Animation::Animation()
{
    distance = 0;
    speed = 0.0;
    lastMilliseconds = 0;
    location = 0.0;
    offset = 0;
    isAnimating = false;
}

Animation::~Animation()
{
    // Empty
}

void Animation::Animate(int animationOffset, int animationDistance, double pixelsPerSecond)
{
    if (isAnimating) { return; }

    distance = animationDistance;
    location = 0.0;
    offset = animationOffset;
    speed = pixelsPerSecond / (1000.0);
    isAnimating = true;
    lastMilliseconds = CurrentMilliseconds();
}

bool Animation::IsAnimating()
{
    return isAnimating;
}

int Animation::Position()
{
    int position = (unsigned int)location;
    if (distance < 0) { position = -position; }
    return position + offset;
}

LONG Animation::CurrentMilliseconds()
{
    SYSTEMTIME time;
    GetSystemTime(&time);
    return (time.wSecond * 1000) + time.wMilliseconds;
}

void Animation::Tick()
{
    LONG currentMilliseconds = CurrentMilliseconds();
    if (currentMilliseconds - lastMilliseconds > 0)
    {
        // Calculate how much we should have moved
        location += speed * (double)(currentMilliseconds - lastMilliseconds);
        if (location >= abs(distance))
        {
            location = abs(distance);
            isAnimating = false;
        }

        // Remeber we moved
        lastMilliseconds = currentMilliseconds;
    }
}
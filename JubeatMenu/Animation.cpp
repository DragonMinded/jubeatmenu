#include <windows.h>
#include "Animation.h"

Animation::Animation()
{
    distance = 0;
    speed = 0.0;
    lastMilliseconds = 0;
    location = 0.0;
    speedChangeLocation = 0.0;
    offset = 0;
    speedChanges = 0;
    isAnimating = false;
}

Animation::~Animation()
{
    // Empty
}

void Animation::Animate(int animationOffset, int animationDistance, double pixelsPerSecond, bool allowDeceleration)
{
    if (isAnimating) { return; }

    distance = animationDistance;
    location = 0.0;
    offset = animationOffset;
    speed = pixelsPerSecond / (1000.0);
    originalSpeed = speed;
    speedChangeLocation = (double)abs(animationDistance) * SPEED_REDUCE_LOCATION;
    speedChanges = allowDeceleration ? 0 : MAX_SPEED_CHANGES;
    isAnimating = true;
    lastMilliseconds = CurrentMilliseconds();
}

bool Animation::IsAnimating()
{
    return isAnimating;
}

void Animation::CancelDeceleration()
{
    if (!isAnimating) { return; }

    speed = originalSpeed;
    speedChanges = MAX_SPEED_CHANGES;
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
    if (currentMilliseconds - lastMilliseconds != 0)
    {
        if (currentMilliseconds - lastMilliseconds < 0)
        {
            // We went backwards?
            lastMilliseconds = currentMilliseconds;
            return;
        }

        // Calculate how much we should have moved
        location += speed * (double)(currentMilliseconds - lastMilliseconds);
        if (location >= abs(distance))
        {
            location = abs(distance);
            isAnimating = false;
        }
        else if (location >= speedChangeLocation && speedChanges < MAX_SPEED_CHANGES)
        {
            speed *= SPEED_REDUCE_PERCENTAGE;
            speedChangeLocation = location + (((double)abs(distance) - location) * SPEED_REDUCE_LOCATION);
            speedChanges ++;
        }

        // Remeber we moved
        lastMilliseconds = currentMilliseconds;
    }
}
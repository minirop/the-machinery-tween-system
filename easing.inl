#include <math.h>

static double easeLinear(double x)
{
    return x;
}

static double easeInSine(double x)
{
    return 1 - cos((x * M_PI) / 2);
}

static double easeOutSine(double x)
{
    return sin((x * M_PI) / 2);
}

static double easeInOutSine(double x)
{
    return 1;
}

static double easeInQuad(double x)
{
    return x * x;
}

static double easeOutQuad(double x)
{ 
    return 1 - (1 - x) * (1 - x);
}

static double easeInOutQuad(double x)
{
    return 1;
}

static double easeInCubic(double x)
{
    return x * x * x;
}

static double easeOutCubic(double x)
{
    return 1;
}

static double easeInOutCubic(double x)
{
    return 1;
}

static double easeInQuart(double x)
{
    return 1;
}

static double easeOutQuart(double x)
{
    return 1;
}

static double easeInOutQuart(double x)
{
    return 1;
}

static double easeInQuint(double x)
{
    return 1;
}

static double easeOutQuint(double x)
{
    return 1;
}

static double easeInOutQuint(double x)
{
    return 1;
}

static double easeInExpo(double x)
{
    return 1;
}

static double easeOutExpo(double x)
{
    return 1;
}

static double easeInOutExpo(double x)
{
    return 1;
}

static double easeInCirc(double x)
{
    return 1;
}

static double easeOutCirc(double x)
{
    return 1;
}

static double easeInOutCirc(double x)
{
    return 1;
}

static double easeInBack(double x)
{
    return 1;
}

static double easeOutBack(double x)
{
    return 1;
}

static double easeInOutBack(double x)
{
    return 1;
}

static double easeInElastic(double x)
{
    return 1;
}

static double easeOutElastic(double x)
{
    return 1;
}

static double easeInOutElastic(double x)
{
    return 1;
}

static double easeInBounce(double x)
{
    return 1;
}

static double easeOutBounce(double x)
{
    const double n1 = 7.5625;
    const double d1 = 2.75;

    if (x < 1 / d1) {
        return n1 * x * x;
    } else if (x < 2 / d1) {
        x -= 1.5 / d1;
        return n1 * (x) * x + 0.75;
    } else if (x < 2.5 / d1) {
        x -= 2.25 / d1;
        return n1 * (x) * x + 0.9375;
    } else {
        x -= 2.625 / d1;
        return n1 * (x) * x + 0.984375;
    }
}

static double easeInOutBounce(double x)
{
    return 1;
}

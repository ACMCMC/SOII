#include <stdlib.h>
#include <stdio.h>

// Aldán Creo Mariño, SOII 20/21

int main() {
    float a, b, c, res, t;
    a = 0.1f;
    b = 0.2f;
    c = 23.0f;
    t=a*b;
    res= t*c;
    printf("(a+b)+c = %f\n", res);
    t=b*c;
    res=a*t;
    printf("(b+c)+a = %f\n", res);
}
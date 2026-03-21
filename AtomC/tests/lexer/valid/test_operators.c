int main()
{
    int a;
    int b;
    a = 1;
    b = a.x;
    b = a + 2;
    b = a - 1;
    b = a * 3;
    b = a / 2;
    if (a == 1) b = 0;
    if (a != 2) b = 1;
    if (a < 3) b = 2;
    if (a <= 3) b = 3;
    if (a > 0) b = 4;
    if (a >= 0) b = 5;
    if (a && b) b = 6;
    if (a || b) b = 7;
    if (!a) b = 8;
    return b;
}

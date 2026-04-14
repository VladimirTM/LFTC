int calc(int a, int b) {
    int r;
    r = a + b * 2;
    r = a - b / 2;
    r = (a + b) * 2;
    if (a == b || a != 0 && !b) {
        r = 1;
    }
    if (a <= b) {
        r = a >= 0;
    }
    return r;
}

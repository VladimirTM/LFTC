struct Point
{
    int x;
    double y;
    char z;
};

void foo()
{
    return;
}

int bar(int n)
{
    if (n > 0)
    {
        return n;
    }
    else
    {
        while (n < 0)
        {
            n = n + 1;
        }
        return n;
    }
}

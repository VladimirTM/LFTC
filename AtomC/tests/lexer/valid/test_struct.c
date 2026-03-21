struct Point
{
    int x;
    int y;
};

int main()
{
    struct Point p;
    p.x = 10;
    p.y = 20;
    int arr[5];
    arr[0] = p.x + p.y;
    return arr[0];
}

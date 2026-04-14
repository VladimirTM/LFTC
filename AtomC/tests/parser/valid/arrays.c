struct Node {
    int val;
};

int sum(int n) {
    int arr[10];
    arr[0] = n;
    arr[1] = arr[0] + 1;
    return arr[0] + arr[1];
}

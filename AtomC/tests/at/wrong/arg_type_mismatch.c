struct S {
    int n;
};
struct S s;
void g(int x) {}
void f() {
    g(s);
}

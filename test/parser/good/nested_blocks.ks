// FIXME we generate wrong tac for this
int main(int a, int b)
{
    int x = 10;
    {
        int x = 20;
        int a = x + 10;
    }
    return x;
}

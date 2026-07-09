int main()
{
    int a = 1;
    {
        int a = 2;
        a = a + 1;
    }
    return a;
}

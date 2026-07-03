int main( int a )
{
    int b = 5;
    {
        int a = 5;
        int c = a + b;
        {
            int b = c + a;
        }
        int b = 4;
    }
    return b;
}
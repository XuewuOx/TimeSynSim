simple Clock
{
    parameters:
        volatile double offset;
        volatile double drift;
        volatile double sigma1;
        volatile double sigma2;
        volatile double sigma3;
        volatile double Tcamp;
    gates:
        input inclock;
        output outclock;
}

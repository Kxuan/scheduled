#include <iostream>
#include <thread>
#include <scheduled.h>

void func(void *pVoid)
{
    for (int i = 0; i < 5; i++)
    {
        std::cout << __FUNCTION__ << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main()
{
    Scheduled<> S;
    S.schedule_now(func);

    for (int i = 0; i < 5; i++)
    {
        std::cout << __FUNCTION__ << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}

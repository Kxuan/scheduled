#include <iostream>
#include <thread>
#include <scheduled.h>

void func(void *pVoid)
{
    std::cout << "func" << std::endl;
}

int main()
{
    Scheduled<> S;
    S.schedule_for(std::chrono::milliseconds(2500), func);

    for (int i = 0; i < 5; i++)
    {
        std::cout << "main" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}

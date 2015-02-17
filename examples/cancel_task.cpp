#include <iostream>
#include <thread>
#include <scheduled.h>

void func(std::string *param)
{
    std::cout << *param << std::endl;
}

int main()
{
    Scheduled<> S;
    S.schedule_for(std::chrono::milliseconds(2500), S.callback_cast(func), std::string("first"));
    {
        auto task = S.schedule_for(std::chrono::milliseconds(1500), S.callback_cast(func), std::string("second"));
        task->cancel();
    }
    for (int i = 0; i < 5; i++)
    {
        std::cout << "main" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}

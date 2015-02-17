#include <iostream>
#include <thread>
#include <scheduled.h>

void func(std::string *p)
{
    for (int i = 0; i < 5; i++)
    {
        std::cout << *p << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main()
{
    Scheduled<> S;
    //if you join thread before any task, the default worker will be disabled.
    S.schedule_now(S.callback_cast(func), std::string("first"));
    S.schedule_now(S.callback_cast(func), std::string("second"));


    S.join();//join the current thread to worker pool
    //you can try the next line (comment S.join()) to see the difference
    //std::this_thread::sleep_for(std::chrono::seconds(11));


    return 0;
}

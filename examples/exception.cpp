#include <iostream>
#include <thread>
#include <scheduled.h>

void func(void *pVoid)
{
    throw std::string("unexception");
}

void catch_func(void *pVoid)
{
    try
    {
        std::rethrow_exception(std::current_exception());
    } catch (std::string const &arg)
    {
        std::cout << "Exception:" << arg << std::endl;
    }
}

int main()
{
    Scheduled<> S;
    S.schedule_now(func, nullptr, nullptr, catch_func);
    S.join();
    return 0;
}

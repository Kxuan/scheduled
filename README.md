# Scheduled
Scheduled can help you to run some code at a specified time.
 - Asynchronous task
 - Delayed task
 - Run multi tasks in same time.

## Version
0.1
## Complie
You must using c++11 standard and enable multithreading for your complier.
For example:
```sh
$ g++ --std=c++11 -pthread async.cpp -o async
```
## Example
### Asynchronous task

```cpp
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
```
Output
```
main
func
mainfunc

main
func
main
func
mainfunc

```
###Simple Delayed Task
```cpp
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
```
Output
```
main
main
main
func
main
main
```
See example/ to get more example.
## License
Apache

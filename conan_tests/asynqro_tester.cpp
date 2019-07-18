#include <asynqro/asynqro>
#include <iostream>

using namespace asynqro;

int main()
{
    auto f = Future<int, int>::successful(5).andThenValue(25.0);
    auto p = Promise<int, int>();
    auto f2 = p.future()
                  .recover([](auto f) { return f; })
                  .recoverWith([](auto f) { return Future<int, int>::failed(f); })
                  .recoverValue(5)
                  .map([](auto) { return 5; })
                  .filter([](auto) { return true; })
                  .flatMap([f](auto) { return f; })
                  .andThen([f]() { return f; })
                  .mapFailure([](auto fail) { return fail; });
    auto finalFuture = (f2 + tasks::run([]() { return 40 + 2; })) >> [](const auto &x) { return std::get<1>(x); };
    p.success(10);
    finalFuture.wait(1000);
    if (!finalFuture.isCompleted()) {
        std::cout << "Error: Future never finished" << std::endl;
        return -1;
    }
    if (!finalFuture.isSucceeded()) {
        std::cout << "Error: Future failed" << std::endl;
        return -1;
    }
    std::cout << "Expected result = 42. Future result = " << finalFuture.result() << "." << std::endl;
    return finalFuture.result() == 42 ? 0 : -1;
}

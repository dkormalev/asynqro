[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Release](https://img.shields.io/github/release/dkormalev/asynqro.svg)](https://github.com/dkormalev/asynqro/releases/latest)
[![Conan](https://api.bintray.com/packages/dkormalev/conan/asynqro%3Adkormalev/images/download.svg)](https://bintray.com/dkormalev/conan/asynqro%3Adkormalev/_latestVersion)<br/>
[![Travis-CI Build Status](https://travis-ci.com/dkormalev/asynqro.svg?branch=develop)](https://travis-ci.com/dkormalev/asynqro)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/dkormalev/asynqro?svg=true&branch=develop)](https://ci.appveyor.com/project/dkormalev/asynqro)<br/>
[![Code Coverage](https://codecov.io/gh/dkormalev/asynqro/branch/master/graph/badge.svg)](https://codecov.io/gh/dkormalev/asynqro)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/dkormalev/asynqro.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/dkormalev/asynqro/context:cpp)

# Asynqro

Asynqro is a small library with purpose to make C++ programming easier by giving developers rich monadic Future API (mostly inspired by Future API in Scala language). This library is another implementation of ideas in https://github.com/opensoft/proofseed (now moved to asynqro usage, for historic purposes check tags before 02/25/19), but has much cleaner API, refined task scheduling logic and is not tied to any framework.

### Dependencies
- **C++17**: Should work with Clang `>=6`, GCC `>=7` and MSVC `>=16`. Tested with Clang9 ([travis](https://travis-ci.com/dkormalev/asynqro)), GCC9 ([travis](https://travis-ci.com/dkormalev/asynqro)) and MSVC16 (VS2019, [appveyor](https://ci.appveyor.com/project/dkormalev/asynqro)). Also tested for compatibility with clang 6.0 and gcc 7.4 on Ubuntu Bionic image (([travis](https://travis-ci.com/dkormalev/asynqro))). Conan packages exist for GCC 7/8/9@Linux, Clang 6/7/8@Linux, Apple-Clang 10/11@MacOSX, MSVC16@Windows.
- **CMake** `>= 3.12.0`
- **GoogleTest**. Will be automatically downloaded during cmake phase.
- **lcov** `>= 1.14`. Used for code coverage calculation, not needed for regular build.
- Optional **Qt5** `>= 5.10`. It is not required though and by default asynqro is built without Qt support. There is no Qt dependency in library itself, but enabling it brings support for Qt containers, adds `Future::fromQtSignal()` and `Future::fromQtFuture()`. Also `Future::wait()` becomes guithread-aware.

Asynqro has two main parts:
- [Future/Promise](#future-promise)
- [Tasks scheduling](#tasks-scheduling)

## Future/Promise
There are already a lot of implementations of Future mechanism in C++:
- `std::future` - no API at all, except very basic operations like retrieve value and wait for it. There is a Concurrency TS with `.then`, but it is still not in the standard.
- `boost::future` - almost the same as `std::future`.
- `QFuture` - Mostly unusable outside of QtConcurrent without Qt private headers because there is no way to fill it from user code.
- `Folly` - Folly futures are also inspired by Scala ones (but different ones, from Twitter framework) and have good API but are a part of huge framework which is too hard to use partially.
- Many others not mentioned here

So why not to create another one?

Future-related part of asynqro contains:
- [Promise](#promise)
- [Future](#future)
- [CancelableFuture](#cancelablefuture)
- [WithFailure](#withfailure)
- [Trampoline](#trampoline)
- [repeat() and repeatForSequence() helpers](#repeat-helpers)

All classes are reentrant and thread-safe.

All higher-order methods are exception-safe. If any exception happens inside function passed to such method, then Future will fail (or task will gracefully stop if it is `runAndForget`).

It is possible to use Future with movable-only classes (except `sequence()`). In this case `resultRef()` should be used instead of `result()`.

Asynqro is intended to be used by including `asynqro/asynqro` header that includes `asynqro/future.h` and `asynqro/tasks.h`. It is also possible to include only `asynqro/futures.h` if task scheduling is not needed. `simplefuture.h` provides simple wrapper with std::any as failure type. All other headers except these three are considered as implementation and should not be included directly.

Good example of customizing Future to specific needs can be found in https://github.com/opensoft/proofseed/blob/develop/include/proofseed/asynqro_extra.h .

### Promise
There is not a lot of methods in this class and its main usage is to generate Future object which later will be filled by this Promise at most one time. All subsequent fills will be ignored.

### Future
This class shouldn't be instantiated directly in users code, but rather is obtained either from Promise or from tasks scheduling part. Also new Future object is returned from all transformation Future methods. It complies with functor and monad laws from FP and provides all operators required by them (`successful`/`failed`, `map`, `flatMap`).

Future is also sort of left-biased EitherT with result type as left side and failure type as right value (to provide failure reason). Sides were chosen non-canonical way (typical Either usually has right side as result and left as error) for compatibility purposes: std::expected type in C++ is sided the same way.

Almost all Future methods returns Future object, so they can be effectively chained. Futures are almost immutable. *Almost* because they will change there state at most one time when they are filled. Adding callbacks doesn't change behavior of other already applied callbacks (i.e. it will not change state of Future and/or its value).

if higher-order method is called on already filled Future it will be called (in case of matching status) immediately in the same thread. If it is not yet filled, it will be put to queue, which will be called (in non-specified order) on Future filling in thread that filled the Future.

#### Future API
- `successful` - `T->Future<T, FailureType>` creates new Future object filled as successful with provided value
- `failed` - `FailureType->Future<T, FailureType>` creates new Future object filled as failed with provided reason
- `fromQtSignal` - `(QObject, Signal)->Future<T, FailureType>` creates new Future object that will be filled when signal emits. `T` here should be either bool or same type as signal parameter (if signal has more than one parameter `T` should be `std::tuple`)
- `fromQtFuture` - `QFuture<OtherT>->Future<T, FailureType>` creates new Future object that will be filled with QFuture result. `OtherT` and `T` must follow next rules:
  - if `OtherT` is `void` then `T` must be `bool`
  - if `T` is `bool` and `OtherT` is not convertible to `bool` then result will always be `true`
  - if `T` is a container of `OtherT` all results from QFuture will be used
  - if nothing above is true then `OtherT` must be convertible to `T` and in this case first result from QFuture will be used
- `wait` - waits for Future to be filled (either as successful or as failed) if it is not yet filled with optional timeout
- `isCompleted`/`isFailed`/`isSucceeded` - returns current state of Future
- `result`/`resultRef`/`failureReason` - returns result of Future or failure reason. Will wait for Future to be filled if it isn't already.
- `onSuccess` - `(T->void)->Future<T, FailureType>` adds a callback for successful case.
- `onFailure` - `(FailureType->void)->Future<T, FailureType>` adds a callback for failure case.
- `filter` - `(T->bool, FailureType)->Future<T, FailureType>` fails Future if function returns false for filled value.
- `map` - `(T->U)->Future<U, FailureType>` transforms Future inner type. Also available as `>>` operator.
- `mapFailure` - `(FailureType->OtherFailureType)->Future<T, OtherFailureType>` transforms Future failure type.
- `flatMap` - `(T->Future<U, FailureType>)->Future<U, FailureType>` transforms Future inner type. Also available as `>>` operator.
- `andThen` - `(void->Future<U, FailureType>)->Future<U, FailureType>` shortcut for flatMap if value of previous Future doesn't matter. Also available as `>>` operator.
- `andThenValue` - `U->Future<U, FailureType>` shortcut for andThen if all we need is to replace value of successful Future with some already known value.
- `innerReduce`/`innerMap`/`innerFilter`/`innerFlatten` - applicable only for Future with sequence inner type. Allows to modify sequence by reducing, mapping, filtering or flattening it.
- `recover` - `(FailureType->T)->Future<T, FailureType>` transform failed Future to successful
- `recoverWith` - `(FailureType->Future<T, FailureType>)->Future<T, FailureType>` the same as recover, but allows to return Future in callback
- `recoverValue` - `T->Future<T, FailureType>` shortcut for recover when we just need to replace with some already known value
- `zip` - `(Future<U, FailureType>, ...) -> Future<std::tuple<T, U, ...>, FailureType>` combines values from different Futures. If any of the Futures already have tuple as inner type, then U will be list of types from this std::tuple (so resulting tuple will be a flattened one). If zipped Futures have different FailureTypes then they will be combined in std::variant (with flattening if some of FailureTypes are already variants). Also available as `+` operator.
- `zipValue` - `U->Future<std::tuple<T, U>, FailureType>` - shortcut for zip with already known value.
- `sequence` - `Sequence<Future<T, FailureType>> -> Future<Sequence<T>, FailureType>` transformation from sequence of Futures to single Future.
- `sequenceWithFailures` - `Sequence<Future<T, FailureType>> -> Future<std::pair<AssocSequence<Sequence::size_type, T>, AssocSequence<Sequence::size_type, FailureType>>, FailureType>` transformation from sequence of Futures to single Future with separate containers for successful Futures and failed ones. `AssocSequence` can be set as optional type parameter.

### CancelableFuture
API of this class is the same as Future API plus `cancel` method, that immediately fills this Future. CancelableFuture can be created only from Promise so it is up to providing side to decide if return value should be cancelable or not. Returning CancelableFuture however doesn't bind to follow cancelation as order, it can be considered as a hint. For example, Network API can return CancelableFuture and cancelation will be provided only for requests that are still in queue.

Providing side can check if Future was canceled by checking if Promise was already filled.

All CancelableFuture methods return simple Future to prevent possible cancelation of original Promise somewhere in downstream.

### WithFailure
It is possible to fail any transformation by using `WithFailure` helper struct.
```cpp
Future<int, std::string> f = /*...*/;
f.flatMap([](int x) -> Future<int, std::string> {
  if (shouldNotPass(x))
    return WithFailure<std::string>("You shall not pass!");
  else
    return asyncCalculation(x);
})
.map([](int x) -> int {
  if (mayItPass(x))
    return 42;
  return WithFailure<std::string>("You shall not pass!");
})
.recover([](const std::string &reason) -> int {
  if (reason.empty())
    return -1;
  return WithFailure<std::string>("You shall not pass, I said.");
});
```
This structure **SHOULD NOT** be saved anyhow and should be used only as a helper to return failure. Implicit casting operator will move from stored failure.

### Trampoline
Using `map()` and other blocking transformations is something where we expect that stack can overflow, because we know that it will be called immediately each after another.

Although, for `flatMap()` or `andThen()` it is definitely not something one can expect due to its pseudo-asynchronous nature. But, in case of lots of flatMaps, it will still overflow on backward filling when last Future is filled.

To avoid such behavior `Trampoline` struct can be used anywhere where Future return is expected. It wraps a Future with extra transformation which will make sure that stack will be reset by moving it to another thread from `Intensive` thread pool.

```cpp
Future<int, std::string> f = /*...*/;
f.flatMap([](int x) -> Future<int, std::string> {
    return Trampoline(asyncCalculation(x));
});
```

## Repeat Helpers
Header `asynqro/repeat.h` contains `asynqro::repeat()` function that allows to do while-loop-styled calls to user function that can return either data or future with this data. User function should return either `Continue` with new set of arguments or `Finish` with final result.

Typical use case for `repeat()` is when it is needed to process something in serial manner, but do so using `Future` mechanism.

`repeat()` signature can be:
 - `((Args...->RepeaterResult<T, Args...>), Args...) -> Future<T, FailureT>` - this form passes `Args` to function while function returns `Continue` with new set of arguments. When function returns `Finish` invocation stops and `repeat()` itself returns `Future` filled with result. This form is blocking. It doesn't do any extra copies of arguments or result if function properly moves everything.
 - `((Args...->RepeaterFutureResult<T, FailureT, Args...>), Args...) -> Future<T, FailureT>` - this form passes `Args` to function and expects Future in return. This Future can be filled either with `Finish` or `Continue` or `TrampolinedContinue`. Third option is the same as regular `Continue` but uses [Trampoline](#trampoline) inside to ensure that stack wouldn't be overflown. This form is non-blocking if function is non-blocking. It doesn't do any extra copies on top of what is done by `flatMap()`.

In case when there is a container with data we need to pass to our function one by one in serial manner, it is better to use `repeatForSequence()`. It accepts container, initial value and `(Data, T)->Future<T, FailureT>` function, where first argument is element from container and second is previous result (or initial value in case of first element). `repeatForSequence()` function returns `Future<T, FailureT>` with either final result or first occurred failure (and will not proceed forward with container values after failed one).

## Tasks scheduling
The same as with futures, there are lots of implementations of task scheduling:
- `Boost.Asio` - Asio is much bigger than just scheduling, but it also provides thread pool with some API for running jobs in it.
- `QtConcurrent` - part of Qt that uses QFuture and provides `run()` for scheduling task somewhere in QThreadPool.
- `Thread-Pool-Cpp` - most simplest possible implementation of thread pool that is extremely fast. It is as basic as it can be though - round-robin on threads with ability to pass queued jobs between them.
- `Folly` - Folly task scheduling using two different executors and MPMC queue is, again as Futures, an awesome solution, but it is a huge framework and can't be easily added only for task scheduling.
- `Intel TBB` - pretty good in performance (lags on some corner cases, see 8+ concurrency in timed reposts, but it is probably some bug that would be fixed eventually), not that good in API and requires a lot of boilerplate if low-level task scheduling needed. TBB is more oriented towards parallel computations (for which it provides solid set of helpers) than general parallel tasks.
- many others not listed here.

Asynqro's task scheduling provides next functionality:
- **Priorities**. Tasks can be prioritized or de-prioritized to control when they should be executed
- **Subpools**. By default all tasks are running in subpool named `Intensive`, it is non-configurable and depends on number of cores in current system. It is, however, only a subpool of whole threads pool available in asynqro and it is possible to create `Custom` subpools with specified size to schedule other tasks (like IO or other mostly waiting operations). `Custom` subpools can also be paused (i.e. moved to 0 capacity with restoring old value on resuming).
- **Thread binding**. It is possible to assign subset of jobs to specific thread so they could use some shared resource that is not thread-safe (like QSqlDatabase for example).
- **Future as return type**. by default task scheduling returns CancelableFuture object that can be used for further work on task result. It also provides ability to cancel task if it is not yet started. It is also possible to specify what failure type should be in this Future by passing TaskRunner specialization to `run` (example can be found in https://github.com/opensoft/proofseed/blob/develop/include/proofseed/asynqro_extra.h).
- **Sequence scheduling**. Asynqro allows to run the same task on sequence of data in specified subpool.
- **Clustering**. Similar to sequence scheduling, but doesn't run each task in new thread. Instead of that divides sequence in clusters and iterates through each cluster in its own thread.
- **Task continuation**. It is possible to return `Future<T>` from task. It will still give `Future<T>` as scheduling result but will fulfill it only when inner Future is filled (without keeping thread occupied of course).
- **Fine tuning**. Some scheduling parameters can be tuned:
  - `Idle amount` - specifies how much empty loops worker should do in case of no tasks available for it before going to wait mode. More idle loops uses more CPU after tasks are done (so it is not really efficient in case of rare tasks) but in case when tasks are scheduled frequently it can be feasible to use bigger idle amount to not let workers sleep. 1024 by default.
  - `Pool capacity` - `qMax(64, INTENSIVE_CAPACITY * 8)` by default.
  - `Thread binding amount` - max amount of threads to be used for thread bound tasks. 1/4 of total pool size by default.
  - `Preheating` - it is possible to *preheat* (i.e. create worker threads) pool in advance. Either whole pool can be preheated or fraction of it.

Limitations:
- Maximum amount of possible threads is 512. It is an artificial limitation that could be lifted in future versions.

## Task scheduling performance
Task scheduling engine should be not only rich in its API but also has good performance in scheduling itself. `benchmarks` directory contains 4 synthetic benchmarks that can show at least some hints about how big the overhead of Asynqro is.

Tests were run few times for each solution on i7 with 4 cores+HT (MacBook Pro 15 mid 2014, i7 2.2GHz). Smallest value was chosen for each case.

Intensive and ThreadBound mean what type of scheduling was used in this suite. In ThreadBound tasks were assigned to amount of cores not bigger than number of logic cores.

If asynqro benchmark is marked with `+F` then it is using `run` function (that returns Future). If it isn't mark so - it uses `runAndForget`. `+F` mark can indirectly show how much overhead Future usage adds. Keep in mind that this overhead is not only about pure Future versus nothing, but also about `run()` logic overhead related to Future filling.

For Intel TBB repost benchmarks there are two different modes.
- enqueue means that all tasks are added using `enqueue()`, which adds them to shared queue. It is comparable to how Intensive tasks scheduling works in asynqro.
- spawn means that all child tasks (i.e. ones that were added from other tasks) are added by `spawn()`, which adds them to current thread queue, which resembles a bit ThreadBound behavior (but for TBB they also can be stolen by other threads).

Ideas behind what exactly each benchmarks measures:
- `timed-repost` - the most **real life** benchmark from the set. It emulates system with a lot of tasks that are being added from multiple threads with some pauses (payload of the task). It allocates these task additions across timeline and shows how system under stress would work.
- `timed-avalanche` - same tasks as in repost, but ALL tasks are added from one main thread. Tasks don't add new tasks anymore. This benchmark is about working with long tasks queue still keeping these payloads to emulate some useful work done in real systems.
- `empty-repost` - same as timed-repost, but without any payload. Equals to extra brutality on concurrent access to shared queue. This benchmark is where asynqro lags behind comparing to Boost.Asio and TBB and can have some improvements. But, as mentioned earlier - such case should rarely happen in real projects.
- `empty-avalanche` - same as timed-avalanche, but without any payload.

These benchmarks are synthetical and it is not an easy thing to properly benchmark such thing as task scheduling especially due to non-exclusive owning of CPU, non-deterministic nature of spinlocks and other stuff, but at least it can be used to say with some approximation how big overhead is gonna be under different amount of load.

Benchmarks listed below were collected with 0.7.0 version.

### empty-avalanche
Big for loop that sends a lot of tasks without any payload (except filling current time of execution) to thread pool. It produces only one result - how many time it took to go through whole list.

System/Jobs                          | 10000   | 100000  | 1000000 | 10000000
------------------------------------ | ------- | ------- | ------- | ----------
asynqro (idle=1000, Intensive)       | 4.44095 | 45.4319 | 455.2   | 4666.12
asynqro (idle=1000, Intensive, +F)   | 9.05857 | 94.1342 | 926.153 | 9508.67
asynqro (idle=1000, ThreadBound)     | 2.98527 | 27.4865 | 270.969 | 2714.79
asynqro (idle=1000, ThreadBound, +F) | 8.98741 | 92.0745 | 908.951 | 9383.99
boostasio                            | 33.7501 | 318.911 | 2955.63 | 30074.4
Intel TBB                            | 3.79893 | 26.0585 | 252.715 | 2545.12
qtconcurrent                         | 131.674 | 1339.33 | 13335.3 | 133160
threadpoolcpp                        | 1.2125  | 4.50206 | 47.2289 | 472.346

### timed-avalanche
The same as empty-avalanche, but in this case tasks are with some payload that tracks time. Each task should be `~0.1ms` of payload. Result in this benchmark is difference between total time and summary of payload time divided by number of cores.

System/Jobs                          | 10000    | 100000  | 1000000
------------------------------------ | -------- | ------- | --------
asynqro (idle=1000, Intensive)       | 6.30689  | 54.6901 | 1701.55
asynqro (idle=1000, Intensive, +F)   | 5.72308  | 229.756 | 8162.24
asynqro (idle=1000, ThreadBound)     | 0.849481 | 7.88768 | 44.8928
asynqro (idle=1000, ThreadBound, +F) | 3.1938   | 23.9305 | 159.716
boostasio                            | 0.920996 | 9.22965 | 105.14
Intel TBB                            | 19.8788  | 185.326 | 1841.44
qtconcurrent                         | 5.66463  | 102.161 | 2437.86
threadpoolcpp                        | 2.7514   | 7.54758 | 18.915

### empty-repost
This benchmark was originally taken from thread-pool-cpp and adapted to qtconcurrent and asynqro usage. It starts C tasks, each of them counts how many times it was sent and if not enough yet (1kk) - sends itself again. Otherwise it reports time spent.
It produces C different results. For each run we take highest one as a result (which actually means how much time it took to run all of them).

System/Concurrency                   | 1       | 2       | 4       | 6       | 8       | 16      | 32
------------------------------------ | ------- | ------- | ------- | ------- | ------- | ------- | -------
asynqro (idle=1, Intensive)          | 3902.78 | 4310    | 8734.67 | 7076.88 | 10073.9 | 22368.9 | 56975
asynqro (idle=1000, Intensive)       | 600.418 | 778.71  | 2284.93 | 8127.4  | 12763.2 | 26450.1 | 47548.6
asynqro (idle=1000, Intensive, +F)   | 1379.67 | 1623.07 | 2103.98 | 8135.33 | 12011.9 | 25870.5 | 58762.1
asynqro (idle=100000, Intensive)     | 546.062 | 758.235 | 2262.83 | 7868.74 | 12336.4 | 26092.8 | 58722.1
asynqro (idle=1, ThreadBound)        | 200.017 | 402.391 | 1225.84 | 2107.63 | 3095.3  | 6473.62 | 12505.2
asynqro (idle=1000, ThreadBound)     | 201.293 | 390.617 | 1132.82 | 2293.56 | 2616.4  | 6108.04 | 12549.6
asynqro (idle=1000, ThreadBound, +F) | 483.751 | 650.649 | 978.401 | 1436.61 | 2235.9  | 4357.5  | 8627.51
asynqro (idle=100000, ThreadBound)   | 202.246 | 409.661 | 1302.81 | 2239.01 | 2623.15 | 6250.3  | 11650.1
boostasio                            | 1493.45 | 1890.09 | 1874.66 | 1809.04 | 2166.56 | 4754.33 | 9756.77
Intel TBB (enqueue)                  | 309.177 | 526.463 | 715.759 | 876.114 | 1062.48 | 1811    | 3339.66
Intel TBB (spawn)                    | 109.763 | 137.671 | 148.276 | 153.028 | 262.128 | 427.955 | 773.134
qtconcurrent                         | 8233.54 | 26872.4 | 48353.2 | 54523.5 | 59111.9 | 118219  | 237817
threadpoolcpp                        | 32.8009 | 33.2034 | 34.945  | 46.963  | 56.2666 | 110.815 | 221.312

### timed-repost
Almost the same as empty-repost, but tasks are filled with payload (the same way as timed-avalanche). Number of task runs for each task is reduced to 100k. Result of benchmark is again difference between total time and summary of payload time divided by number of cores.

ThreadBound asynqro and threadpoolcpp behaves poorly on this benchmark on 10, 12, 14 jobs (i.e. more than cores and not the multiplier) due to nature of these schedulers. Intensive asynqro, boostasio and qtconcurrent worked with similar behavior as 8 and 16 results. These odd results are not included in the table for brevity, but it is something reader should be aware of.

#### payload of `~0.1ms`
System/Concurrency                   | 1       | 2       | 4       | 6       | 8       | 16      | 32
------------------------------------ | ------- | ------- | ------- | ------- | ------- | ------- | -------
asynqro (idle=1, Intensive)          | 393.207 | 412.942 | 913.603 | 655.622 | 121.654 | 206.67  | 385.578
asynqro (idle=1000, Intensive)       | 237.078 | 231.968 | 189.993 | 177.917 | 109.911 | 204.463 | 407.792
asynqro (idle=1000, Intensive, +F)   | 299.721 | 282.969 | 252.367 | 255.991 | 182.743 | 359.916 | 687.301
asynqro (idle=100000, Intensive)     | 77.2099 | 82.6826 | 83.3019 | 99.53   | 117.448 | 220.647 | 408.673
asynqro (idle=1, ThreadBound)        | 27.8577 | 38.1681 | 41.3093 | 45.9538 | 53.8854 | 134.18  | 221.157
asynqro (idle=1000, ThreadBound)     | 27.4433 | 40.0168 | 37.6035 | 46.5768 | 77.7791 | 137.169 | 239.959
asynqro (idle=1000, ThreadBound, +F) | 60.2763 | 71.8109 | 77.6684 | 102.253 | 122.571 | 250.267 | 489.204
asynqro (idle=100000, ThreadBound)   | 27.829  | 41.3547 | 37.5749 | 45.292  | 66.3068 | 117.178 | 239.459
boostasio                            | 178.826 | 195.186 | 216.133 | 225.054 | 40.7238 | 89.3711 | 187.989
Intel TBB (enqueue)                  | 168.437 | 122.945 | 105.613 | 71.8165 | 1494.4  | 2983.28 | 5961.23
Intel TBB (spawn)                    | 159.379 | 100.803 | 65.9654 | 48.5725 | 10189.7 | 10167.9 | 10176.9
qtconcurrent                         | 327.731 | 345.655 | 392.61  | 526.27  | 271.911 | 482.723 | 1131.03
threadpoolcpp                        | 10.3491 | 11.3457 | 11.9767 | 13.9743 | 23.2465 | 35.1778 | 59.5406

#### payload of `~1ms`
System/Concurrency                   | 1       | 2       | 4       | 6       | 8       | 16      | 32
------------------------------------ | ------- | ------- | ------- | ------- | ------- | ------- | -------
asynqro (idle=1, Intensive)          | 345.436 | 480.884 | 1032.17 | 700.264 | 193.722 | 231.541 | 442.502
asynqro (idle=1000, Intensive)       | 333.487 | 366.489 | 500.979 | 460.211 | 187.748 | 225.557 | 454.981
asynqro (idle=1000, Intensive, +F)   | 371.643 | 404.025 | 582.341 | 508.068 | 275.083 | 398.329 | 760.355
asynqro (idle=100000, Intensive)     | 153.605 | 135.368 | 119.951 | 186.767 | 181.475 | 239.224 | 460.824
asynqro (idle=1, ThreadBound)        | 33.8516 | 46.8522 | 45.2664 | 60.2888 | 132.012 | 228.833 | 337.351
asynqro (idle=1000, ThreadBound)     | 33.3664 | 45.2354 | 45.1481 | 65.5007 | 159.857 | 249.508 | 368.766
asynqro (idle=1000, ThreadBound, +F) | 68.4836 | 77.18   | 90.0423 | 117.263 | 222.851 | 346.748 | 672.402
asynqro (idle=100000, ThreadBound)   | 33.675  | 46.0016 | 45.582  | 59.6052 | 128.506 | 195.254 | 330.591
boostasio                            | 151.232 | 165.312 | 213.826 | 234.668 | 66.3717 | 75.0968 | 137.803
Intel TBB (enqueue)                  | 193.201 | 136.318 | 128.363 | 111.218 | 14432.8 | 28828.8 | 57611.2
Intel TBB (spawn)                    | 183.936 | 121.877 | 101.83  | 87.967  | 100227  | 100253  | 100354
qtconcurrent                         | 275.124 | 295.076 | 382.866 | 464.971 | 249.251 | 476.755 | 926.302
threadpoolcpp                        | 17.4312 | 18.4445 | 20.1527 | 19.9876 | 47.1506 | 57.6971 | 88.9992

## Examples

### Student in library
Let's say we need to authenticate student in library system, and after that fetch list of books she loaned with extra info about each of them. We also will need to fetch personalized suggestions and show them with list of books to return. However we know that there is a bug in suggestions and sometimes it can return book with age restriction higher than users age, so we need to filter them out.

We already have library system API designed as class that returns Future for each request.

We need to emit a signal loanedBooksFetched with loaned books list and suggestionsFetched with suggestions list. We can't, however send list of Book objects directly to QML, we need to transform it to QVariantList using static Book method.

We need to return resulting `Future<bool>` to know when everything is loaded or if any error occurred.

```cpp
Future<bool, MyFailure> Worker::fetchData(QString username, QString password)
{
  return api->authenticate(username, password).flatMap([this](const User &userInfo) {
    auto taken = api->fetchTakenBooks().flatMap([this](const QVector<QString> &bookIds) {
      QVector<Future<Book>> result;
      result.reserve(bookIds.count());
      for (const QString &id : bookIds)
        result << api->fetchBook(id);
      return Future<Book>::sequence(result);
    })
    .map([this](const QVector<Book> &books) { return Book::qmled(books); })
    .onSuccess([this](const QVariantList &books) { emit loanedBooksFetched(books); });

    auto suggestions = api->fetchSuggestions().innerFilter([userInfo](const Book &book) {
      return book->ageRestriction < userInfo.age;
    })
    .map([](const QVector<Book> &books) { return Book::qmled(books); })
    .onSuccess([this](const QVariantList &books) { emit suggestionsFetched(books); });

    return taken.zip(suggestions).andThenValue(true);
  });
}
```

### Repeat
We have some data that we need to send to our API and we want to do few retries before we decide that it is not possible to send it now.

```cpp
using DataSendRepeater = RepeaterFutureResult<bool, std::string, int>;
Future<bool, std::string> Worker::sendData(Data data, int retries)
{
  return repeat<bool, int>([data](int retriesLeft) -> DataSendRepeater {
    if (retries < 0)
      return Future<bool, MyFailure>::failed("Too many retries");
    return api->sendData(data)
      .map([](bool result) -> DataSendRepeater::Value { return Finish(result); }
      .recover([](const auto &) -> DataSendRepeater::Value { return TrampolinedContinue(retries - 1); };
  }, retries);
}
```

### Repeat for known sequence
We have input sequence that we need to process serially in determined order. If error occurs during calculation - it will fastfail.

```cpp
Future<double, std::string> Worker::blackBox(int value, double accumulator);

Future<double, std::string> Worker::calculate(std::vector<int> data)
{
  return repeatForSequence(data, 0.0, [](int x, double result) -> Future<double, std::string> {
    return blackBox(x, result) >> [](double result){ return result < 0.0 ? 0.0 : result; };
  });
}
```

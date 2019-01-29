[![Travis-CI Build Status](https://travis-ci.com/dkormalev/asynqro.svg?branch=develop)](https://travis-ci.com/dkormalev/asynqro)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/dkormalev/asynqro?svg=true&branch=develop)](https://ci.appveyor.com/project/dkormalev/asynqro)
[![Code Coverage](https://codecov.io/gh/dkormalev/asynqro/branch/master/graph/badge.svg)](https://codecov.io/gh/dkormalev/asynqro)
[![Release](https://img.shields.io/github/release/dkormalev/asynqro.svg)](https://github.com/dkormalev/asynqro/releases/latest)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# Asynqro

Asynqro is a small library with purpose to make C++/Qt programming easier by giving developers rich  monadic Future API (mostly inspired by Future API in Scala language). This library is another implementation of ideas in https://github.com/opensoft/proofseed, but has much cleaner API, refined task scheduling logic and is not tied to any framework.

### Dependencies
- **C++17**: tested with Clang7 (travis), GCC8 (travis) and MSVC17 (appveyor)
- **Qt5** `>= 5.10` (technically even something like 5.6 will work, but some tests use `QThread::create` from 5.10)
- **CMake** `>= 3.12.0`
- **GoogleTest**. Will be automatically downloaded during cmake phase
- **lcov**. 1.13 from github or 1.13-4 from debian is not enough. Should contain [1e0df57](https://github.com/linux-test-project/lcov/commit/1e0df571198229b4701100ce5f596cf1658ede4b) commit. Used for code coverage calculation, not needed for regular build

Asynqro has two main parts:
- Future/Promise
- Tasks scheduling

## Future/Promise
There are already a lot of implementations of Future mechanism in C++:
- `std::future` - no API at all, except very basic operations like retrieve value and wait for it. There is a Concurrency TS with `.then`, but it is still not in the standard.
- `boost::future` - almost the same as `std::future`.
- `QFuture` - Mostly unusable outside of QtConcurrent without Qt private headers because there is no way to fill it from user code.
- `Folly` - Folly futures are also inspired by Scala ones (but different ones, from Twitter framework) and have good API but are a part of huge framework which is too hard to use partially.
- `ProofSeed` - this one is also a part of rather big framework and mostly unusable outside of it due to reasons like non-portable Failure type.
- Many others not mentioned here

So why not to create another one?

Future-related part of asynqro contains next classes:
- `Promise`
- `Future`
- `CancelableFuture`

All classes are reentrant and thread-safe.

All higher-order methods are exception-safe. If any exception happens inside function passed to such method, then Future will fail (or task will gracefully stop if it is `runAndForget`).

It is possible to use Future with movable-only classes (except `sequence()`). In this case `resultRef()` should be used instead of `result()`.

Asynqro is intended to be used by including `asynqro/asynqro` header that includes `asynqro/future.h` and `asynqro/tasks.h`. It is also possible to include only `asynqro/futures.h` if task scheduling is not needed. All other headers except these three are considered as implementation and should not be included directly.

### Promise
There is not a lot of methods in this class and its main usage is to generate Future object which later will be filled by this Promise at most one time. All subsequent fills will be ignored.

### Future
This class shouldn't be instantiated directly in users code, but rather is obtained either from Promise or from tasks scheduling part. Also new Future object is returned from all transformation Future methods. It complies with functor and monad laws from FP and provides all operators required by them (`successful`/`failed`, `map`, `flatMap`).

Future is also sort of right-biased EitherT with result type as right side and `QVariant` as left value (to provide failure reason).

Almost all Future methods returns Future object, so they can be effectively chained. Futures are almost immutable. *Almost* because they will change there state at most one time when they are filled. Adding callbacks doesn't change behavior of other already applied callbacks (i.e. it will not change state of Future and/or its value).

if higher-order method is called on already filled Future it will be called (in case of matching status) immediately in the same thread. If it is not yet filled, it will be put to queue, which will be called (in non-specified order) on Future filling in thread that filled the Future.

#### Future API
- `successful` - `T->Future<T>` creates new Future object filled as successful with provided value
- `failed` - `QVariant->Future<T>` creates new Future object filled as failed with provided reason
- `wait` - waits for Future to be filled (either as successful or as failed) if it is not yet filled with optional timeout
- `isCompleted`/`isFailed`/`isSucceeded` - returns current state of Future
- `result`/`resultRef`/`failureReason` - returns result of Future or failure reason. Will wait for Future to be filled if it isn't already.
- `onSuccess` - `(T->void)->Future<T>` adds a callback for successful case.
- `onFailure` - `(QVariant->void)->Future<T>` adds a callback for failure case.
- `filter` - `(T->bool, QVariant)->Future<T>` fails Future if function returns false for filled value.
- `map` - `(T->U)->Future<U>` transforms Future inner type.
- `flatMap` - `(T->Future<U>)->Future<U>` transforms Future inner type.
- `andThen` - `(void->Future<U>)->Future<U>` shortcut for flatMap if value of previous Future doesn't matter.
- `andThenValue` - `U->Future<U>` shortcut for andThen if all we need is to replace value of successful Future with some already known value.
- `innerReduce`/`innerMap`/`innerFilter`/`innerFlatten` - applicable only for Future with sequence inner type. Allows to modify sequence by reducing, mapping, filtering or flattening it.
- `recover` - `(QVariant->T)->Future<T>` transform failed Future to successful
- `recoverWith` - `(QVariant->Future<T>)->Future<T>` the same as recover, but allows to return Future in callback
- `recoverValue` - `T->Future<T>` shortcut for recover when we just need to replace with some already known value
- `zip` - `(Future<U>, ...) -> Future<std::tuple<T, U, ...>>` combines values from different Futures. If any of the Futures already have tuple as inner type, then U will be list of types from this std::tuple (so resulting tuple will be a flattened one).
- `zipValue` - `U->Future<std::tuple<T, U>>` - shortcut for zip
- `sequence` - `Sequence<Future<T>> -> Future<Sequence<T>>` transformation from sequence of Futures to single Future.

### CancelableFuture
API of this class is the same as Future API plus `cancel` method, that immediately fills this Future. CancelableFuture can be created only from Promise so it is up to providing side to decide if return value should be cancelable or not. Returning CancelableFuture however doesn't bind to follow cancelation as order, it can be considered as a hint. For example, Network API can return CancelableFuture and cancelation will be provided only for requests that are still in queue.

Providing side can check if Future was canceled by checking if Promise was already filled.

All CancelableFuture methods return simple Future to prevent possible cancelation of original Promise somewhere in downstream.

### WithFailure
It is possible to fail any transformation by using `WithFailure` helper struct.
```cpp
Future<int> f = /*...*/;
f.flatMap([](int x) -> Future<int> {
  if (shouldNotPass(x))
    return WithFailure("You shall not pass!");
  else
    return asyncCalculation(x);
})
.map([](int x) -> int {
  if (mayItPass(x))
    return 42;
  return WithFailure("You shall not pass!");
})
.recover([](const QVariant &reason) -> int {
  if (reason.toInt())
    return reason.toInt();
  return WithFailure("You shall not pass, I said.");
});
```

## Tasks scheduling
The same as with futures, there are lots of implementations of task scheduling:
- `Boost.Asio` - Asio is much bigger than just scheduling, but it also provides thread pool with some API for running jobs in it
- `QtConcurrent` - part of Qt that uses QFuture and provides `run()` for scheduling task somewhere in QThreadPool.
- `Thread-Pool-Cpp` - most simplest possible implementation of thread pool that is extremely fast. It is as basic as it can be though - round-robin on threads with ability to pass queued jobs between them.
- `Folly` - Folly task scheduling using two different executors and MPMC queue is, again as Futures, an awesome solution, but it is a huge framework and can't be easily added only for task scheduling.
- many others not listed here.

Asynqro's task scheduling provides next functionality:
- **Priorities**. Tasks can be prioritized or de-prioritized to control when they should be executed
- **Subpools**. By default all tasks are running in subpool named `Intensive`, it is non-configurable and depends on number of cores in current system. It is, however, only a subpool of whole threads pool available in asynqro and it is possible to create `Custom` subpools with specified size to schedule other tasks (like IO or other mostly waiting operations).
- **Thread binding**. It is possible to assign subset of jobs to specific thread so they could use some shared resource that is not thread-safe (like QSqlDatabase for example).
- **Future as return type**. by default task scheduling returns CancelableFuture object that can be used for further work on task result. It also provides ability to cancel task if it is not yet started.
- **Sequence scheduling**. Asynqro allows to run the same task on sequence of data in specified subpool.
- **Clustering**. Similar to sequence scheduling, but doesn't run each task in new thread. Instead of that divides sequence in clusters and iterates through each cluster in its own thread.
- **Task continuation**. It is possible to return `Future<T>` from task. It will still give `Future<T>` as scheduling result but will fulfill it only when inner Future is filled (without keeping thread occupied of course).
- **Fine tuning**. Some scheduling parameters can be tuned:
  - `Idle amount` - specifies how much empty loops worker should do in case of no tasks available for it before going to wait mode. More idle loops uses more CPU after tasks are done (so it is not really efficient in case of rare tasks) but in case when tasks are scheduled frequently it can be feasible to use bigger idle amount to not let workers sleep. 1024 by default.
  - `Pool capacity` - `qMax(64, INTENSIVE_CAPACITY * 8)` by default.
  - `Thread binding amount` - max amount of threads to be used for thread bound tasks. 1/4 of total pool size by default.
  - `Preheating` - it is possible to *preheat* (i.e. create worker threads) pool in advance. Either whole pool can be preheated or fraction of it.


## Task scheduling performance
Task scheduling engine should be not only rich in its API but also has good performance in scheduling itself. `benchmarks` directory contains 4 synthetic benchmarks that can show at least some hints about how big the overhead of Asynqro is.

Tests were run few times for each solution on i7 with 4 cores+HT. Smallest one was chosen for each case.

Intensive and ThreadBound mean what type of scheduling was used in this suite. In ThreadBound tasks were assigned to amount of cores not bigger than number of logic cores. All asynqro benchmarks in this section use `runAndForget` function because we don't really need resulting Future. For Future usage overhead please see **Futures usage overhead** section below.

These benchmarks are synthetical and it is not an easy thing to properly benchmark such thing as task scheduling especially due to non-exclusive owning of CPU, non-deterministic nature of spinlocks and other stuff, but at least it can be used to say with some approximation how big overhead is gonna be under different amount of load.

### empty-avalanche
Big for loop that sends a lot of tasks without any payload (except filling current time of execution) to thread pool. It produces only one result - how many time it took to go through whole list.

System/Jobs         | 10000 | 100000 | 1000000 | 10000000
------------------- | ----- | ------ | ------- | ----------
asynqro (idle=1000) | 12.6493 | 105.028 | 1002.89 | 10114.7
boostasio           | 33.7501 | 318.911 | 2955.63 | 30074.4
qtconcurrent        | 131.674 | 1339.33 | 13335.3 | 133160
threadpoolcpp       | 1.2125 | 4.50206 | 47.2289 | 472.346

### timed-avalanche
The same as empty-avalanche, but in this case tasks are with some payload that tracks time. Each task should be `~0.1ms` of payload. Result in this benchmark is difference between total time and summary of payload time divided by number of cores.

System/Jobs         | 10000 | 100000 | 1000000
------------------- | ----- | ------ | --------
asynqro (idle=1000) | 5.97616 | 74.2673 | 1732.07
boostasio           | 0.920996 | 9.22965 | 105.14
qtconcurrent        | 5.66463 | 102.161 | 2437.86
threadpoolcpp       | 2.7514 | 7.54758 | 18.915

### empty-repost
This benchmark was originally taken from thread-pool-cpp and adapted to qtconcurrent and asynqro usage. It starts C tasks, each of them counts how many times it was sent and if not enough yet (1kk) - sends itself again. Otherwise it reports time spent.
It produces C different results. For each run we take highest one as a result (which actually means how much time it took to run all of them).

System/Concurrency               | 1 | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 32
-------------------------------- | - | - | - | - | - | -- | -- | -- | -- | ---
asynqro (idle=1, Intensive) | 5353.89 | 5763.91 | 9888.5 | 12130.2 | 16577.1 | 24114.1 | 28217.3 | 32611.3 | 38594.7 | 76699.8
asynqro (idle=100, Intensive) | 1370.49 | 2460.94 | 5010.37 | 12003.2 | 18423.1 | 23533.1 | 28593.2 | 34074.2 | 38433 | 78832.8
asynqro (idle=100000, Intensive) | 1193.05 | 2096.02 | 5143.41 | 12687.8 | 18437.9 | 23464.5 | 28962.2 | 34065.5 | 36348.8 | 83769.1
asynqro (idle=1, ThreadBound) | 336.57 | 758.194 | 2341.35 | 4735.5 | 6820.04 | 7415.41 | 8516.27 | 9944.06 | 9728.54 | 23180.2
asynqro (idle=100, ThreadBound) | 346.104 | 744.536 | 2234.7 | 4839.15 | 8156.63 | 7937.88 | 8004.71 | 10105.1 | 11659.9 | 21744.1
asynqro (idle=100000, ThreadBound) | 336.549 | 757.518 | 2302.28 | 4799.63 | 7830.33 | 7776.51 | 7814.05 | 8958.35 | 11464.2 | 22830.4
boostasio | 1493.45 | 1890.09 | 1874.66 | 1809.04 | 2166.56 | 2777.52 | 3393.07 | 3998.14 | 4754.33 | 9756.77
qtconcurrent | 8233.54 | 26872.4 | 48353.2 | 54523.5 | 59111.9 | 74712.8 | 90037.1 | 105077 | 118219 | 237817
threadpoolcpp | 32.8009 | 33.2034 | 34.945 | 46.963 | 56.2666 | 81.753 | 86.2359 | 99.6014 | 110.815 | 221.312

### timed-repost
Almost the same as empty-repost, but tasks are filled with payload (the same way as timed-avalanche). Number of task runs for each task is reduced to 100k. Result of benchmark is again difference between total time and summary of payload time divided by number of cores.

#### payload of `~0.1ms`
System/Concurrency               | 1 | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 32
-------------------------------- | - | - | - | - | - | -- | -- | -- | -- | ---
asynqro (idle=1, Intensive) | 537.301 | 557.748 | 959.085 | 745.207 | 155.604 | 249.539 | 293.496 | 342.494 | 307.886 | 592.885
asynqro (idle=100, Intensive) | 531.53 | 558.306 | 679.251 | 525.038 | 159.378 | 253.372 | 289.485 | 343.119 | 301.704 | 609.566
asynqro (idle=100000, Intensive) | 133.554 | 154.672 | 184.165 | 202.536 | 156.459 | 244.286 | 297.066 | 340.463 | 296.144 | 591.354
asynqro (idle=1, ThreadBound) | 39.3806 | 43.4934 | 52.3768 | 67.5977 | 101.586 | 7630.54 | 5110.64 | 2648.81 | 155.436 | 333.36
asynqro (idle=100, ThreadBound) | 38.3216 | 44.1283 | 53.2152 | 64.5184 | 98.2639 | 7625.08 | 5130.63 | 2657.44 | 166.556 | 320.163
asynqro (idle=100000, ThreadBound) | 38.4546 | 46.1561 | 49.886 | 64.4978 | 87.2628 | 7624.41 | 5131.11 | 2655.14 | 176.926 | 325.915
boostasio | 178.826 | 195.186 | 216.133 | 225.054 | 40.7238 | 75.1923 | 70.2192 | 73.9517 | 89.3711 | 187.989
qtconcurrent | 327.731 | 345.655 | 392.61 | 526.27 | 271.911 | 379.417 | 408.333 | 521.243 | 482.723 | 1131.03
threadpoolcpp | 10.3491 | 11.3457 | 11.9767 | 13.9743 | 23.2465 | 4996.5 | 4394.37 | 2216.7 | 35.1778 | 59.5406

#### payload of `~1ms`
System/Concurrency               | 1 | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 32
-------------------------------- | - | - | - | - | - | -- | -- | -- | -- | ---
asynqro (idle=1, Intensive) | 568.14 | 1208.6 | 987.005 | 758.494 | 174.452 | 262.535 | 308.387 | 363.442 | 325.008 | 649.104
asynqro (idle=100, Intensive) | 520.039 | 612.489 | 719.955 | 577.964 | 190.635 | 267.656 | 307.134 | 346.274 | 318.082 | 641.526
asynqro (idle=100000, Intensive) | 179.514 | 183.64 | 213.76 | 239.221 | 177.692 | 255.913 | 292.44 | 347.046 | 310.695 | 634.352
asynqro (idle=1, ThreadBound) | 43.1422 | 47.8293 | 56.1412 | 70.7591 | 117.237 | 75167.2 | 50152.3 | 25211.7 | 192.027 | 398.606
asynqro (idle=100, ThreadBound) | 43.3436 | 47.8513 | 54.5134 | 69.915 | 121.487 | 75132.2 | 50175 | 25160.5 | 224.238 | 407.352
asynqro (idle=100000, ThreadBound) | 42.7681 | 47.9518 | 56.9241 | 71.9153 | 109.301 | 75137.1 | 50145.2 | 25199.9 | 228.498 | 392.165
boostasio | 151.232 | 165.312 | 213.826 | 234.668 | 66.3717 | 56.8949 | 65.7925 | 73.4958 | 75.0968 | 137.803
qtconcurrent | 275.124 | 295.076 | 382.866 | 464.971 | 249.251 | 347.116 | 525.848 | 593.964 | 476.755 | 926.302
threadpoolcpp | 17.4312 | 18.4445 | 20.1527 | 19.9876 | 47.1506 | 49977.7 | 43778.4 | 23450 | 57.6971 | 88.9992

## Futures usage overhead
We can also measure overhead of using futures in task scheduling by running the same benchmarks for asynqro with `run` and `runAndForget` and compare them. Idle amount is 1000 in all tests.

### empty-avalanche
Flavor/Jobs             | 100000  | 1000000
----------------------- | ------- | --------
Intensive, no futures | 106.635 | 1056.49
Intensive, with futures | 195.451 | 1943.28
ThreadBound, no futures | 94.9389 | 931.258
ThreadBound, with futures | 181.489 | 1869.79

### timed-avalanche with 0.1ms
Flavor/Jobs             | 10000   | 100000
----------------------- | ------- | --------
Intensive, no futures | 11.8109 | 64.3695
Intensive, with futures | 14.7544 | 298.561
ThreadBound, no futures | 2.95714 | 20.1497
ThreadBound, with futures | 5.2423  | 37.5535

### empty-repost with 100k tasks
Flavor/Concurrency      | 1 | 4 | 8 | 16
----------------------- | - | - | - | ---
Intensive, no futures | 122.806 | 526.092 | 1671.87 | 3414.7
Intensive, with futures | 302.854 | 545.561 | 1705.33 | 3650.32
ThreadBound, no futures | 32.0795 | 196.733 | 780.659 | 974.038
ThreadBound, with futures | 64.0556 | 171.66 | 520.588 | 768.036

### empty-repost with 1kk tasks
Flavor/Concurrency      | 1 | 4 | 8 | 16
----------------------- | - | - | - | ---
Intensive, no futures | 1195.36 | 5246.65 | 17682.6 | 33589
Intensive, with futures | 2816.33 | 5351.31 | 17028.6 | 36146.4
ThreadBound, no futures | 317.3 | 1968.22 | 6861.81 | 9625.58
ThreadBound, with futures | 664.155 | 1779.41 | 5361.86 | 8048.08

### timed-repost with 10k tasks of 0.1ms each
Flavor/Concurrency      | 1 | 4 | 8 | 16
----------------------- | - | - | - | ---
Intensive, no futures | 24.5251 | 24.6166 | 20.0933 | 37.4036
Intensive, with futures | 33.1252 | 39.9258 | 29.6381 | 51.7628
ThreadBound, no futures | 4.03977 | 5.34057 | 16.3515 | 23.1206
ThreadBound, with futures | 7.89128 | 10.787 | 18.1974 | 32.0577

### timed-repost with 100k tasks of 0.1ms each
Flavor/Concurrency      | 1 | 4 | 8 | 16
----------------------- | - | - | - | ---
Intensive, no futures | 247.519 | 243.835 | 167.819 | 301.238
Intensive, with futures | 345.05 | 366.752 | 238.314 | 453.048
ThreadBound, no futures | 39.0278 | 52.2347 | 103.544 | 179.148
ThreadBound, with futures | 76.5194 | 93.4037 | 164.959 | 305.991

## Examples

### Student in library
Let's say we need to authenticate student in library system, and after that fetch list of books she loaned with extra info about each of them. We also will need to fetch personalized suggestions and show them with list of books to return. However we know that there is a bug in suggestions and sometimes it can return book with age restriction higher than users age, so we need to filter them out.

We already have library system API designed as class that returns Future for each request.

We need to emit a signal loanedBooksFetched with loaned books list and suggestionsFetched with suggestions list. We can't, however send list of Book objects directly to QML, we need to transform it to QVariantList using static Book method.

We need to return resulting `Future<bool>` to know when everything is loaded or if any error occurred.

```cpp
Future<bool> Worker::fetchData(QString username, QString password)
{
  return api->authenticate(username, password).flatMap([this](const User &userInfo) {
    auto taken = api->fetchTakenBooks().flatMap([this](const QVector<QString> &bookIds) {
      QVector<Future<Book>> result;
      result.reserve(bookIds.count());
      for (const QString &id : bookIds)
        result << api->fetchBook(id);
      return Future<>::sequence(result);
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


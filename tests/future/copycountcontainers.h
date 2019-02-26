#ifndef COPYCOUNTCONTAINERS_H
#define COPYCOUNTCONTAINERS_H

#ifdef ASYNQRO_QT_SUPPORT
#    include <QLinkedList>
#    include <QList>
#    include <QVector>
#endif

#include <atomic>
#include <list>
#include <vector>

#ifdef ASYNQRO_QT_SUPPORT
template <typename T>
class CopyCountQVector : public QVector<T>
{
public:
    CopyCountQVector() : QVector<T>() { ++createCounter; }
    CopyCountQVector(const CopyCountQVector &other) : QVector<T>(other) { ++copyCounter; }
    CopyCountQVector &operator=(const CopyCountQVector &other)
    {
        return static_cast<CopyCountQVector &>(QVector<T>::operator=(other));
        ++copyCounter;
    }
    CopyCountQVector(CopyCountQVector &&other) : QVector<T>(std::move(other)) {}
    CopyCountQVector &operator=(CopyCountQVector &&other)
    {
        return static_cast<CopyCountQVector &>(QVector<T>::operator=(std::move(other)));
    }

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};

template <typename T>
class CopyCountQList : public QList<T>
{
public:
    CopyCountQList() : QList<T>() { ++createCounter; }
    CopyCountQList(const CopyCountQList &other) : QList<T>(other) { ++copyCounter; }
    CopyCountQList &operator=(const CopyCountQList &other)
    {
        return static_cast<CopyCountQList &>(QList<T>::operator=(other));
        ++copyCounter;
    }
    CopyCountQList(CopyCountQList &&other) : QList<T>(std::move(other)) {}
    CopyCountQList &operator=(CopyCountQList &&other)
    {
        return static_cast<CopyCountQList &>(QList<T>::operator=(std::move(other)));
    }

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};

template <typename T>
class CopyCountQLinkedList : public QLinkedList<T>
{
public:
    CopyCountQLinkedList() : QLinkedList<T>() { ++createCounter; }
    CopyCountQLinkedList(const CopyCountQLinkedList &other) : QLinkedList<T>(other) { ++copyCounter; }
    CopyCountQLinkedList &operator=(const CopyCountQLinkedList &other)
    {
        return static_cast<CopyCountQLinkedList &>(QLinkedList<T>::operator=(other));
        ++copyCounter;
    }
    CopyCountQLinkedList(CopyCountQLinkedList &&other) : QLinkedList<T>(std::move(other)) {}
    CopyCountQLinkedList &operator=(CopyCountQLinkedList &&other)
    {
        return static_cast<CopyCountQLinkedList &>(QLinkedList<T>::operator=(std::move(other)));
    }

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};
#endif

template <typename T>
class CopyCountVector : public std::vector<T>
{
public:
    CopyCountVector() : std::vector<T>() { ++createCounter; }
    CopyCountVector(const CopyCountVector &other) : std::vector<T>(other) { ++copyCounter; }
    CopyCountVector &operator=(const CopyCountVector &other)
    {
        return static_cast<CopyCountVector &>(std::vector<T>::operator=(other));
        ++copyCounter;
    }
    CopyCountVector(CopyCountVector &&other) : std::vector<T>(std::move(other)) {}
    CopyCountVector &operator=(CopyCountVector &&other)
    {
        return static_cast<CopyCountVector &>(std::vector<T>::operator=(std::move(other)));
    }

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};

template <typename T>
class CopyCountList : public std::list<T>
{
public:
    CopyCountList() : std::list<T>() { ++createCounter; }
    CopyCountList(const CopyCountList &other) : std::list<T>(other) { ++copyCounter; }
    CopyCountList &operator=(const CopyCountList &other)
    {
        return static_cast<CopyCountList &>(std::list<T>::operator=(other));
        ++copyCounter;
    }
    CopyCountList(CopyCountList &&other) : std::list<T>(std::move(other)) {}
    CopyCountList &operator=(CopyCountList &&other)
    {
        return static_cast<CopyCountList &>(std::list<T>::operator=(std::move(other)));
    }

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};
#endif // COPYCOUNTCONTAINERS_H

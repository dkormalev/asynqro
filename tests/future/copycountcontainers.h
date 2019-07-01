#ifndef COPYCOUNTCONTAINERS_H
#define COPYCOUNTCONTAINERS_H

#ifdef ASYNQRO_QT_SUPPORT
#    include <QHash>
#    include <QLinkedList>
#    include <QList>
#    include <QMap>
#    include <QVector>
#endif

#include <atomic>
#include <list>
#include <map>
#include <unordered_map>
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
        ++copyCounter;
        return static_cast<CopyCountQVector &>(QVector<T>::operator=(other));
    }
    CopyCountQVector(CopyCountQVector &&other) : QVector<T>(std::move(other)) {}
    CopyCountQVector &operator=(CopyCountQVector &&other)
    {
        return static_cast<CopyCountQVector &>(QVector<T>::operator=(std::move(other)));
    }
    ~CopyCountQVector() = default;

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
        ++copyCounter;
        return static_cast<CopyCountQList &>(QList<T>::operator=(other));
    }
    CopyCountQList(CopyCountQList &&other) : QList<T>(std::move(other)) {}
    CopyCountQList &operator=(CopyCountQList &&other)
    {
        return static_cast<CopyCountQList &>(QList<T>::operator=(std::move(other)));
    }
    ~CopyCountQList() = default;

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
        ++copyCounter;
        return static_cast<CopyCountQLinkedList &>(QLinkedList<T>::operator=(other));
    }
    CopyCountQLinkedList(CopyCountQLinkedList &&other) : QLinkedList<T>(std::move(other)) {}
    CopyCountQLinkedList &operator=(CopyCountQLinkedList &&other)
    {
        return static_cast<CopyCountQLinkedList &>(QLinkedList<T>::operator=(std::move(other)));
    }
    ~CopyCountQLinkedList() = default;

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};

template <typename T, typename U>
class CopyCountQMap : public QMap<T, U>
{
public:
    CopyCountQMap() : QMap<T, U>() { ++createCounter; }
    CopyCountQMap(const CopyCountQMap &other) : QMap<T, U>(other) { ++copyCounter; }
    CopyCountQMap &operator=(const CopyCountQMap &other)
    {
        ++copyCounter;
        return static_cast<CopyCountQMap &>(QMap<T, U>::operator=(other));
    }
    CopyCountQMap(CopyCountQMap &&other) : QMap<T, U>(std::move(other)) {}
    CopyCountQMap &operator=(CopyCountQMap &&other)
    {
        return static_cast<CopyCountQMap &>(QMap<T, U>::operator=(std::move(other)));
    }
    ~CopyCountQMap() = default;

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};

template <typename T, typename U>
class CopyCountQHash : public QHash<T, U>
{
public:
    CopyCountQHash() : QHash<T, U>() { ++createCounter; }
    CopyCountQHash(const CopyCountQHash &other) : QHash<T, U>(other) { ++copyCounter; }
    CopyCountQHash &operator=(const CopyCountQHash &other)
    {
        ++copyCounter;
        return static_cast<CopyCountQHash &>(QHash<T, U>::operator=(other));
    }
    CopyCountQHash(CopyCountQHash &&other) : QHash<T, U>(std::move(other)) {}
    CopyCountQHash &operator=(CopyCountQHash &&other)
    {
        return static_cast<CopyCountQHash &>(QHash<T, U>::operator=(std::move(other)));
    }
    ~CopyCountQHash() = default;

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
        ++copyCounter;
        return static_cast<CopyCountVector &>(std::vector<T>::operator=(other));
    }
    CopyCountVector(CopyCountVector &&other) : std::vector<T>(std::move(other)) {}
    CopyCountVector &operator=(CopyCountVector &&other)
    {
        return static_cast<CopyCountVector &>(std::vector<T>::operator=(std::move(other)));
    }
    ~CopyCountVector() = default;

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
        ++copyCounter;
        return static_cast<CopyCountList &>(std::list<T>::operator=(other));
    }
    CopyCountList(CopyCountList &&other) : std::list<T>(std::move(other)) {}
    CopyCountList &operator=(CopyCountList &&other)
    {
        return static_cast<CopyCountList &>(std::list<T>::operator=(std::move(other)));
    }
    ~CopyCountList() = default;

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};

template <typename T, typename U>
class CopyCountMap : public std::map<T, U>
{
public:
    CopyCountMap() : std::map<T, U>() { ++createCounter; }
    CopyCountMap(const CopyCountMap &other) : std::map<T, U>(other) { ++copyCounter; }
    CopyCountMap &operator=(const CopyCountMap &other)
    {
        ++copyCounter;
        return static_cast<CopyCountMap &>(std::map<T, U>::operator=(other));
    }
    CopyCountMap(CopyCountMap &&other) : std::map<T, U>(std::move(other)) {}
    CopyCountMap &operator=(CopyCountMap &&other)
    {
        return static_cast<CopyCountMap &>(std::map<T, U>::operator=(std::move(other)));
    }
    ~CopyCountMap() = default;

    const U &operator[](const T &index) const { return std::map<T, U>::at(index); }
    const U &operator[](T &&index) const { return std::map<T, U>::at(index); }

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};

template <typename T, typename U>
class CopyCountUnorderedMap : public std::unordered_map<T, U>
{
public:
    CopyCountUnorderedMap() : std::unordered_map<T, U>() { ++createCounter; }
    CopyCountUnorderedMap(const CopyCountUnorderedMap &other) : std::unordered_map<T, U>(other) { ++copyCounter; }
    CopyCountUnorderedMap &operator=(const CopyCountUnorderedMap &other)
    {
        ++copyCounter;
        return static_cast<CopyCountUnorderedMap &>(std::unordered_map<T, U>::operator=(other));
    }
    CopyCountUnorderedMap(CopyCountUnorderedMap &&other) : std::unordered_map<T, U>(std::move(other)) {}
    CopyCountUnorderedMap &operator=(CopyCountUnorderedMap &&other)
    {
        return static_cast<CopyCountUnorderedMap &>(std::unordered_map<T, U>::operator=(std::move(other)));
    }
    ~CopyCountUnorderedMap() = default;

    const U &operator[](const T &index) const { return std::unordered_map<T, U>::at(index); }
    const U &operator[](T &&index) const { return std::unordered_map<T, U>::at(index); }

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};

namespace asynqro::traverse::detail::containers {
template <typename T1, typename T2>
void add(CopyCountMap<T1, T2> &container, const std::pair<T1, T2> &value)
{
    container.insert(value);
}
template <typename T1, typename T2>
void add(CopyCountUnorderedMap<T1, T2> &container, const std::pair<T1, T2> &value)
{
    container.insert(value);
}
} // namespace asynqro::traverse::detail::containers
#endif // COPYCOUNTCONTAINERS_H

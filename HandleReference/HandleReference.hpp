#pragma once

#include <algorithm>
#include <memory>


template<typename T>
class Handle;


template <typename T>
class Reference
{
public:
    friend class Handle<T>;

    Reference()
        : _cb(nullptr)
    { }

    ~Reference()
    {
        if(!_cb)
            return;

        --(_cb->refs);

        if (_cb->refs == Handle<T>::s_InvalidBit) // No more references, delete control
            delete _cb;
    }

    Reference(const Reference &other)
        : _cb(other._cb)
    {
        ++(_cb->refs);
    }

    Reference(Reference&& other) noexcept
        : _cb(other._cb)
    {
        other._cb = nullptr;
    }

    Reference& operator=(const Reference &other)
    {
        if(this == &other)
            return *this;

        _cb = other._cb;
        ++(_cb->refs);
    }

    Reference& operator=(Reference&& other) noexcept
    {
        _cb = other._cb;
        other._cb = nullptr;
    }


    /// Test whether the referenced memory still exists
    /// \return True if the memory exists, otherwise false
    bool IsValid() const
    {
        return _cb && (_cb->refs & Handle<T>::s_InvalidBit) == 0;
    }


    // Access
    /// Access the pointer to the underlying object with validity check
    /// \return Pointer to object or nullptr if it no longer exists
    T* Get() const
    {
        if(!IsValid())
            return nullptr;
        return _cb->ptr;
    }

    T* operator->() const { return Get(); }

    T& operator*() const { return *Get(); }

private:
    using ControlBlock = Handle<T>::ControlBlock;

    explicit Reference(ControlBlock* cb)
        : _cb(cb)
    {
        ++(_cb->refs);
    }

private:
    ControlBlock* _cb;
};


template<typename T>
class Handle
{
public:
    friend class Reference<T>;

    Handle()
        : _cb(nullptr)
    { }

    ~Handle()
    {
        if(!_cb) // Empty handle, do nothing
            return;

        delete _cb->ptr;

        if (_cb->refs == 0) // No references, delete control block
            delete _cb;
        else // References exist, set invalid bit
            _cb->refs |= s_InvalidBit;
    }

    friend void swap(Handle& a, Handle& b)
    {
        std::swap(a._cb, b._cb);
    }

    Handle(const Handle& other) = delete;

    Handle(Handle&& other) noexcept
    {
        this->~Handle<T>();
        _cb = other._cb;
        other._cb = nullptr;
    }

    Handle& operator=(const Handle& other) = delete;

    Handle& operator=(Handle&& other) noexcept
    {
        this->~Handle<T>();
        _cb = other._cb;
        other._cb = nullptr;

        return *this;
    }


    explicit operator bool() const
    {
        return (bool)_cb;
    }


    template<typename... Args>
    static Handle Make(Args&&... args)
    {
        return Handle(
                new ControlBlock(
                        0,
                        new T(std::forward<Args>(args)...)
                        )
                );
    }


    // Movement
    /// Moves the memory ownership by offering the Handle as an rvalue
    /// \return Handle rvalue
    Handle Move()
    {
        auto cb = _cb;
        _cb = nullptr;
        return Handle(cb);
    }

    void Free()
    {
        this->~Handle<T>();
        _cb = nullptr;
    }


    // Access
    T* operator->() const { return Get(); }

    T& operator*() const { return *Get(); }

    Reference<T> Ref()
    {
        return Reference<T>(_cb);
    }


    static const Handle<T> Emtpy;

private:
    struct ControlBlock
    {
        size_t refs;
        T* ptr;
    };

    explicit Handle(ControlBlock* cb)
            : _cb(cb)
    { }

    T* Get() const { return _cb->ptr; }


private:

    ControlBlock* _cb;

    static constexpr size_t s_InvalidBit = static_cast<size_t>(1) << (sizeof(size_t) * 8 - 1);
};
